#include <stdafx.h>
#include "AudUIPlayer.h"
#include "blue\Include\IBlueResMan.h"

// Wwise includes
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "AudManager.h"

static void EventFinishedCallback(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo);
static void PropagateCallback( void* callbackCookie );

struct CallbackCookie {
	AudUIPlayer* uiPlayer;
	std::wstring eventName;
};

AudUIPlayer::AudUIPlayer( IRoot* lockobj )
{
	m_ID = UI_GAME_OBJ_ID;
	m_name = "UI";
	CreateWwiseObject();
}

AudUIPlayer::~AudUIPlayer()
{
}

unsigned int AudUIPlayer::SendEvent( const std::wstring& name )
{
	if( g_audioEnabled )
	{
		m_playEvent = name;
		m_playID = AK::SoundEngine::PostEvent( name.c_str(), m_ID );
		g_audioManager->LogPostEvent( m_ID, m_playID, AK_INVALID_UNIQUE_ID, name );
		return m_playID;
	}
	return 0;
}

unsigned int AudUIPlayer::SendEventWithCallback( const std:: wstring& name )
{
	if( g_audioEnabled )
	{
		m_callbackEventName = name;
		if(m_callback)
		{
			CallbackCookie* cookie = new CallbackCookie;
			cookie->uiPlayer = this;
			cookie->eventName = m_callbackEventName;
			m_playID = AK::SoundEngine::PostEvent( name.c_str(), m_ID, AK_EndOfEvent, EventFinishedCallback, cookie);
			g_audioManager->LogPostEvent( m_ID, m_playID, AK_INVALID_UNIQUE_ID, name );
		}
		else 
		{
			CCP_LOGWARN("SendEventWithCallback called without any callback function set");
			return 0;
		}
		return m_playID;
	}
	return 0;
}

void AudUIPlayer::Callback(std::wstring eventName)
{
	if(m_callback)
	{
		m_callback.CallVoid( eventName );
	}
}

static void PropagateCallback( void* callbackCookie )
{
	// This will run on the main python thread if called by the mainTreadQueue
	CallbackCookie* callback = reinterpret_cast<CallbackCookie*>(callbackCookie);
	AudUIPlayer* audplayer = reinterpret_cast<AudUIPlayer*>(callback->uiPlayer);
	audplayer->Callback( callback->eventName );
	delete callbackCookie;
}

static void EventFinishedCallback(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo)
{
	//This callback comes on the main audiothread which is not the main Python thread.
	//This add will add the callback to the gmainThreadQueue which will be process on the next tick.
	g_mainThreadQueue->Add( PropagateCallback, in_pCallbackInfo->pCookie, IBlueCallbackMan::BCBF_NONE, nullptr );	
}