#include <stdafx.h>
#include "AudUIPlayer.h"

// Wwise includes
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

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
		return m_playID;
	}
	return 0;
}
