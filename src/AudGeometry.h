////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jan 2026
// Copyright (c) 2026 CCP Games
//

#pragma once


#include <ITr2AudGeometry.h>
#include <ITr2AudEmitter.h>


struct Vector3;


BLUE_CLASS(AudGeometry) :
	public ITr2AudGeometry

{
public:
	AudGeometry(IRoot* lockobj = NULL);
	virtual ~AudGeometry();

	EXPOSE_TO_BLUE();


	// ITr2AudGeometry interface

	void SetGeometry(
		uint64_t geometryId,
		const Tr2AudGeometryData& geometryData,
		const Matrix& worldTransform) override;

	void SetGeometryTransform(uint64_t geometryId, const Matrix& worldTransform) override;
	void RemoveGeometry(uint64_t geometryId) override;
};

TYPEDEF_BLUECLASS( AudGeometry );
