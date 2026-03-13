#include "stdafx.h"
#include "SpatialAudioSettings.h"

SpatialAudioSettings::SpatialAudioSettings()
	: m_occlusionMode( AudOcclusionMode::On )
	, m_maxSoundPropagationDepth( 1 )
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
{
}

AudOcclusionMode SpatialAudioSettings::GetOcclusionMode() const { return m_occlusionMode; }
void SpatialAudioSettings::SetOcclusionMode( AudOcclusionMode value ) { m_occlusionMode = value; }

int SpatialAudioSettings::GetMaxSoundPropagationDepth() const { return m_maxSoundPropagationDepth; }
void SpatialAudioSettings::SetMaxSoundPropagationDepth( int value ) { m_maxSoundPropagationDepth = value; }

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
