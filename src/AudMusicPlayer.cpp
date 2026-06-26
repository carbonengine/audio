// Copyright © 2022 CCP ehf.

#include <stdafx.h>
#include "AudMusicPlayer.h"

AudMusicPlayer::AudMusicPlayer( IRoot* lockobj ) : AudEmitter( MUSIC_GAME_OBJ_ID, lockobj )
{
	m_name = "Music";
	m_additionalCullingWeight = std::numeric_limits<float>::max();
	SetPosition( Vector3( 1, 0, 0 ), Vector3( 0, 1, 0 ), Vector3( 0, 0, 0 ) );
}

AudMusicPlayer::~AudMusicPlayer()
{
}