#include "stdafx.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>				// Sound Engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>				// Interactive music engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>					// Memory Manager
#include <AK/SoundEngine/Common/IAkStreamMgr.h>					// Streaming Manager
#include <AK/SoundEngine/Common/AkModule.h>						// Default memory and stream managers
#include <AK/SoundEngine/Common/AkQueryParameters.h>

#include <AK/Plugin/AkVorbisDecoderFactory.h>
#include <AK/Plugin/AkCompressorFXFactory.h>
#include <AK/Plugin/AkSilenceSourceFactory.h>
#include <AK/Plugin/AkParametricEQFXFactory.h>
#include <AK/Plugin/AkRoomVerbFXFactory.h>
#include <AK/Plugin/AkMatrixReverbFXFactory.h>
#include "CCPAudioStream/include/CCPAudioStreamSourceFactory.h"
#include "CCPAudioStream/AudioEngineFX/CCPFXSrcAudioStreamSourceFactory.h"
#include <AK/Plugin/AkMeterFXFactory.h>
#include <AK/Plugin/AkPeakLimiterFXFactory.h>
#include <AK/Plugin/AkFlangerFXFactory.h>
#include <AK/Plugin/AkGuitarDistortionFXFactory.h>
#include <AK/Plugin/AkHarmonizerFXFactory.h>

#include "AudManager.h"
#include "AudSettingsRegistrar.h"
#include "AudAlloc.h"
#include "AudResource.h"
#include "AudSettings.h"
#include "AudConfig.h"
#include "AudLowLevelIO.h"
#include "AudEmitterMulti.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "WwiseAssert" );
static GameObjIDVector s_gameObjectsToBeDestroyed;

static void WwiseAssertHook(const char* in_pszExpression,const char* in_pszFileName,int in_lineNumber)
{
	CCP_LOGWARN_CH( s_ch, "Assert expression failed: %s in file %s at line %d", in_pszExpression, in_pszFileName, in_lineNumber);
}

AudManager::AudManager( IRoot* lockobj ) :
	m_tickInterval( 10 ),
	m_waitingEventsMutex( "AudManager", "m_waitingEventsMutex" ),
	m_multiEmitterMutex( "AudManager", "m_multiEmitterMutex"),
	m_useDoppler( false )
{
}

AudManager::~AudManager()
{
	if( g_audioInitialized )
	{
		Terminate();
	}
}

void AudManager::Process()
{
	if( g_audioInitialized )
	{
		ProcessMultiEmitterList();

		// Process bank requests, events, positions, RTPC, etc.
		AK::SoundEngine::RenderAudio();

		//Update main thread queue
		g_mainThreadQueue->Update();

		// Executing gameobjects on death row....culling it you might say!
		// Resist the urge to cache the end pointer here - since we are erasing from the list, end can change and must
		// therefore be queried on every iteration of the loop! <halldor>
		for (GameObjIDVector::iterator it = s_gameObjectsToBeDestroyed.begin(); it != s_gameObjectsToBeDestroyed.end(); )
		{
			AKRESULT result = AK::SoundEngine::UnregisterGameObj(*it);
			if (result == AK_Success)
			{
				it = s_gameObjectsToBeDestroyed.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}

bool AudManager::Init()
{
	if( !InitLowLevel() )
	{
		return false;
	}

	if( !InitCommunication() )
	{
		return false;
	}

	if( !InitSound() )
	{
		return false;
	}

	if( !InitMusic() )
	{
		return false;
	}
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
	if ( AK::SoundEngine::IsInitialized() )
	{	
		// Terminate the sound engine
		AK::SoundEngine::Term();
	}
	
	// Terminate the streaming manager
    if ( AK::IAkStreamMgr::Get() )
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
	
	BeOS->NextScheduledEvent(m_tickInterval);
}

bool AudManager::InitLowLevel()
{
//-----------------------------------------------------------------------------
    // Create and initialize an instance of the default memory manager. Note
	// that you can override the default memory manager with your own. Refer
	// to the Wwise SDK documentation for more information.
//-----------------------------------------------------------------------------

	if ( AK::MemoryMgr::Init( &m_initConfig->m_memSettings ) != AK_Success )
    {
        return false;
    }

//-----------------------------------------------------------------------------
    // Create and initialize an instance of the default streaming manager. Note
	// that you can override the default streaming manager with your own. Refer
	// to the SDK documentation for more information.
//-----------------------------------------------------------------------------

    // Streaming settings
    // Create and initialize an instance of our stream manager.
	AK::IAkStreamMgr * pStreamMgr = AK::StreamMgr::Create( m_initConfig->m_streamMgrSettings );
    if ( ! pStreamMgr )
    {
        return false;
    }

//-----------------------------------------------------------------------------
    // Create default IO device.
//-----------------------------------------------------------------------------
	// Device settings
	AudLowLevelIOPtr tmp = reinterpret_cast<AudLowLevelIO*>(m_initConfig->m_lowLevelIO.p);
	if ( tmp->Init( m_initConfig->m_deviceSettings, m_initConfig->m_asyncFileOpen ) != AK_Success )
    {
        return false;
    }

	return true;
}

bool AudManager::InitCommunication()
{
	#ifndef AK_OPTIMIZED
	AK::Comm::GetDefaultInitSettings( m_commSettings );
	if( AK::Comm::Init( m_commSettings ) != AK_Success )
	{
		assert( ! "Audio2: Could not init communication lib" );
		return false;
	}
	#endif
	
	return true;
}

bool AudManager::InitSound()
{
	//Change the assert hook
	m_initConfig->m_initSettings.pfnAssertHook = &WwiseAssertHook;

	if ( AK::SoundEngine::Init( &m_initConfig->m_initSettings, &m_initConfig->m_platformInitSettings ) != AK_Success )
    {
        return false;
    }

	return true;
}

bool AudManager::InitMusic()
{
	if ( AK::MusicEngine::Init( &m_initConfig->m_musicSettings ) != AK_Success )
	{
		return false;
	}

	return true;
}

void AudManager::SetEnabled( bool newStatus )
{
	if( newStatus == g_audioEnabled )
	{
		return;
	}

	g_audioEnabled = newStatus;

	if( g_audioEnabled )
	{
		// Initialize WWISE
		if( !Init() )
		{
			CCP_LOGERR( "Failed to initialize audio" );
			g_audioEnabled = false;
			return;
		}

		g_audioInitialized = true;
		//Reload resources
		BankVector::iterator bankEnd = m_loadedBanks.end();
		AkBankID tmp;
		for( BankVector::iterator it = m_loadedBanks.begin(); it != bankEnd; ++it )
		{
			AK::SoundEngine::LoadBank( it->c_str(), AK_DEFAULT_POOL_ID, tmp );
		}
		AudResource::RecreateResources();

		BeOS->RegisterForTicks(this, (void*)"Audio::Tick");

		m_initConfig->m_dirty = false;
	}
	else
	{
		CcpAutoMutex guard( m_waitingEventsMutex );
		m_waitingEvents.clear();

		//Unload all resources
		AK::SoundEngine::StopAll();
		AK::SoundEngine::UnregisterAllGameObj();
		AK::SoundEngine::ClearBanks();
		//Tear down WWISE
		//Make this more flexible
		Terminate();
		g_audioInitialized = false;
		BeOS->UnregisterForTicks( this, (void*)"Audio::Tick" );
	}
}

struct BankLoadUnloadStatus
{
	BankLoadUnloadStatus() : isDone( 0 ), result( AK_Fail ) {}
	bool isDone;
	AKRESULT result;
};

namespace
{
	void BankLoadUnloadCb( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie )
	{
		BankLoadUnloadStatus* status = reinterpret_cast<BankLoadUnloadStatus*>( in_pCookie );
		status->isDone = true;
		status->result = in_eLoadResult;
	}

	void WaitForLoadUnload( BankLoadUnloadStatus* status )
	{
		while( !status->isDone )
		{
			if( PyOS->CanYield() )
			{
				if( !PyOS->Yield() )
				{
					// Tasklet killed
					break;
				}
			}
			else
			{
				CcpThreadSleep( 10 );
			}
		}
	}
}

void AudManager::LoadBank( const std::wstring& name )
{
	{
		// Ensure iterators go out of scope before we call WaitForLoadUnload below
		// as it may yield and that can cause issues with iterator validation in
		// debug builds.
		BankVector::iterator end = m_loadedBanks.end();
		BankVector::iterator result = std::find( m_loadedBanks.begin(), end, name );
		if( result != end )
		{
			return;
			}

		m_loadedBanks.push_back( name );
	}

	if( g_audioEnabled )
	{
		AkBankID tmp;
		BankLoadUnloadStatus* status = CCP_NEW( "LoadBank/status" ) BankLoadUnloadStatus;
		AKRESULT result = AK::SoundEngine::LoadBank( name.c_str(), BankLoadUnloadCb, status, AK_DEFAULT_POOL_ID, tmp );
		if( result == AK_Fail )
		{
			CCP_LOGERR( "AK::SoundEngine::LoadBank failed for %S", name.c_str() );
				return;
			}

		CCP_LOG( "AK::SoundEngine::LoadBank scheduled for %S", name.c_str() );
		
		WaitForLoadUnload( status );

		ProcessWaitingEvents( );
		
			CCP_DELETE status;
		CCP_LOG( "AK::SoundEngine::LoadBank done for %S", name.c_str() );
	}
}

void AudManager::UnloadBank( const std::wstring& name )
{
	{
		// Ensure iterators go out of scope before we call WaitForLoadUnload below
		// as it may yield and that can cause issues with iterator validation in
		// debug builds.

		BankVector::iterator end = m_loadedBanks.end();
		BankVector::iterator result = std::find( m_loadedBanks.begin(), end, name );
		if( result == end )
		{
			return;
		}

		m_loadedBanks.erase( result );
	}

	if( g_audioEnabled )
	{
		// The pInMemoryBankPtr can be NULL is NULL is passed when loading the bank.
		const void *pInMemoryBankPtr = NULL;
		AkMemPoolId out_pool_id = NULL;

		BankLoadUnloadStatus* status = CCP_NEW( "LoadBank/status" ) BankLoadUnloadStatus;
		AKRESULT result = AK::SoundEngine::UnloadBank( name.c_str(), pInMemoryBankPtr, BankLoadUnloadCb, status );
		if( result == AK_Fail )
		{
			CCP_LOGERR( "AK::SoundEngine::UnloadBank failed for %S", name.c_str() );
			return;
		}

		CCP_LOG( "AK::SoundEngine::UnloadBank scheduled for %S", name.c_str() );

		WaitForLoadUnload( status );

		CCP_DELETE status;
		CCP_LOG( "AK::SoundEngine::UnloadBank done for %S", name.c_str() );
	}
}

void AudManager::AddToDestructionVector(AkGameObjectID gameObjID)
{
	s_gameObjectsToBeDestroyed.push_back( gameObjID );
}


AudSettings& AudManager::GetSettings()
{
	static CAudSettings s; // C-object & singleton, no reference counting magic!
	return s;
}

std::vector<std::wstring> AudManager::GetLoadedSoundBanks()
{
	return m_loadedBanks;
}

void AudManager::AddWaitingEvent( AkUniqueID eventID, AkGameObjectID gameObjID )
{
	CcpAutoMutex guard( m_waitingEventsMutex );

	WaitingEvent failedEvent = { eventID, gameObjID, 0 };
	for (std::vector<WaitingEvent>::iterator it = m_waitingEvents.begin() ; it != m_waitingEvents.end(); ++it)
	{
		if ((it->eventID == failedEvent.eventID) && (it->gameObjectID == failedEvent.gameObjectID))
		{
			// Return early if same event-gameobjectID combo is already in the list.
			return;
		}
	}
	m_waitingEvents.push_back(failedEvent);
}

void AudManager::ProcessWaitingEvents()
{
	CcpAutoMutex guard( m_waitingEventsMutex );

	for (std::vector<WaitingEvent>::iterator it = m_waitingEvents.begin() ; it != m_waitingEvents.end();)
	{
		AkInt32 playback_ID = AK::SoundEngine::PostEvent( it->eventID, it->gameObjectID );
		it->numRetries += 1;
		if ((playback_ID != AK_INVALID_PLAYING_ID) || it->numRetries > 7)
		{
			it = m_waitingEvents.erase( it );
		}
		else 
		{
			++it;
		}
	}
}

AudEmitterMulti* AudManager::GetEmitterForEventID(AkUniqueID eventID)
{
	CcpAutoMutex guard( m_multiEmitterMutex );

	for (EmitterMultiSet::iterator it = m_multiEmitters.begin() ; it != m_multiEmitters.end(); ++it)
	{
		AudEmitterMulti* aem = *it;
		if (aem->m_eventID == eventID)
		{
			return aem;
		}
	}
	return NULL;
}

void AudManager::AddMultiEmitterToList(AudEmitterMulti* emitter)
{
	CcpAutoMutex guard( m_multiEmitterMutex );
	m_multiEmitters.insert(emitter);
}

void AudManager::RemoveMultiEmitterFromList(AudEmitterMulti* emitter)
{
	CcpAutoMutex guard( m_multiEmitterMutex );
	m_multiEmitters.erase(emitter);
}

void AudManager::ProcessMultiEmitterList()
{
	CcpAutoMutex guard( m_multiEmitterMutex );

	for (EmitterMultiSet::iterator it = m_multiEmitters.begin() ; it != m_multiEmitters.end(); ++it)
	{
		AudEmitterMulti* aem = *it;
		aem->ProcessPlacementList();
	}
}

Be::Result<std::string> AudManager::GetEmitterForEventName( const std::wstring& eventName, AudEmitterMulti** out )
{
	AkUniqueID eventID = AK::SoundEngine::GetIDFromString( eventName.c_str() );

	AudEmitterMulti* multi = g_audioManager->GetEmitterForEventID( eventID );
	if( multi )
	{
		reinterpret_cast<IRoot*>( multi )->Lock();
		*out = multi;
		return Be::Result<std::string>();
	}
	else
	{
		AudEmitterMulti* p = new OAudEmitterMulti();
		if( !p )
		{
			*out = nullptr;
			return Be::Result<std::string>( "Could not create an instance of AudEmitterMulti" );
		}
		else
		{
			p->Initialize( eventName );
			*out = p;
			return Be::Result<std::string>();
		}
	}
}