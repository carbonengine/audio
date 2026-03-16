#include "stdafx.h"
#include "SpatialAudioSettings.h"
#include <AK/SpatialAudio/Common/AkSpatialAudio.h>

SpatialAudioSettings::SpatialAudioSettings()
	: m_spatialAudioGeometryEnabled( false )
	, m_movementThreshold( 100.0f )
	, m_numberOfPrimaryRays( 35 )
	, m_maxReflectionOrder( 0 )
	, m_maxDiffractionOrder( 4 )
	, m_maxEmitterRoomAuxSends( 0 )
	, m_diffractionOnReflectionsOrder( 0 )
	, m_maxPathLength( 1000.0f )
	, m_cpuLimitPercentage( 20.0f )
	, m_loadBalancingSpread( 1 )
	, m_enableDiffractionAndTransmission( true )
	, m_calcEmitterVirtualPosition( true )
	, m_transmissionLoss( 0.7f )
	, m_enableDiffraction( true )
	, m_enableDiffractionOnBoundaryEdges( true )
{
}

bool SpatialAudioSettings::GetSpatialAudioGeometryEnabled() const { return m_spatialAudioGeometryEnabled; }
void SpatialAudioSettings::SetSpatialAudioGeometryEnabled( bool value ) { m_spatialAudioGeometryEnabled = value; }

float SpatialAudioSettings::GetMovementThreshold() const { return m_movementThreshold; }
void SpatialAudioSettings::SetMovementThreshold( float value ) { m_movementThreshold = value; }

int SpatialAudioSettings::GetNumberOfPrimaryRays() const { return m_numberOfPrimaryRays; }
void SpatialAudioSettings::SetNumberOfPrimaryRays( int value ) { m_numberOfPrimaryRays = value; }

int SpatialAudioSettings::GetMaxReflectionOrder() const { return m_maxReflectionOrder; }
void SpatialAudioSettings::SetMaxReflectionOrder( int value ) { m_maxReflectionOrder = value; }

int SpatialAudioSettings::GetMaxDiffractionOrder() const { return m_maxDiffractionOrder; }
void SpatialAudioSettings::SetMaxDiffractionOrder( int value ) { m_maxDiffractionOrder = value; }

int SpatialAudioSettings::GetMaxEmitterRoomAuxSends() const { return m_maxEmitterRoomAuxSends; }
void SpatialAudioSettings::SetMaxEmitterRoomAuxSends( int value ) { m_maxEmitterRoomAuxSends = value; }

int SpatialAudioSettings::GetDiffractionOnReflectionsOrder() const { return m_diffractionOnReflectionsOrder; }
void SpatialAudioSettings::SetDiffractionOnReflectionsOrder( int value ) { m_diffractionOnReflectionsOrder = value; }

float SpatialAudioSettings::GetMaxPathLength() const { return m_maxPathLength; }
void SpatialAudioSettings::SetMaxPathLength( float value ) { m_maxPathLength = value; }

float SpatialAudioSettings::GetCPULimitPercentage() const { return m_cpuLimitPercentage; }
void SpatialAudioSettings::SetCPULimitPercentage( float value ) { m_cpuLimitPercentage = value; }

int SpatialAudioSettings::GetLoadBalancingSpread() const { return m_loadBalancingSpread; }
void SpatialAudioSettings::SetLoadBalancingSpread( int value ) { m_loadBalancingSpread = value; }

bool SpatialAudioSettings::GetEnableDiffractionAndTransmission() const { return m_enableDiffractionAndTransmission; }
void SpatialAudioSettings::SetEnableDiffractionAndTransmission( bool value ) { m_enableDiffractionAndTransmission = value; }

bool SpatialAudioSettings::GetCalcEmitterVirtualPosition() const { return m_calcEmitterVirtualPosition; }
void SpatialAudioSettings::SetCalcEmitterVirtualPosition( bool value ) { m_calcEmitterVirtualPosition = value; }

float SpatialAudioSettings::GetTransmissionLoss() const { return m_transmissionLoss; }
void SpatialAudioSettings::SetTransmissionLoss( float value ) { m_transmissionLoss = std::max( 0.0f, std::min( 1.0f, value ) ); }

bool SpatialAudioSettings::GetEnableDiffraction() const { return m_enableDiffraction; }
void SpatialAudioSettings::SetEnableDiffraction( bool value ) { m_enableDiffraction = value; }

bool SpatialAudioSettings::GetEnableDiffractionOnBoundaryEdges() const { return m_enableDiffractionOnBoundaryEdges; }
void SpatialAudioSettings::SetEnableDiffractionOnBoundaryEdges( bool value ) { m_enableDiffractionOnBoundaryEdges = value; }

void SpatialAudioSettings::PopulateInitSettings( AkSpatialAudioInitSettings& out ) const
{
	out.fMovementThreshold                        = m_movementThreshold;
	out.uNumberOfPrimaryRays                      = m_numberOfPrimaryRays;
	out.uMaxReflectionOrder                       = m_maxReflectionOrder;
	out.uMaxDiffractionOrder                      = m_maxDiffractionOrder;
	out.uMaxEmitterRoomAuxSends                   = m_maxEmitterRoomAuxSends;
	out.uDiffractionOnReflectionsOrder            = m_diffractionOnReflectionsOrder;
	out.fMaxPathLength                            = m_maxPathLength;
	out.fCPULimitPercentage                       = m_cpuLimitPercentage;
	out.uLoadBalancingSpread                      = m_loadBalancingSpread;
	out.bEnableGeometricDiffractionAndTransmission = m_enableDiffractionAndTransmission;
	out.bCalcEmitterVirtualPosition               = m_calcEmitterVirtualPosition;
}
