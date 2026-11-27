////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <ITr2AudWorld.h>

#include <cstdint>

/**
 * @brief The object trinity creates to tell audio where its scene origin is.
 *
 * Trinity creates one for each scene. AudWorldFrame keeps the origin and lets one scene at a time set it.
 */
BLUE_CLASS( AudWorld ) :
	public ITr2AudWorld
{
public:
	AudWorld( IRoot* lockobj = NULL );
	virtual ~AudWorld();

	EXPOSE_TO_BLUE();

	void SetSceneOrigin( double x, double y, double z ) override;

private:
	/// Tells this scene apart from the others for AudWorldFrame.
	const uint64_t m_source;
};

TYPEDEF_BLUECLASS( AudWorld );
