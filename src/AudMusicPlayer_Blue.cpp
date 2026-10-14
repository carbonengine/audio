// Copyright © 2022 CCP ehf.

#include <stdafx.h>
#include "AudMusicPlayer.h"

BLUE_DEFINE_ABSTRACT( AudMusicPlayer );

const Be::ClassInfo* AudMusicPlayer::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudMusicPlayer, "Simple wrapper for an audio emitter dedicated to playing music in the game." )
		MAP_INTERFACE( AudMusicPlayer )

	EXPOSURE_CHAINTO( AudEmitter )
}

static AudMusicPlayerPtr s_audMusicPlayer = nullptr;

AudMusicPlayer* GetOrCreateMusicPlayer()
{
	if ( s_audMusicPlayer == nullptr )
	{
		s_audMusicPlayer = new OAudMusicPlayer;
	}
	return s_audMusicPlayer.p;
}

static PyObject* PyGetMusicPlayer( PyObject* self, PyObject* args )
{
	return PyOS->WrapBlueObject( GetOrCreateMusicPlayer()->GetRawRoot() );
}
MAP_FUNCTION
( 
	"GetMusicPlayer", 
	PyGetMusicPlayer,
	"Gets an instance of AudMusicPlayer, if no instance exists one will be created."
);
