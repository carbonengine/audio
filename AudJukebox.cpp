#include "stdafx.h"
#include "AudJukebox.h"
#include "AudPlaylistItem.h"

// Wwise includes
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

static void PythonEvent( void* context )
{
	PayLoad* tmp = (PayLoad*)context;
	if( tmp->eventListener && g_audioInitialized )
	{
		tmp->eventListener->HandleEvent( tmp->name.c_str() );
	}
	//So we don't leak, we know that when this gets called
	//the content of tmp, created in AudJukebox SendEvent is no longer needed.
	delete tmp;
	tmp = NULL;
}

static void EventCallback( AkCallbackType in_eType, AkCallbackInfo *in_pCallbackInfo )
{
	g_mainThreadQueue->Add( PythonEvent, in_pCallbackInfo->pCookie, 0, NULL );
}

AudJukebox::AudJukebox( IRoot* lockobj ) :
	m_baseInfo( CCPMP3BaseInfo() )
{
	m_ID = JUKEBOX_GAME_OBJ_ID;
	m_name ="Jukebox";
	CreateWwiseObject();
}

AudJukebox::~AudJukebox()
{
}

unsigned int AudJukebox::SendEvent( const std::wstring& name )
{
	if( g_audioInitialized )
	{
		PayLoad* tmp = new PayLoad( name, m_eventListener );
		m_playEvent = name;
		m_playID = AK::SoundEngine::PostEvent( name.c_str(), m_ID, AK_MusicSyncExit, &EventCallback, (void*)tmp);
		return m_playID;
	}
	return 0;
}

void AudJukebox::Play( const std::wstring& absPath )
{
    if( g_audioInitialized )
	{
        PayLoad* tmp = new PayLoad( absPath.c_str(), m_eventListener );
		bool ret = CCPMP3InfoRead( const_cast<wchar_t*>(absPath.c_str()), m_baseInfo );

        // Small optimization: Do not even go through WWise and all the queues in case of playback failing.
        if ( ret )
        {
		    CCPMP3PrepareForPlayback( const_cast<wchar_t*>(absPath.c_str()), m_baseInfo );
		    AK::SoundEngine::PostEvent( L"jukebox_play", m_ID, AK_EndOfEvent, &EventCallback, (void*)tmp );
        }
        else
        {
            AkCallbackInfo info;
            info.gameObjID = m_ID;
            info.pCookie = (void*)tmp;
            EventCallback( AK_EndOfEvent, &info );
        }
	}
}
