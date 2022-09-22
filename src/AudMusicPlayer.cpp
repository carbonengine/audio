#include <stdafx.h>
#include "AudMusicPlayer.h"

AudMusicPlayer::AudMusicPlayer( IRoot* lockobj ) : AudEmitter( MUSIC_GAME_OBJ_ID, lockobj )
{
	m_name = "Music";
	m_additionalCullingWeight = std::numeric_limits<float>::max();
	m_position = Vector3( 0, 0, 0 );
	RegisterWwiseObject();
}

AudMusicPlayer::~AudMusicPlayer()
{
}