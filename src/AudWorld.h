////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <ITr2AudWorld.h>

/**
 * @brief The object trinity creates to tell audio where its scene origin is.
 *
 * Holds no state, AudWorldFrame keeps the origin.
 */
BLUE_CLASS( AudWorld ) :
	public ITr2AudWorld
{
public:
	AudWorld( IRoot* lockobj = NULL );
	virtual ~AudWorld();

	EXPOSE_TO_BLUE();

	void SetSceneOrigin( double x, double y, double z ) override;
};

TYPEDEF_BLUECLASS( AudWorld );
