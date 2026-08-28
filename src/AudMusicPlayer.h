// Copyright © 2022 CCP ehf.

#pragma once

#include "AudEmitter.h"

BLUE_CLASS( AudMusicPlayer ) : public AudEmitter 
{
public:
	AudMusicPlayer( IRoot* lockobj = NULL );
	~AudMusicPlayer();

	// The music player is never spatialized and should not be considered for any calculations that depend on world position.
	bool HasUsableWorldPosition() const override;

	EXPOSE_TO_BLUE();
};

TYPEDEF_BLUECLASS( AudMusicPlayer );