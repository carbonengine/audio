////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jan 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <ITr2AudGeometry.h>

/**
 * @brief The object trinity creates to register meshes as Wwise Spatial Audio geometry.
 *
 * A geometry set is a mesh, shared by every instance that places it in the world.
 * Holds no state, AudGeometryManager keeps the geometry and sends it to Wwise.
 */
BLUE_CLASS( AudGeometry ) :
	public ITr2AudGeometry
{
public:
	AudGeometry( IRoot* lockobj = NULL );
	virtual ~AudGeometry();

	EXPOSE_TO_BLUE();

	void SetGeometry(
		uint64_t geometrySetId,
		uint64_t instanceId,
		const Tr2AudGeometryData& geometryData,
		const Matrix& worldTransform ) override;

	void SetGeometryTransform(
		uint64_t geometrySetId,
		uint64_t instanceId,
		const Matrix& worldTransform ) override;

	void RemoveGeometry(
		uint64_t geometrySetId,
		uint64_t instanceId ) override;
};

TYPEDEF_BLUECLASS( AudGeometry );
