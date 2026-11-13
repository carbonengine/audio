#include "stdafx.h"
#include "AudGeometry.h"
#include "AudGeometryManager.h"

AudGeometry::AudGeometry( IRoot* lockobj )
{}

AudGeometry::~AudGeometry()
{}

void AudGeometry::SetGeometry(
	uint64_t geometrySetId,
	uint64_t instanceId,
	const Tr2AudGeometryData& geometryData,
	const Matrix& worldTransform )
{
	AudGeometryManager::Get().SetGeometry( geometrySetId, instanceId, geometryData, worldTransform );
}

void AudGeometry::SetGeometryTransform(
	uint64_t geometrySetId,
	uint64_t instanceId,
	const Matrix& worldTransform )
{
	AudGeometryManager::Get().SetGeometryTransform( instanceId, worldTransform );
}

void AudGeometry::RemoveGeometry(
	uint64_t geometrySetId,
	uint64_t instanceId )
{
	AudGeometryManager::Get().RemoveGeometry( instanceId );
}
