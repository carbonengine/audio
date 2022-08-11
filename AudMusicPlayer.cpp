#include <stdafx.h>
#include "AudMusicPlayer.h"

AudMusicPlayer::AudMusicPlayer( IRoot* lockobj ) : AudEmitter( MUSIC_GAME_OBJ_ID, lockobj )
{
	m_name = "Music";
	m_additionalCullingWeight = std::numeric_limits<float>::max();
	RegisterWwiseObject();
}

AudMusicPlayer::~AudMusicPlayer()
{
}