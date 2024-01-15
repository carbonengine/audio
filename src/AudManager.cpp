#include "stdafx.h"
#include "AudManager.h"

#include <string>

#include <AK/AkWwiseSDKVersion.h>
#include <AK/MusicEngine/Common/AkMusicEngine.h> // Interactive music engine
#include <AK/SoundEngine/Common/AkSoundEngine.h> // Sound Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h> // Memory Manager
#include <AK/SoundEngine/Common/AkStreamMgrModule.h> // AkDeviceSettings, AkStreamMgrSettings
#include <AK/SoundEngine/Common/IAkStreamMgr.h> // Streaming Manager
#include <AK/SoundEngine/Common/AkModule.h> // Default memory and stream managers
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include <AK/Plugin/AkVorbisDecoderFactory.h>
#include <AK/Plugin/AkCompressorFXFactory.h>
#include <AK/Plugin/AkSilenceSourceFactory.h>
#include <AK/Plugin/AkParametricEQFXFactory.h>
#include <AK/Plugin/AkRoomVerbFXFactory.h>
#include <AK/Plugin/AkMatrixReverbFXFactory.h>
#include <AK/Plugin/AkMeterFXFactory.h>
#include <AK/Plugin/AkPeakLimiterFXFactory.h>
#include <AK/Plugin/AkFlangerFXFactory.h>
#include <AK/Plugin/AkGuitarDistortionFXFactory.h>
#include <AK/Plugin/AkHarmonizerFXFactory.h>
#include <AK/Plugin/AkConvolutionReverbFXFactory.h>
#include <AK/Plugin/AkTremoloFXFactory.h>
#include <AK/Plugin/AkStereoDelayFXFactory.h>
#include <AK/Plugin/AkPitchShifterFXFactory.h>
#include <AK/Plugin/AkDelayFXFactory.h>
#include <AK/Plugin/MasteringSuiteFXFactory.h>
#include <AK/Plugin/AkRecorderFXFactory.h>

#include "tbb/parallel_for.h"

#if _WIN32
#include "LowLevelIO/Win32/AkDefaultIOHookBlocking.h"
#elif __APPLE__
#include "LowLevelIO/POSIX/AkDefaultIOHookBlocking.h"
#endif

#include "AudActionLog.h"
#include "AudEmitter.h"
#include "AudSettings.h"
#include "AudStaticDataRepository.h"
#include "LogBridge.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudioManager" );

static void WwiseAssertHook( const char* in_pszExpression, const char* in_pszFileName, int in_lineNumber )
{
	CCP_LOGWARN_CH( s_ch, "Assert expression failed: %s in file %s at line %d", in_pszExpression, in_pszFileName, in_lineNumber );
}

AudManager::AudManager( IRoot* lockobj ) :
	m_tickInterval( 10 ),
	m_asyncOpen( true ),
	m_log(),
	m_audioCullingEnabled( true ),
	m_maxAwakeGameObjects( m_cullingInitSettings.maxAwakeGameObjects ),
	m_oneShotWindow( m_cullingInitSettings.oneShotWindow ),
	m_waitingOneShotWeight( m_cullingInitSettings.waitingOneShotWeight),
	m_usedEmitterWeight( m_cullingInitSettings.usedEmitterWeight ),
	m_rangeWeight( m_cullingInitSettings.rangeWeight ),
	m_playingEventsWeight( m_cullingInitSettings.activeSoundsWeight),
	m_visibleWeight( m_cullingInitSettings.visibleWeight ),
	m_playing2DWeight( m_cullingInitSettings.playing2DWeight ),
	m_playingVitalSoundWeight( m_cullingInitSettings.playingVitalSoundWeight ),
	m_weightMultiplier( m_cullingInitSettings.weightMultiplier ),
	m_soundBankMutex( "AudManager", "m_soundBankMutex" )
{}

AudManager::~AudManager()
{
	if( g_audioInitialized )
	{
		Disable();
	}
}

void AudManager::Process()
{
	if( g_audioInitialized )
	{
		if ( m_audioCullingEnabled && g_audioEnabled )
		{
			CullAudio();
		}

		// Process bank requests, events, positions, RTPC, etc.
		AK::SoundEngine::RenderAudio();

		//Update main thread queue
		g_mainThreadQueue->Update();

		if( m_log )
		{
			m_log->Flush();
		}
	}
}

//-----------------------------------------------------
// Description:
//   Compute a Wwise hash given a soundbank name. Wwise uses an internal hashing system for events, soundbanks and other 
//   Wwise objects. This will give you the ID they use for the given SoundBank name.
// Arguments:
//   soundBankName: The soundbank name you want hashed. This method works with a string that both ends with .bnk or not.
//-----------------------------------------------------
AkBankID AudManager::ComputeWwiseHashForSoundBank( const std::wstring& soundBankName )
{
	std::wstring filteredBankName = soundBankName.substr( 0, soundBankName.find( L".", 0 ) ); // Remove .bnk from the SoundBank name because Wwise does this when hashing.
	return AK::SoundEngine::GetIDFromString( filteredBankName.c_str() );
}

void AudManager::CullAudio()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	AudListenerPtr listener = GetListener();
	if ( listener != nullptr )
	{
		{
			// Calculate distance from the listener for all game objects and update their cumulative culling weight.
			CCP_STATS_ZONE("CullAudio_CalculateCullingWeight"); 
			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
			AkGameObjectID listenerID = listener->GetID();
			Vector3 listenerPosition = listener->GetPosition();
			tbb::parallel_for(
				tbb::blocked_range<size_t>( 0, m_gameObjects.size() ),
				[&]( const tbb::blocked_range<size_t>& range ) -> void {
					for( size_t index = range.begin(); index != range.end(); ++index )
					{
						AudGameObjResource* gameObject = m_gameObjects[index].second;
						if( gameObject->GetID() != listenerID )
						{
							float distanceSq = LengthSq( gameObject->GetPosition() - listenerPosition );
							gameObject->SetDistanceSqFromListener( distanceSq );
						}
						gameObject->CalculateCullingWeight( now );
					}
				} 
			);
		}
		{
			CCP_STATS_ZONE("CullAudio_SortGameObjects"); 
			// Sort all game objects by their cumulative culling weight. The larger the number the more likely to be culled.
			sort( m_gameObjects.begin(), m_gameObjects.end(),
				[]( const std::pair<AkGameObjectID, AudGameObjResource*>& a, const std::pair<AkGameObjectID, AudGameObjResource*>& b ) -> bool
					{
						return a.second->GetCullingWeight() < b.second->GetCullingWeight();
					});
		}
		{
			CCP_STATS_ZONE("CullAudio_WakeAndCullGameObjects"); 
			// Keep the first m_maxAwakeGameObjects game objects awake.
			int numAwake = 0;
			for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it)
			if ( numAwake > m_maxAwakeGameObjects )
			{
				if ( !it->second->IsCulled() )
				{
					it->second->Cull();
				}
			}
			else
			{
				if ( it->second->IsCulled() )
				{
					it->second->Wake();
				}
				++numAwake;
			}
		}
	}
}

bool AudManager::Init()
{
	if ( g_staticDataRepository == nullptr || !g_staticDataRepository->IsInitialized() )
	{
		CCP_LOGERR( "The static data repository in audio2 has not been generated and needs to exist for audio2 "
				    "to be able to run. See AudStaticDataRepository for more info on how to do this.");
		return false;
	}

	if( !InitLowLevel() )
	{
		CCP_LOGERR("Failed to initialize Low Level audio");
		return false;
	}

#ifndef AK_OPTIMIZED
	if( !InitCommunication() )
	{
		CCP_LOGERR("Failed to initialize audio : Communication");
		return false;
	}
	WwiseLogServerBridgeInit( AK::Monitor::ErrorLevel_All );
#endif

	if( !InitSound() )
	{
		CCP_LOGERR("Failed to initialize audio : Sound");
		return false;
	}

	if( !InitMusic() )
	{
		CCP_LOGERR("Failed to initialize audio : Music");
		return false;
	}

	g_audioInitialized = true;
	return true;
}

void AudManager::Terminate()
{
#ifndef AK_OPTIMIZED
	AK::Comm::Term();
#endif

	//
	// Terminate the music engine
	//
	AK::MusicEngine::Term();

	//Terminate sound engine.
	if( AK::SoundEngine::IsInitialized() )
	{
		// Terminate the sound engine
		AK::SoundEngine::Term();
	}

	// Terminate the streaming manager
	if( AK::IAkStreamMgr::Get() )
	{
		AK::IAkStreamMgr::Get()->Destroy();
	}

	//m_pLowLevelIO.p->Term();

	// Terminate the Memory Manager
	AK::MemoryMgr::Term();

	g_audioInitialized = false;
}

void AudManager::OnTick( Be::Time realTime, Be::Time simTime, void* cookie )
{
	Process();

	BeOS->NextScheduledEvent( m_tickInterval );
}

bool AudManager::InitLowLevel()
{
	CCP_LOG( "Audio Backend: Wwise(R) SDK Version %S. %s", g_wwiseVersion.c_str(), AK_WWISESDK_COPYRIGHT );
	//-----------------------------------------------------------------------------
	// Create and initialize an instance of the default memory manager. Note
	// that you can override the default memory manager with your own. Refer
	// to the Wwise SDK documentation for more information.
	//-----------------------------------------------------------------------------

	AkMemSettings memSettings;
	AK::MemoryMgr::GetDefaultSettings( memSettings );
	if( AK::MemoryMgr::Init( &memSettings ) != AK_Success )
	{
		CCP_LOGERR( "Failed to start Wwise Memory Manager" );
		return false;
	}

	//-----------------------------------------------------------------------------
	// Create and initialize an instance of the default streaming manager. Note
	// that you can override the default streaming manager with your own. Refer
	// to the SDK documentation for more information.
	//-----------------------------------------------------------------------------

	AkStreamMgrSettings streamSettings;
	AK::StreamMgr::GetDefaultSettings( streamSettings );
	if( !AK::StreamMgr::Create( streamSettings ) )
	{
		CCP_LOGERR( "Failed to start Wwise Stream Manager" );
		return false;
	}

	//-----------------------------------------------------------------------------
	// Create default IO device.
	//-----------------------------------------------------------------------------
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings( deviceSettings );

	if( m_lowLevelIO.Init( deviceSettings, m_asyncOpen ) != AK_Success )
	{
		CCP_LOGERR( "Failed to create Wwise Low Level IO Hook" );
		return false;
	}

	if( m_lowLevelIO.SetBasePath( m_settings->m_baseSoundBankPath.c_str() ) != AK_Success )
	{
		CCP_LOGERR( "Soundbank path %S is invalid and soundbanks will not be loaded correctly.", m_settings->m_baseSoundBankPath.c_str() );
		return false;
	}
	if( AK::StreamMgr::SetCurrentLanguage( m_settings->m_soundbankLanguage.c_str() ) != AK_Success )
	{
		CCP_LOGERR( "Setting soundbank language to %S failed and soundbanks will not be able to be loaded.", m_settings->m_soundbankLanguage.c_str() );
		return false;
	}

	return true;
}

bool AudManager::InitCommunication()
{
	//-----------------------------------------------------------------------------
	// Only used if audio dev is enabled. Initializes remote communication with
	// Wwise to allow for remote debugging.
	//-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
	AK::Comm::GetDefaultInitSettings( m_commSettings );
	AKPLATFORM::SafeStrCpy( &m_commSettings.szAppNetworkName[0], m_settings->m_applicationName.c_str(), AK_COMM_SETTINGS_MAX_STRING_SIZE );
	if( AK::Comm::Init( m_commSettings ) != AK_Success )
	{
		assert( !"Audio2: Could not init communication lib" );
		return false;
	}
	g_wwiseCommunicationEnabled = true;
#endif

	return true;
}

bool AudManager::InitSound()
{
	AkInitSettings initSettings;
	AkPlatformInitSettings platformInitSettings;
	AK::SoundEngine::GetDefaultInitSettings( initSettings );
	AK::SoundEngine::GetDefaultPlatformInitSettings( platformInitSettings );
	initSettings.uCommandQueueSize = 512000;

	if( AK::SoundEngine::Init( &initSettings, &platformInitSettings ) != AK_Success )
	{
		CCP_LOGERR( "Failed to initialize Wwise Sound Engine" );
		return false;
	}

	return true;
}

bool AudManager::InitMusic()
{
	AkMusicSettings musicSettings;
	AK::MusicEngine::GetDefaultInitSettings( musicSettings );
	if( AK::MusicEngine::Init( &musicSettings ) != AK_Success )
	{
		CCP_LOGERR( "Failed to initialize Wwise Music Engine" );
		return false;
	}

	return true;
}

bool AudManager::SetGlobalRTPC( const std::wstring& rtpcName, float value )
{
	if( g_audioInitialized )
	{
		AKRESULT result = AK::SoundEngine::SetRTPCValue( rtpcName.c_str(), value );
		if ( result != AK_Success )
		{
			CCP_LOGERR( "Failed to set global RTPC %S to %f. Receive Wwise result code %d", rtpcName.c_str(), value, result );
			return false;
		}
		g_audioManager->LogSetRTPC( 0, rtpcName, value );
		return true;
	}
	return false;
}


bool AudManager::SetState( const std::wstring& stateGroup, const std::wstring& stateName )
{
	if( g_audioInitialized )
	{
		// SetState always returns True so no need to check the result.
		AK::SoundEngine::SetState( stateGroup.c_str(), stateName.c_str() );
		return true;
	}
	return false;
}

void AudManager::UpdateSettings( AudSettings* settings )
{
	m_settings = settings;
}

void AudManager::LoadBank( const std::wstring& name )
{
	if( g_audioEnabled )
	{
		AkBankID bankID = ComputeWwiseHashForSoundBank( name );
		SoundBankStatus soundBankStatus = GetSoundBankStatus( bankID );
		if( soundBankStatus == SoundBankStatus::LOADED || soundBankStatus == SoundBankStatus::LOADING )
		{ 
			std::string soundBankStr = ( soundBankStatus == SoundBankStatus::LOADED ) ? "loaded" : "loading";
			CCP_LOG( "SoundBank %S has been requested to load when it is already %s.", name.c_str(), soundBankStr.c_str() );
			return;
		}

		CcpAutoMutex mutex( m_soundBankMutex );
		m_soundBankInfoMap[bankID] = SoundBankInfo{ SoundBankStatus::LOADING, bankID, name };

		AkBankID outBankID;
		AkBankCallbackFunc callback = &AudManager::LoadBankCallback;
		AKRESULT out_result = AK::SoundEngine::LoadBank( name.c_str(), callback, this, outBankID );

		if( outBankID != bankID )
		{
			CCP_LOGERR( "The SoundBank ID %d returned from LoadBank is different than the computed SoundBank ID %d. There is something fishy about that.", outBankID, bankID );
			StopTrackingSoundBank( bankID );	
			m_soundBankInfoMap[outBankID] = SoundBankInfo{ SoundBankStatus::LOADING, outBankID, name };
		}

		if( out_result != AK_Success )
		{
			StopTrackingSoundBank( outBankID );
			CCP_LOGERR( "SoundBank %S failed to be scheduled to load", name.c_str() );
		}
		else
		{
			CCP_LOG( "SoundBank %S scheduled to be loaded.", name.c_str() );
		}
	}
}

void AudManager::LoadBankCallback( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, void* in_pCookie )
{
	AudManager* audManagerPtr = reinterpret_cast<AudManager*>( in_pCookie );
	SoundBankStatus soundBankStatus = audManagerPtr->GetSoundBankStatus( in_bankID );
	if( soundBankStatus != SoundBankStatus::NOT_LOADED )
	{
		std::wstring soundBankName = audManagerPtr->GetSoundBankName( in_bankID );
		if( in_eLoadResult == AK_Success || in_eLoadResult == AK_BankAlreadyLoaded )
		{
			audManagerPtr->UpdateSoundBankStatus( in_bankID, SoundBankStatus::LOADED );
			CCP_LOG( "SoundBank %S was successfully loaded.", soundBankName.c_str() );
		}
		else
		{
			CCP_LOGERR( "Soundbank %S failed to be loaded with Wwise error %d", soundBankName.c_str(), in_eLoadResult );
			audManagerPtr->StopTrackingSoundBank( in_bankID );
		}
	}
	else
	{
		CCP_LOGERR( "LoadBankCallback is being called for bank ID %d which is not being tracked.", in_bankID );
	}
}

void AudManager::UnloadBank( const std::wstring& name )
{
	if( g_audioEnabled )
	{
		AkBankID bankID = ComputeWwiseHashForSoundBank( name );
		SoundBankStatus soundBankStatus = GetSoundBankStatus( bankID );

		if( soundBankStatus == SoundBankStatus::NOT_LOADED )
		{
			CCP_LOGERR( "SoundBank %S was requested to be unloaded when it was not actually loaded to begin with.", name.c_str() );
			return;
		}
		else if( soundBankStatus == SoundBankStatus::UNLOADING )
		{ 
			CCP_LOG( "SoundBank %S has been requested to unload when it is already unloading.", name.c_str() );
			return;
		}

		UpdateSoundBankStatus( bankID, SoundBankStatus::UNLOADING );

		// The pInMemoryBankPtr can be NULL is NULL is passed when loading the bank.
		const void* pInMemoryBankPtr = NULL;
		AkBankCallbackFunc callback = &AudManager::UnloadBankCallback;
		AKRESULT result = AK::SoundEngine::UnloadBank( name.c_str(), pInMemoryBankPtr, callback, this);
		if( result != AK_Success )
		{
			StopTrackingSoundBank( bankID );
			CCP_LOGERR( "SoundBank %S failed to be scheduled to be unloaded.", name.c_str() );
		}
		else
		{
			CCP_LOG( "SoundBank %S was successfully scheduled to be unloaded.", name.c_str() );
		}

	}
}

void AudManager::UnloadBankCallback( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, void* in_pCookie )
{
	AudManager* audManagerPtr = reinterpret_cast<AudManager*>(in_pCookie);
	SoundBankStatus soundBankStatus = audManagerPtr->GetSoundBankStatus( in_bankID );
	if( soundBankStatus != SoundBankStatus::NOT_LOADED )
	{
		std::wstring soundBankName = audManagerPtr->GetSoundBankName( in_bankID );
		if( in_eLoadResult != AK_Success )
		{
			CCP_LOGERR( "SoundBank %S failed to be unloaded.", soundBankName.c_str() );
		}
		else
		{
			CCP_LOG( "SoundBank %S was successfully unloaded.", soundBankName.c_str() );
		}
		audManagerPtr->StopTrackingSoundBank( in_bankID );
	}
	else
	{
		CCP_LOGERR( "UnloadBankCallback was called for bank ID %d which is not being tracked.", in_bankID );
	}
}

std::wstring AudManager::GetSoundBankName(const AkBankID bankID)
{
	CcpAutoMutex mutex( m_soundBankMutex );
	auto mapIterator = m_soundBankInfoMap.find( bankID );
	if( mapIterator != m_soundBankInfoMap.end() )
	{
		return mapIterator->second.soundBankName;
	}
	return L"";
}

SoundBankStatus AudManager::GetSoundBankStatus( const AkBankID bankID ) 
{
	CcpAutoMutex mutex( m_soundBankMutex );
	auto mapIterator = m_soundBankInfoMap.find( bankID );
	if( mapIterator != m_soundBankInfoMap.end() )
	{
		return mapIterator->second.soundBankStatus;
	}
	return SoundBankStatus::NOT_LOADED;
}

SoundBankStatus AudManager::GetSoundBankStatus( const std::wstring& soundBankName )
{
	CcpAutoMutex mutex( m_soundBankMutex );
	for (auto it = m_soundBankInfoMap.begin(); it != m_soundBankInfoMap.end(); ++it)
	{
		if( it->second.soundBankName == soundBankName )
		{
			return it->second.soundBankStatus;
		}
		
	}
	return SoundBankStatus::NOT_LOADED;
}

void AudManager::StopTrackingSoundBank( const AkBankID bankID )
{
	CcpAutoMutex mutex( m_soundBankMutex );
	m_soundBankInfoMap.erase( bankID );
}

void AudManager::UpdateSoundBankStatus( const AkBankID bankID, const SoundBankStatus soundBankStatus )
{
	CcpAutoMutex mutex( m_soundBankMutex );
	auto mapIterator = m_soundBankInfoMap.find( bankID );
	if( mapIterator != m_soundBankInfoMap.end() )
	{
		mapIterator->second.soundBankStatus = soundBankStatus;

		if (soundBankStatus == SoundBankStatus::LOADED)
		{
			for( auto it = mapIterator->second.waitingEventsAfterLoad.begin(); it != mapIterator->second.waitingEventsAfterLoad.end(); ++it )
			{
				if( it->first != nullptr )
				{
					it->first->PostEvent( it->second );
				}
			}
			mapIterator->second.waitingEventsAfterLoad.clear();
		}
	}
}

void AudManager::ClearBanks()
{
	// Unloads all banks currently loaded in Wwise.
	if( g_audioInitialized )
	{
		AKRESULT result = AK::SoundEngine::ClearBanks();
		if( result == AK_Fail )
		{
			CCP_LOGERR( "AK::SoundEngine::ClearBanks failed" );
			return;
		}
	
		CcpAutoMutex mutex( m_soundBankMutex );
		m_soundBankInfoMap.clear();
		CCP_LOG( "All SoundBanks unloaded in Wwise" );
	}
}

//-----------------------------------------------------
// Description:
//   Disable CarbonAudio which culls all game objects, unloads all SoundBanks, terminates
//   the sound engine and stops the audio thread.
//-----------------------------------------------------
void AudManager::Disable() 
{
	if ( g_audioEnabled == false )
	{
		return;
	}

	StopAll();
	for( auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it )
	{
		( *it->second ).Cull();
	}
	ClearBanks();
	Terminate();
	g_audioEnabled = false;
	BeOS->UnregisterForTicks( this, (void*)"Audio::Tick" );
}

//-----------------------------------------------------
// Description:
//   Enable CarbonAudio which will initialize the sound engine, load the given SoundBanks,
//   Wake up all audio emitters (if sound prioritization is enabled) and registers the audio thread.
// Arguments:
//   soundBanks - The name of all soundbanks you want to load when enabling the sound engine 
//				  by name (relative to the base SoundBank path). Note: The Init SoundBank is implicitly
//				  loaded and does not need to be passed in here.
//-----------------------------------------------------
void AudManager::Enable(BankVector soundBanksToLoad) 
{
	if ( g_audioEnabled )
	{
		return;
	}
	if( !Init() )
	{
		CCP_LOGERR( "Failed to initialize audio" );
		return;
	}

	g_audioEnabled = true;
	LoadBank( L"Init.bnk" );

	BankVector::iterator bankEnd = soundBanksToLoad.end();
	for( BankVector::iterator it = soundBanksToLoad.begin(); it != bankEnd; ++it )
	{
		LoadBank( *it );
	}

	for( auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it )
	{
		( *it->second ).Wake();
	}

	BeOS->RegisterForTicks( this, (void*)"Audio::Tick" );
	return;
}

const std::vector<std::wstring> AudManager::GetLoadedSoundBanks()
{
	BankVector loadedSoundBanks;
	CcpAutoMutex mutex( m_soundBankMutex );
	for( auto it = m_soundBankInfoMap.begin(); it != m_soundBankInfoMap.end(); ++it )
	{
		if( it->second.soundBankStatus == SoundBankStatus::LOADED )
		{
			loadedSoundBanks.push_back( it->second.soundBankName );
		}
	}

	return loadedSoundBanks;
}

//-----------------------------------------------------
// Description:
//   Retrieve the given emitter if it is currently alive. 
// Arguments:
//   emitterID - The ID of the emitter you want to retrieve. 
//-----------------------------------------------------
AudGameObjResource* AudManager::GetAudioEmitter( AkGameObjectID emitterID )
{
	auto it = m_gameObjects.begin();
	while( it != m_gameObjects.end() )
	{
		if ( it->first == emitterID )
		{
			return it->second;
		}
		else
		{
			++it;
		}
	}
	return nullptr;
}

//-----------------------------------------------------
// Description:
//   Retrieve an event name for the given playing ID if the given emitter is playing that event.
// Arguments:
//   emitterID - The ID of the emitter that is playing the event whose name you want to retrieve.
//	 playingID - The playing ID of the event whose name you want to retrieve.
// Returns:
//   An empty wstring if the given emitter is not playing an event with the given playing ID.
//-----------------------------------------------------
#ifndef AK_OPTIMIZED
const std::wstring AudManager::GetEventName( AkGameObjectID emitterID,  AkPlayingID playingID )
{
	AudGameObjResource* emitter = GetAudioEmitter( emitterID );
	std::map<AkPlayingID, std::wstring> playingEvents = emitter->GetPlayingEvents();
	auto it = playingEvents.find( playingID );
	if( it != playingEvents.end() )
	{
		return it->second;
	}
	return L"";
}
#endif

void AudManager::StopAll()
{
	if( g_audioInitialized )
	{
		for( auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it )
		{
			( *it->second ).StopAll();
		}
	}
}

void AudManager::RegisterGameObject( AkGameObjectID emitterID, AudGameObjResource* emitter )
{
	m_gameObjects.push_back( std::make_pair( emitterID, emitter ) );
}

//-----------------------------------------------------
// Description:
//   Register an event to be sent to Wwise after it is done loading (e.g. in the LoadBank callback function). 
//   Only works with soundbanks in the SoundBankStatus::Loading state.
// Arguments:
//   bankID - The ID of the SoundBank you want to register an event on after it loads. 
//	 eventName - The event you want to be sent when the given bankID is done loading. 
//	 emitter - The audio emitter you want the registered event to be sent to.
//-----------------------------------------------------
void AudManager::RegisterEventAfterSoundBankLoad( std::wstring& soundBankName, std::wstring& eventName, AudGameObjResource* emitter )
{
	CcpAutoMutex mutex( m_soundBankMutex );
	for( auto it = m_soundBankInfoMap.begin(); it != m_soundBankInfoMap.end(); ++it )
	{
		if( it->second.soundBankName == soundBankName )
		{
			it->second.waitingEventsAfterLoad.push_back( std::make_pair( emitter, eventName ) );
			CCP_LOG("Event %S registered to be sent to audio emitter %d after SoundBank %S is done loading.", eventName.c_str(), emitter->GetID(), soundBankName.c_str() );
		}
	}
}

void AudManager::UnregisterGameObject( AkGameObjectID emitterID )
{
	m_gameObjects.erase(
		std::remove_if( 
			begin( m_gameObjects ), end( m_gameObjects ), [emitterID]( const std::pair<AkGameObjectID, AudGameObjResource*>& p ) 
			{
				return p.first == emitterID;
			} 
		), end( m_gameObjects ) 
	);
}

void AudManager::DisableAudioCulling()
{
	for ( auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it )
	{
		if ( it->second->IsCulled() )
		{
			( *it->second ).Wake();
		}
	}
	m_audioCullingEnabled = false;
	CCP_LOG_CH( s_ch, "The sound prioritization system has been disabled." );
}

void AudManager::EnableAudioCulling()
{
	m_audioCullingEnabled = true;
	CCP_LOG_CH( s_ch, "The sound prioritization system has been enabled." );
}

void AudManager::ResetCullingSettings()
{
	m_maxAwakeGameObjects = m_cullingInitSettings.maxAwakeGameObjects;
	m_waitingOneShotWeight = m_cullingInitSettings.waitingOneShotWeight;
	m_usedEmitterWeight = m_cullingInitSettings.usedEmitterWeight;
	m_rangeWeight = m_cullingInitSettings.rangeWeight;
	m_playingEventsWeight = m_cullingInitSettings.activeSoundsWeight;
	m_visibleWeight = m_cullingInitSettings.visibleWeight;
	m_playing2DWeight = m_cullingInitSettings.playing2DWeight;
	m_playingVitalSoundWeight = m_cullingInitSettings.playingVitalSoundWeight;
	m_weightMultiplier = m_cullingInitSettings.weightMultiplier;
}

void AudManager::LogPostEvent( AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name )
{
	if( m_log )
	{
		m_log->LogPostEvent( emitterID, playID, eventID, name );
	}
}

void AudManager::LogExecuteActionOnPlayingID( AkGameObjectID emitterID, AkPlayingID playID, const std::wstring& action )
{
	if( m_log )
	{
		m_log->LogExecuteActionOnPlayingID( emitterID, playID, action );
	}
}

void AudManager::LogSetSwitch( AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state )
{
	if( m_log )
	{
		m_log->LogSetSwitch( emitterID, group, state );
	}
}

void AudManager::LogSetState( const std::wstring& group, const std::wstring& state )
{
	if( m_log )
	{
		m_log->LogSetState( group, state );
	}
}

void AudManager::LogSetRTPC( AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID )
{
	if( m_log )
	{
		m_log->LogSetRTPC( emitterID, name, value, playID );
	}
}

void AudManager::EnableDebugDisplayAllEmitters()
{
	g_debugDisplayAllEmitters = true;
}

void AudManager::DisableDebugDisplayAllEmitters()
{
	g_debugDisplayAllEmitters = false;
}

bool AudManager::GetDebugDisplayAllEmitters()
{
	return g_debugDisplayAllEmitters;
}

bool AudManager::GetAudioCullingEnabled() const
{
	return m_audioCullingEnabled;
}

long long AudManager::GetOneShotWindow() const
{
	return m_oneShotWindow;
}

float AudManager::GetPlaying2DWeight() const
{
	return m_weightMultiplier * m_playing2DWeight;
}

float AudManager::GetPlayingEventsWeight() const
{
	return m_weightMultiplier * m_playingEventsWeight;
}

float AudManager::GetPlayingVitalSoundWeight() const
{
	return m_weightMultiplier * m_playingVitalSoundWeight;
}

float AudManager::GetRangeWeight() const
{
	return m_weightMultiplier * m_rangeWeight;
}

float AudManager::GetUsedEmitterWeight() const 
{
	return m_weightMultiplier * m_usedEmitterWeight;
}

float AudManager::GetVisibleWeight() const
{
	return m_weightMultiplier * m_visibleWeight;
}

float AudManager::GetWaitingOneShotWeight() const 
{
	return m_weightMultiplier * m_waitingOneShotWeight;
}

void AudManager::SetOneShotWindow( long long numMilliseconds )
{
	m_oneShotWindow = numMilliseconds;
}

void AudManager::SetPlaying2DWeight( float weight )
{
	m_playing2DWeight = weight;
}

void AudManager::SetPlayingEventsWeight( float weight )
{
	m_playingEventsWeight = weight;
}

void AudManager::SetPlayingVitalSoundWeight( float weight )
{
	m_playingVitalSoundWeight = weight;
}

void AudManager::SetRangeWeight( float weight )
{
	m_rangeWeight = weight;
}

void AudManager::SetUsedEmitterWeight( float weight )
{
	m_usedEmitterWeight = weight;
}

void AudManager::SetVisibleWeight( float weight )
{
	m_visibleWeight = weight;
}

void AudManager::SetWaitingOneShotWeight( float weight )
{
	m_waitingOneShotWeight = weight;
}

AudListenerPtr AudManager::GetListener()
{
	AudGameObjResourcePtr listenerGameObj = GetAudioEmitter( LISTENER_GAME_OBJ_ID );
	AudListenerPtr listener = dynamic_cast<AudListener*>( listenerGameObj.p );
	return listener;
}

std::vector<std::pair<AkGameObjectID, AudGameObjResource*>> AudManager::GetPrioritizedAudioEmitters()
{
	return m_gameObjects;
}
