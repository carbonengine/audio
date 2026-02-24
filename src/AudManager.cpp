#include "stdafx.h"
#include "AudManager.h"

#include <AK/Plugin/AkCompressorFXFactory.h>
#include <AK/Plugin/AkConvolutionReverbFXFactory.h>
#include <AK/Plugin/AkDelayFXFactory.h>
#include <AK/Plugin/AkFlangerFXFactory.h>
#include <AK/Plugin/AkGuitarDistortionFXFactory.h>
#include <AK/Plugin/AkHarmonizerFXFactory.h>
#include <AK/Plugin/AkMatrixReverbFXFactory.h>
#include <AK/Plugin/AkMeterFXFactory.h>
#include <AK/Plugin/AkParametricEQFXFactory.h>
#include <AK/Plugin/AkPeakLimiterFXFactory.h>
#include <AK/Plugin/AkPitchShifterFXFactory.h>
#include <AK/Plugin/AkRecorderFXFactory.h>
#include <AK/Plugin/AkRoomVerbFXFactory.h>
#include <AK/Plugin/AkSilenceSourceFactory.h>
#include <AK/Plugin/AkStereoDelayFXFactory.h>
#include <AK/Plugin/AkTremoloFXFactory.h>
#include <AK/Plugin/AkVorbisDecoderFactory.h>
#include <AK/Plugin/MasteringSuiteFXFactory.h>
#include <AK/Plugin/Ak3DAudioBedMixerFXFactory.h>

#include "tbb/parallel_for.h"

#if _WIN32
#include "LowLevelIO/Win32/AkDefaultIOHookDeferred.h"
#elif __APPLE__
#include "LowLevelIO/POSIX/AkDefaultIOHookDeferred.h"
#endif

#include "AudActionLog.h"
#include "AudEmitter.h"
#include "AudSettings.h"
#include "AudStaticDataRepository.h"
#include "LogBridge.h"

#ifndef AK_OPTIMIZED
// Declare statistics for SoundEngine
CCP_STATS_DECLARE( sampleRate, "CarbonAudio/AudManager/SampleRate", false, CST_COUNTER_LOW, "The current audio sample rate." );
CCP_STATS_DECLARE( numSamplesPerFrame, "CarbonAudio/AudManager/NumSamplesPerFrame", false, CST_COUNTER_LOW, "Number of samples per audio frame (256, 512, 1024, or 2048)." );
#ifndef AK_MAC_OS_X
CCP_STATS_DECLARE( maxSpatialAudioObjects, "CarbonAudio/AudManager/maxSpatialAudioObjects", false, CST_COUNTER_LOW, " Dictates how many Spatial Sound dynamic objects will be reserved by the System sink." );
#endif

// Declare statistics for AkResourceMonitorDataSummary
CCP_STATS_DECLARE( totalCPU, "CarbonAudio/AudManager/TotalCPU", false, CST_COUNTER_LOW, "Percentage of the CPU time used for processing audio." );
CCP_STATS_DECLARE( pluginCPU, "CarbonAudio/AudManager/PluginCPU", false, CST_COUNTER_LOW, "Percentage of the CPU time used by plugin processing." );
CCP_STATS_DECLARE( physicalVoices, "CarbonAudio/AudManager/PhysicalVoices", false, CST_COUNTER_LOW, "Number of active physical voices." );
CCP_STATS_DECLARE( virtualVoices, "CarbonAudio/AudManager/VirtualVoices", false, CST_COUNTER_LOW, "Number of active virtual voices." );
CCP_STATS_DECLARE( totalVoices, "CarbonAudio/AudManager/TotalVoices", false, CST_COUNTER_LOW, "Number of active physical and virtual voices." );
CCP_STATS_DECLARE( nbActiveEvents, "CarbonAudio/AudManager/ActiveEvents", false, CST_COUNTER_LOW, "Number of events triggered at a certain time." );
#endif

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudioManager" );

static void WwiseAssertHook( const char* in_pszExpression, const char* in_pszFileName, int in_lineNumber )
{
	CCP_LOGWARN_CH( s_ch, "Assert expression failed: %s in file %s at line %d", in_pszExpression, in_pszFileName, in_lineNumber );
}

// Used for spatial audio related functions, must be static to be able to be accessed by a Wwise callback that does not use cookies.
static CcpMutex s_audioDeviceMutex( "AudManager", "s_audioDeviceMutex" );
static bool s_systemSupportsSpatialAudio = false;
static BlueScriptCallback s_audioDeviceChangeCallback;
std::atomic<size_t> AudManager::s_lastLoggedDeviceHash{ 0 };


AudManager::AudManager( IRoot* lockobj ) :
	m_asyncOpen( true ),
	m_log(),
	m_spatialAudioEnabled( true ),
	m_moniteredParametersMapMutex( "AudManager", "m_monitoredParametersMapMutex" ),
	m_soundBankMutex( "AudManager", "m_soundBankMutex" ),
	m_isProfilerCapturing( false ),
	m_audioCullingEnabled( true )
{
	// Initialize sound prioritization system
	m_soundPrioritization = new SoundPrioritization();
	m_obstruction = new AudObstruction();
}

AudManager::~AudManager()
{
	// Clean up sound prioritization system
	delete m_soundPrioritization;
	delete m_obstruction;

	if( g_audioInitialized )
	{
		Disable();
	}
}

void AudManager::Process()
{
	if( g_audioInitialized )
	{
		if( m_soundPrioritization->GetAudioCullingEnabled() && g_audioEnabled )
		{
			m_soundPrioritization->CullAudio();
		}

		// Smooth obstruction values using Wwise's computed diffraction/transmission paths
		m_obstruction->Update( LISTENER_GAME_OBJ_ID,
			m_soundPrioritization->GetPrioritizedAudioObjects() );

		// Process bank requests, events, positions, RTPC, etc.
		AK::SoundEngine::RenderAudio();

		// Update main thread queue
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

bool AudManager::Init()
{
	if( g_staticDataRepository == nullptr || !g_staticDataRepository->IsInitialized() )
	{
		CCP_LOGERR( "The static data repository in audio2 has not been generated and needs to exist for audio2 "
					"to be able to run. See AudStaticDataRepository for more info on how to do this." );
		return false;
	}

	if( !InitLowLevel() )
	{
		CCP_LOGERR( "Failed to initialize Low Level audio" );
		return false;
	}
	if( !InitSound() )
	{
		CCP_LOGERR( "Failed to initialize audio : Sound" );
		return false;
	}

	if( !InitMusic() )
	{
		CCP_LOGERR( "Failed to initialize audio : Music" );
		return false;
	}

	if( !InitSpatialAudioGeometry() )
	{
		CCP_LOGERR( "Failed to initialize audio : Spatial Audio Geometry" );
		return false;
	}

#ifndef AK_OPTIMIZED
	if( !InitCommunication() )
	{
		CCP_LOGERR( "Failed to initialize audio : Communication" );
		return false;
	}
	WwiseLogServerBridgeInit( AK::Monitor::ErrorLevel_All );
#endif

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
	m_lowLevelIO.Term();
	if( AK::IAkStreamMgr::Get() )
	{
		AK::IAkStreamMgr::Get()->Destroy();
	}

	// Terminate the Memory Manager
	AK::MemoryMgr::Term();

	g_audioInitialized = false;
}

void AudManager::OnTick( Be::Time realTime, Be::Time simTime, void* cookie )
{
	Process();
}

bool AudManager::InitLowLevel()
{
	CCP_LOG_CH( s_ch, "Audio Backend: Wwise(R) SDK Version %s. %s", g_wwiseVersion.c_str(), AK_WWISESDK_COPYRIGHT );
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

	if (m_lowLevelIO.Init(deviceSettings) != AK_Success)
	{
		CCP_LOGERR("Failed to create Wwise Low Level IO Hook");
		return false;
	}

	if (m_lowLevelIO.SetBasePath(m_settings->m_baseSoundBankPath.c_str()) != AK_Success)
	{
		CCP_LOGERR("Soundbank path %S is invalid", m_settings->m_baseSoundBankPath.c_str());
		return false;
	}

	if (m_lowLevelIO.SetEssentialPath(m_settings->m_essentialPath.c_str()) != AK_Success)
	{
		CCP_LOGERR("Essential path %S is invalid", m_settings->m_essentialPath.c_str());
		return false;
	}

	if (m_lowLevelIO.SetAudioSrcPath(m_settings->m_audioSrcPath.c_str()) != AK_Success)
	{
		CCP_LOGERR("Audio source path %S is invalid", m_settings->m_audioSrcPath.c_str());
		return false;
	}

	if (AK::StreamMgr::SetCurrentLanguage(m_settings->m_soundbankLanguage.c_str()) != AK_Success)
	{
		CCP_LOGERR("Setting soundbank language to %S failed and soundbanks will not be able to be loaded.", m_settings->m_soundbankLanguage.c_str());
		return false;
	}

	return true;
}

void AudManager::RegisterGameObject( AkGameObjectID gameObjID, AudGameObjResource* gameObj )
{
	if( !gameObj )
	{
		CCP_LOGERR( "Attempted to register a null game object with ID %d", gameObjID );
		return;
	}

	if( m_soundPrioritization )
	{
		m_soundPrioritization->RegisterGameObject( static_cast<IPrioritizedObject*>( gameObj ) );
	}
}

void AudManager::UnregisterGameObject( AkGameObjectID gameObjID )
{
	if( m_soundPrioritization )
	{
		m_soundPrioritization->UnregisterGameObject( gameObjID );
	}
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
#ifndef AK_OPTIMIZED
	initSettings.fnProfilerPopTimer = AkPlatformProfilerPopTimer;
	initSettings.fnProfilerPushTimer = AkPlatformProfilerPushTimer;
	initSettings.fnProfilerPostMarker = AkPlatformProfilerPostmarker;
#endif

	AkUniqueID deviceSharesetID;
	if( m_settings->m_spatialAudioEnabled )
	{
		deviceSharesetID = AK::SoundEngine::GetIDFromString( m_settings->m_spatialAudioDeviceName.c_str() );
		CCP_LOG( "Carbon audio will be initialized with spatial audio enabled by using the Wwise audio "
				 "device named %S.", m_settings->m_spatialAudioDeviceName.c_str() );
		m_spatialAudioEnabled = true;
	}
	else
	{
		deviceSharesetID = AK::SoundEngine::GetIDFromString( m_settings->m_stereoAudioDeviceName.c_str() );
		CCP_LOG( "Carbon audio will be initialized with spatial audio disabled by using the Wwise audio "
				 "device named %S.",
				 m_settings->m_stereoAudioDeviceName.c_str() );
		m_spatialAudioEnabled = false;
	}
	initSettings.settingsMainOutput.audioDeviceShareset = deviceSharesetID;

	if( !AK::SoundEngine::IsInitialized() )
	{
		if( AK::SoundEngine::Init( &initSettings, &platformInitSettings ) != AK_Success )
		{
			CCP_LOGERR( "Failed to initialize Wwise Sound Engine" );
			return false;
		}

		if( AK::SoundEngine::RegisterGlobalCallback( GlobalCallbackEndRender, AkGlobalCallbackLocation_EndRender, this ) != AK_Success )
		{
			CCP_LOGERR( "Registering for Wwise's end render callback failed! Audio will continue to function correctly except audio driven visuals will not work!" );
		}

		AKRESULT result = AK::SoundEngine::RegisterAudioDeviceStatusCallback( &AudManager::AudioDeviceStatusChangeCallback );
		if( result != AK_Success )
		{
			CCP_LOGERR( "Failed to register an audio device status callback." );
		}
	}

#ifndef AK_OPTIMIZED
	AKRESULT result = AK::SoundEngine::RegisterResourceMonitorCallback( ResourceMonitorCallback );
	if( result != AK_Success )
	{
		CCP_LOGERR( "Failed to register resource monitor callback" );
	}
	CCP_STATS_SET( sampleRate, AK::SoundEngine::GetSampleRate() );
	CCP_STATS_SET( numSamplesPerFrame, initSettings.uNumSamplesPerFrame );
#ifndef AK_MAC_OS_X
	CCP_STATS_SET( maxSpatialAudioObjects, platformInitSettings.uMaxSystemAudioObjects );
#endif
#endif

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

bool AudManager::InitSpatialAudioGeometry()
{
	AkSpatialAudioInitSettings spatialSettings;

	spatialSettings.bEnableGeometricDiffractionAndTransmission = true;

	if( AK::SpatialAudio::Init( spatialSettings ) != AK_Success )
	{
		CCP_LOGERR( "Failed to initialize Wwise Spatial Audio for geometry processing" );
		return false;
	}

	CCP_LOG_CH( s_ch, "Wwise Spatial Audio initialized for geometry-based occlusion/diffraction" );
	return true;
}

bool AudManager::SetGlobalRTPC( const std::wstring& rtpcName, float value )
{
	if( g_audioInitialized && !g_shuttingDown)
	{
		AKRESULT result = AK::SoundEngine::SetRTPCValue( rtpcName.c_str(), value );
		if( result != AK_Success )
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
	if( g_audioInitialized && !g_shuttingDown )
	{
		// SetState always returns True so no need to check the result.
		AK::SoundEngine::SetState( stateGroup.c_str(), stateName.c_str() );
		return true;
	}
	return false;
}

//-----------------------------------------------------
// Description:
//   Signals whether Carbon Audio supports spatial audio features on this operating system.
//-----------------------------------------------------
const bool AudManager::SpatialAudioIsSupported()
{
	return s_systemSupportsSpatialAudio;
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
			if( soundBankStatus == SoundBankStatus::LOADING )
			{
				audManagerPtr->UpdateSoundBankStatus( in_bankID, SoundBankStatus::LOADED );
				CCP_LOG( "SoundBank %S was successfully loaded.", soundBankName.c_str() );
			}
			else if( soundBankStatus == SoundBankStatus::UNLOADING )
			{
				CCP_LOG( "SoundBank %S was supposed to be loaded, but has since been requested to be unloaded.", soundBankName.c_str() );
			}
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

		switch( soundBankStatus )
		{
		case SoundBankStatus::NOT_LOADED: {
			CCP_LOGERR( "SoundBank %S was requested to be unloaded when it was not actually loaded to begin with.", name.c_str() );
			break;
		}
		case SoundBankStatus::UNLOADING: {
			CCP_LOG( "SoundBank %S has been requested to unload when it is already unloading.", name.c_str() );
			break;
		}
		default: {
			UpdateSoundBankStatus( bankID, SoundBankStatus::UNLOADING );

			// The pInMemoryBankPtr can be NULL is NULL is passed when loading the bank.
			const void* pInMemoryBankPtr = NULL;
			AkBankCallbackFunc callback = &AudManager::UnloadBankCallback;
			AKRESULT result = AK::SoundEngine::UnloadBank( name.c_str(), pInMemoryBankPtr, callback, this );
			if( result != AK_Success )
			{
				StopTrackingSoundBank( bankID );
				CCP_LOGERR( "SoundBank %S failed to be scheduled to be unloaded.", name.c_str() );
			}
			else
			{
				CCP_LOG( "SoundBank %S scheduled to be unloaded.", name.c_str() );
			}
		}
		}
	}
}

void AudManager::UnloadBankCallback( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, void* in_pCookie )
{
	AudManager* audManagerPtr = reinterpret_cast<AudManager*>( in_pCookie );
	SoundBankStatus soundBankStatus = audManagerPtr->GetSoundBankStatus( in_bankID );
	if( soundBankStatus != SoundBankStatus::NOT_LOADED )
	{
		std::wstring soundBankName = audManagerPtr->GetSoundBankName( in_bankID );
		if( in_eLoadResult != AK_Success )
		{
			CCP_LOGERR( "SoundBank %S failed to be unloaded.", soundBankName.c_str() );
			audManagerPtr->StopTrackingSoundBank( in_bankID );
		}
		else
		{
			if( soundBankStatus == SoundBankStatus::UNLOADING )
			{
				CCP_LOG( "SoundBank %S was successfully unloaded.", soundBankName.c_str() );
				audManagerPtr->StopTrackingSoundBank( in_bankID );
			}
			else if( soundBankStatus == SoundBankStatus::LOADING )
			{
				CCP_LOG( "SoundBank %S was supposed to be unloaded but has since been requested to be loaded. ", soundBankName.c_str() );
			}
		}
	}
	else
	{
		CCP_LOGERR( "UnloadBankCallback was called for bank ID %d which is not being tracked.", in_bankID );
	}
}

std::wstring AudManager::GetSoundBankName( const AkBankID bankID )
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
	for( auto it = m_soundBankInfoMap.begin(); it != m_soundBankInfoMap.end(); ++it )
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

		if( soundBankStatus == SoundBankStatus::LOADED )
		{
			for( auto it = mapIterator->second.waitingEventsAfterLoad.begin(); it != mapIterator->second.waitingEventsAfterLoad.end(); ++it )
			{
				if( it->first != nullptr )
				{
					it->first->PostEvent( it->second, true );
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
	if( g_audioEnabled == false )
	{
		return;
	}

	g_shuttingDown = true;

	auto objects = m_soundPrioritization->GetPrioritizedAudioObjects();
	for( auto obj : objects )
	{
		obj->Cull();
	}

	ClearBanks();
#ifndef AK_OPTIMIZED
	AK::SoundEngine::UnregisterResourceMonitorCallback(ResourceMonitorCallback);
#endif

	Terminate();
	g_audioEnabled = false;
	g_shuttingDown = false;
	BeOS->UnregisterForTicks( this, (void*)"Audio::Tick" );
}


//-----------------------------------------------------
// Description:
//   Disable spatial audio, whether or not the user has a spatial audio endpoint active.
//   In order for this to work properly the "m_stereoAudioDeviceName" property in AudSettings.h
//   needs to point to an existing Wwise Audio Device made through the Wwise authoring tool that has
//   "Allow 3D Audio" disabled and an updated set of SoundBanks with this audio device needs to be loaded.
// Returns:
//   true if the request to change the audio device was successfully queued, false if the stereo audio device
//   defined in AudSettings does not exist in the loaded SoundBanks or if another unknown error occured.
//   Keep an eye on loglite to know what the problem was if this returns false.
//-----------------------------------------------------
bool AudManager::DisableSpatialAudio()
{
	const AkOutputDeviceID defaultOutputDeviceId = 0;

	if( !g_audioInitialized )
	{
		return false;
	}

	if( !m_spatialAudioEnabled )
	{
		return true;
	}

	AkOutputSettings settings = AkOutputSettings();
	settings.audioDeviceShareset = AK::SoundEngine::GetIDFromString( m_settings->m_stereoAudioDeviceName.c_str() );
	AKRESULT result = AK::SoundEngine::ReplaceOutput( settings, defaultOutputDeviceId );

	if( result != AK_Success )
	{
		if( result == AK_IDNotFound )
		{
			CCP_LOGERR( "Spatial audio was not able to be disabled because the Wwise Audio device named %S could not be found. "
						"Either create a new 2D audio device with the expected name or change the name of the 2D audio device in "
						"Carbon Audio's settings before initializing it.",
						m_settings->m_stereoAudioDeviceName.c_str() );
		}
		else
		{
			CCP_LOGERR( "Setting audio output to stereo failed to be scheduled for an unknown reason. Spatial audio will stay enabled." );
		}
		return false;
	}

	m_spatialAudioEnabled = false;
	return true;
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
void AudManager::Enable( BankVector soundBanksToLoad )
{
	if( g_audioEnabled )
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

	auto objects = m_soundPrioritization->GetPrioritizedAudioObjects();
	for( auto obj : objects )
	{
		obj->Wake();
	}

	BeOS->RegisterForTicks( this, (void*)"Audio::Tick" );
	return;
}

//-----------------------------------------------------
// Description:
//   Enable spatial audio features in Carbon Audio. This will not take effect unless the user has a spatial audio
//   endpoint active on their operating system that supports spatial audio (e.g. Dolby Atmos or Windows Sonic for Headphones).
//   In order for this to work properly the "m_spatialAudioDeviceName" property in AudSettings.h
//   needs to point to an existing Wwise Audio Device made through the Wwise authoring tool that has
//   "Allow 3D Audio" enabled and an updated set of SoundBanks with this audio device needs to be loaded.
// Returns:
//   true if the request to change the audio device was successfully queued, false if the spatial audio device
//   defined in AudSettings does not exist in the loaded SoundBanks or if another unknown error occured.
//   Keep an eye on loglite to know what the problem was if this returns false.
//-----------------------------------------------------
bool AudManager::EnableSpatialAudio()
{
	if( !g_audioInitialized )
	{
		return false;
	}

	if( m_spatialAudioEnabled )
	{
		return true;
	}

	const AkOutputDeviceID defaultOutputDeviceId = 0;
	AkOutputSettings settings = AkOutputSettings();
	settings.audioDeviceShareset = AK::SoundEngine::GetIDFromString( m_settings->m_spatialAudioDeviceName.c_str() );
	AKRESULT result = AK::SoundEngine::ReplaceOutput( settings, defaultOutputDeviceId );

	if( result != AK_Success )
	{
		if( result == AK_IDNotFound )
		{
			CCP_LOGERR( "Spatial audio was not able to be enabled because the Wwise Audio device named %S could not be found. "
						"Either create a new 3D audio device with the expected name or change the name of the 3D audio device in "
						"Carbon Audio's settings before initializing it.",
						m_settings->m_spatialAudioDeviceName.c_str() );
		}
		else
		{
			CCP_LOGERR( "Setting audio output to 3D failed to be scheduled. Audio output will continue to output in stereo." );
		}
		return false;
	}

	m_spatialAudioEnabled = true;
	return true;
}

const std::vector<std::wstring> AudManager::GetLoadedSoundBanks()
{
	BankVector loadedSoundBanks;
	CcpAutoMutex mutex( m_soundBankMutex );
	for( auto it = m_soundBankInfoMap.begin(); it != m_soundBankInfoMap.end(); ++it )
	{
		// Because of the asynchronous nature of loading SoundBanks, sometimes the state of loaded SoundBanks is erroneously reported
		// back to Python because it is not "Loaded". If it is "Loading" then it can be considered loaded as far as Python is concerned.
		if( it->second.soundBankStatus == SoundBankStatus::LOADED || it->second.soundBankStatus == SoundBankStatus::LOADING )
		{
			loadedSoundBanks.push_back( it->second.soundBankName );
		}
	}

	return loadedSoundBanks;
}

const MonitoredParameterInfo* AudManager::GetParameterInfo( const std::wstring& audioParameterName )
{
	CcpAutoMutex lock( m_moniteredParametersMapMutex );
	auto it = m_monitoredParametersMap.find( audioParameterName );
	if( it != m_monitoredParametersMap.end() )
	{
		return &it->second;
	}
	return nullptr;
}

//-----------------------------------------------------
// Description:
//   Retrieve the given emitter if it is currently alive.
// Arguments:
//   emitterID - The ID of the emitter you want to retrieve.
//-----------------------------------------------------
AudGameObjResource* AudManager::GetAudioEmitter( AkGameObjectID emitterID )
{
	auto objects = m_soundPrioritization->GetPrioritizedAudioObjects();
	for( auto obj : objects )
	{
		if( obj->GetID() == emitterID )
			return static_cast<AudGameObjResource*>( obj );
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
const std::wstring AudManager::GetEventName( AkGameObjectID emitterID, AkPlayingID playingID )
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
		auto objects = m_soundPrioritization->GetPrioritizedAudioObjects();
		for( auto obj : objects )
		{
			static_cast<AudGameObjResource*>( obj )->StopAll();
		}
	}
}

void AudManager::RegisterParameter( const std::wstring& audioParameterName )
{
	if( g_audioInitialized )
	{
		CcpAutoMutex lock( m_moniteredParametersMapMutex );
		m_monitoredParametersMap[audioParameterName].watchers++;
	}
}

void AudManager::UnregisterParameter( const std::wstring& audioParameterName )
{
	if( g_audioInitialized )
	{
		CcpAutoMutex lock( m_moniteredParametersMapMutex );
		auto it = m_monitoredParametersMap.find( audioParameterName );
		if( it != m_monitoredParametersMap.end() )
		{
			it->second.watchers -= 1;
			if( it->second.watchers == 0 )
			{
				m_monitoredParametersMap.erase( it );
			}
		}
	}
}

//-----------------------------------------------------
// Description:
//   Register a callback to be called every time Wwises audio device changes. This callback can be used to determine
//   if the user's current audio device supports spatial audio every time the audio device is changed, either by the user
//   or manually by Carbon Audio when using the EnableSpatialAudio and DisableSpatialAudio methods. Look at the documentation
//   for this method in AudManager_Blue.cpp for an example of how the Python function should look.
// Arguments:
//   callback - A Python callback that will be called every time the user has an audio device change.
//-----------------------------------------------------
void AudManager::RegisterAudioDeviceChangeCallback( const BlueScriptCallback callback )
{
	CcpAutoMutex mutex( s_audioDeviceMutex );
	s_audioDeviceChangeCallback = callback;
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
			CCP_LOG( "Event %S registered to be sent to audio emitter %d after SoundBank %S is done loading.", eventName.c_str(), emitter->GetID(), soundBankName.c_str() );
		}
	}
}

void AudManager::UpdateMonitoredParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CcpAutoMutex lock( m_moniteredParametersMapMutex );
	for( auto it = m_monitoredParametersMap.begin(); it != m_monitoredParametersMap.end(); ++it )
	{
		AK::SoundEngine::Query::RTPCValue_type rtpcValueType = AK::SoundEngine::Query::RTPCValue_type::RTPCValue_Global;
		float audioParameterValue = 0.0f;
		AKRESULT result = AK::SoundEngine::Query::GetRTPCValue( it->first.c_str(), AK_INVALID_GAME_OBJECT, AK_INVALID_PLAYING_ID, audioParameterValue, rtpcValueType );
		bool audioParameterExists = ( result == AK_Success ) ? true : false;

		it->second.parameterValue = audioParameterValue;
		it->second.parameterExists = audioParameterExists;
	}
}

void AudManager::DisableAudioCulling()
{
	auto objects = m_soundPrioritization->GetPrioritizedAudioObjects();
	for( auto obj : objects )
	{
		if( obj->IsCulled() )
		{
			obj->Wake();
		}
	}
	m_soundPrioritization->DisableAudioCulling();
	m_audioCullingEnabled = false;
	CCP_LOG_CH( s_ch, "The sound prioritization system has been disabled." );
}

void AudManager::EnableAudioCulling()
{
	m_soundPrioritization->EnableAudioCulling();
	m_audioCullingEnabled = true;
	CCP_LOG_CH( s_ch, "The sound prioritization system has been enabled." );
}

void AudManager::ResetCullingSettings()
{
	m_soundPrioritization->ResetCullingSettings();
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
	m_audioCullingEnabled = m_soundPrioritization->GetAudioCullingEnabled();
	return m_audioCullingEnabled;
}

bool AudManager::GetAudioCullingEnabledProperty() const
{
	return GetAudioCullingEnabled();
}

AudListenerPtr AudManager::GetListener()
{
	AudGameObjResourcePtr listenerGameObj = GetAudioEmitter( LISTENER_GAME_OBJ_ID );
	AudListenerPtr listener = dynamic_cast<AudListener*>( listenerGameObj.p );
	return listener;
}

std::vector<AudGameObjResource*> AudManager::GetPrioritizedAudioEmitters()
{
	std::vector<AudGameObjResource*> result;
	auto objects = m_soundPrioritization->GetPrioritizedAudioObjects();
	result.reserve( objects.size() );
	for( auto obj : objects )
	{
		result.push_back( static_cast<AudGameObjResource*>( obj ) );
	}
	return result;
}
// Callback from Wwise to use for tracking performance of the sound engine. This is called when a timer stops. Only applicable in Profile or Debug Wwise flavors.
void AudManager::AkPlatformProfilerPopTimer()
{
	// TracyLeaveZone( g_audioManager );
}

// Callback from Wwise to use for tracking performance of the sound engine. This is called when special Wwise events happen like voice starvation.
void AudManager::AkPlatformProfilerPostmarker( AkPluginID in_uPluginID, const char* in_pszMarkerName )
{
	if( in_uPluginID != AKMAKECLASSID( AkPluginTypeNone, AKCOMPANYID_AUDIOKINETIC, AK::ProfilingID::AudioFrameBoundary ) )
	{
                CCP_STATS_ZONE(in_pszMarkerName);
	}
}

// Callback from Wwise to use for tracking performance of the sound engine. This is called when a timer starts. Only applicable in Profile or Debug Wwise flavors.
void AudManager::AkPlatformProfilerPushTimer( AkPluginID in_uPluginID, const char* in_pszZoneName )
{
        // TracyEnterZone( g_audioManager, in_pszZoneName, __FILE__, __LINE__);
}

//-----------------------------------------------------
// Description:
//	 A callback that is registered during initialization to be called every Wwise tick after it is done rendering
//   audio. This particular function takes care of updating monitored audio parameters because any AK::SoundEngine::Query
//   functions from the Wwise SDK should only be called after rendering is done to minmize CPU spikes. This info
//   is taken from https://www.audiokinetic.com/en/library/edge/?source=SDK&id=goingfurther_eventmgrthread.html.
// Arguments
//   in_pCookie - A pointer to this AudManager instance.
//-----------------------------------------------------
void AudManager::GlobalCallbackEndRender( AK::IAkGlobalPluginContext* in_pContext, AkGlobalCallbackLocation in_eLocation, void* in_pCookie )
{
	AudManager* audManager = static_cast<AudManager*>( in_pCookie );
	audManager->UpdateMonitoredParameters();
}

//-----------------------------------------------------
// Description:
//	 A callback that is registered during initialization that is triggered every Wwise's audio device is changed. This method takes care of
//   calling the callback that can be registered with the RegisterAudioDeviceCallback method which can be used to keep track of if the
//   user's current audio output device supports spatial audio or not. The kinds of events that will trigger this callback method are:
//     * Initializion of carbon audio, e.g. the first time an audio output device is created.
//     * Swapping audio device sharesets during runtime (e.g. DisableSpatialAudio and EnableSpatialAudio).
//     * When the user changes their audio device at an operating system level (e.g. changing from headphones to speakers).
//-----------------------------------------------------
void AudManager::AudioDeviceStatusChangeCallback( AK::IAkGlobalPluginContext* in_pContext, AkUniqueID in_idAudioDeviceShareset, AkUInt32 in_idDeviceID, AK::AkAudioDeviceEvent in_idEvent, AKRESULT in_AkResult )
{
	size_t currentErrorHash = std::hash<AkUniqueID>()( in_idDeviceID ) ^ std::hash<AkUniqueID>()( in_idAudioDeviceShareset ) ^ std::hash<AKRESULT>()( in_AkResult ) ^ std::hash<AK::AkAudioDeviceEvent>()( in_idEvent );

	if( currentErrorHash == s_lastLoggedDeviceHash.load() )
	{
		return;
	}

	if( in_idEvent == AK::AkAudioDeviceEvent::AkAudioDeviceEvent_Initialization )
	{
		if( in_AkResult == AK_Success )
		{
			CCP_LOG( "Audio device %u with shareset %u was successfully initialized.", in_idDeviceID, in_idAudioDeviceShareset );

			AKRESULT result = AK::SoundEngine::GetDeviceSpatialAudioSupport( in_idDeviceID );
			CcpAutoMutex mutex( s_audioDeviceMutex );
			if( result == AK_Success )
			{
				CCP_LOG( "The user's system supports spatial audio (audio device %u).", in_idDeviceID );
				s_systemSupportsSpatialAudio = true;
			}
			else if( result == AK_NotCompatible )
			{
				CCP_LOG( "The user's system does not support spatial audio (audio device %u).", in_idDeviceID );
				s_systemSupportsSpatialAudio = false;
			}
			else
			{
				CCP_LOGERR( "Checking for spatial audio support with audio device id %u failed for an unknown reason.", in_idDeviceID );
				s_systemSupportsSpatialAudio = false;
			}
		}
		else
		{
			CCP_LOGERR( "Audio device %u with share set %u was unable to be initialized.", in_idDeviceID, in_idAudioDeviceShareset );
		}
	}
	else
	{
		if( in_idEvent == AK::AkAudioDeviceEvent::AkAudioDeviceEvent_Removal )
		{
			CCP_LOG( "Audio device %u with shareset %u was manually requested to be removed.", in_idDeviceID, in_idAudioDeviceShareset );
		}
		else if( in_idEvent == AK::AkAudioDeviceEvent::AkAudioDeviceEvent_SystemRemoval )
		{
			CCP_LOG( "Audio device %u with shareset %u was removed because of a change in the user's system output.", in_idDeviceID, in_idAudioDeviceShareset );
		}
		else
		{
			CCP_LOGERR( "An unkown Audio Device Event %d was received in the AudioDeviceStatusChangeCallback.", in_idEvent );
		}
	}

	s_lastLoggedDeviceHash.store( currentErrorHash );

	// This exists solely to be able to propagate the audio device change callback on the main thread.
	// It is important to propagate it on the main thread because it is theoretically possible that Carbon Audio
	// would hold up Wwise's audio thread as it waits for Python. This circumvents that and will make sure this is processed on the next tick.
	g_mainThreadQueue->Add( []( void* ) {
		if( s_audioDeviceChangeCallback )
		{
			s_audioDeviceChangeCallback.CallVoid( s_systemSupportsSpatialAudio );
		}
	},
							nullptr,
							IBlueCallbackMan::BCBF_NONE,
							nullptr );
}

AKRESULT AudManager::StartProfilerCapture()
{
	if( !g_audioEnabled )
	{
		CCP_LOGERR( "Cannot start profiler capture: Audio is not initialized." );
		return AK_Fail;
	}

	// Use current date and time as the filename
	time_t now = time( 0 );
	struct tm timeinfo;
#if defined( _WIN32 )
	localtime_s( &timeinfo, &now );
#else
	localtime_r( &now, &timeinfo );
#endif

	char filename[64];
	strftime( filename, sizeof( filename ), "WwiseCapture_%Y%m%d_%H%M%S.prof", &timeinfo );

#if defined( _WIN32 )
	AkOSChar wfilename[128];
	MultiByteToWideChar( CP_UTF8, 0, filename, -1, wfilename, 128 );
#else
	const AkOSChar* wfilename = filename;
#endif

	AKRESULT result = AK::SoundEngine::StartProfilerCapture( wfilename );
	if( result == AK_Success )
	{
		m_isProfilerCapturing = true;
		CCP_LOG_CH( s_ch, "Started Wwise profiler capture: %s", filename );
	}
	else
	{
		CCP_LOGERR( "Failed to start Wwise profiler capture: %s (Error code: %d)", filename, result );
	}

	return result;
}

AKRESULT AudManager::StopProfilerCapture()
{
	if( !g_audioEnabled )
	{
		CCP_LOGERR( "Cannot stop profiler capture: Audio is not initialized." );
		return AK_Fail;
	}

	AKRESULT result = AK::SoundEngine::StopProfilerCapture();

	if( result == AK_Success )
	{
		m_isProfilerCapturing = false;
		CCP_LOG_CH( s_ch, "Stopped Wwise profiler capture." );
	}
	else
	{
		CCP_LOGERR( "Failed to stop Wwise profiler capture.", result );
	}

	return result;
}

bool AudManager::IsProfilerCapturing() const
{
	return m_isProfilerCapturing;
}

#ifndef AK_OPTIMIZED
void AudManager::ResourceMonitorCallback( const AkResourceMonitorDataSummary* dataSummary )
{
	CCP_STATS_SET( totalCPU, dataSummary->totalCPU );
	CCP_STATS_SET( pluginCPU, dataSummary->pluginCPU );
	CCP_STATS_SET( physicalVoices, dataSummary->physicalVoices );
	CCP_STATS_SET( virtualVoices, dataSummary->virtualVoices );
	CCP_STATS_SET( totalVoices, dataSummary->totalVoices );
	CCP_STATS_SET( nbActiveEvents, dataSummary->nbActiveEvents );
}
#endif