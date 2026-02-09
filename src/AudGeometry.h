////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jan 2026
// Copyright (c) 2026 CCP Games
//

#pragma once


#include <ITr2AudGeometry.h>
#include <ITr2AudEmitter.h>

#include <unordered_map>

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
		uint64_t geometrySetId,
		uint64_t instanceId,
		const Tr2AudGeometryData& geometryData,
		const Matrix& worldTransform) override;

	void SetGeometryTransform(
		uint64_t geometrySetId,
		uint64_t instanceId,
		const Matrix& worldTransform) override;

	void RemoveGeometry(
		uint64_t geometrySetId,
		uint64_t instanceId) override;

private:
	// Tracks how many active instances reference each geometry set
	std::unordered_map<uint64_t, uint32_t> m_geometrySetRefCounts;

	CcpMutex m_mutex;
};

TYPEDEF_BLUECLASS( AudGeometry );
