////////////////////////////////////////////////////////////
//
// Creator: Eric Nielsen
// Creation Date: May 2022
// Copyright 2022 CCP Games
//

#pragma once

#include "AudEmitter.h"

BLUE_CLASS( AudMusicPlayer ) : public AudEmitter 
{
public:
	AudMusicPlayer( IRoot* lockobj = NULL );
	~AudMusicPlayer();

	EXPOSE_TO_BLUE();
};

TYPEDEF_BLUECLASS( AudMusicPlayer );