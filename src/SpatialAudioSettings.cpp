#include "stdafx.h"
#include "SpatialAudioSettings.h"
#include <AK/SpatialAudio/Common/AkSpatialAudio.h>

#include <algorithm>
#include <cmath>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "SpatialAudioSettings" );

namespace
{
	/// Smallest diffraction angle, which Wwise needs above zero.
	constexpr float MIN_DIFFRACTION_ANGLE_DEGREES = 1.0f;

	/// Stores value in setting, and reports whether that changed it, so Wwise only hears about changes.
	template<typename T>
	bool Store( T& setting, T value )
	{
		if( setting == value )
		{
			return false;
		}
		setting = value;
		return true;
	}

	void LogIfFailed( AKRESULT result, const char* setting )
	{
		if( result != AK_Success )
		{
			CCP_LOGERR_CH( s_ch, "Failed to apply the spatial audio setting %s, AKRESULT: %d", setting, result );
		}
	}
}

SpatialAudioSettings::SpatialAudioSettings()
	: m_initialized( false )
	, m_spatialAudioGeometryEnabled( false )
	, m_spatialAudioRoomsEnabled( false )
	, m_movementThreshold( 100.0f )
	// Wwise's Ray Tracing Engine Geometry Guide: as few as 5 primary rays still give good results, and they set the ray tracing cost.
	, m_numberOfPrimaryRays( 5 )
	, m_maxReflectionOrder( 0 )
	, m_maxDiffractionOrder( 4 )
	, m_maxDiffractionPaths( 3 )
	, m_maxGlobalReflectionPaths( 0 )
	, m_maxEmitterRoomAuxSends( 0 )
	, m_diffractionOnReflectionsOrder( 0 )
	, m_maxDiffractionAngleDegrees( 180.0f )
	, m_maxPathLength( 30000.0f )
	, m_cpuLimitPercentage( 10.0f )
	// The same guide: spreads under 10 frames should not noticeably affect quality.
	, m_loadBalancingSpread( 9 )
	, m_smoothingConstantMs( 500.0f )
	, m_adjacentRoomBleed( 1.0f )
	, m_maxSoundPropagationDepth( AK_MAX_SOUND_PROPAGATION_DEPTH )
	, m_enableDiffractionAndTransmission( true )
	, m_calcEmitterVirtualPosition( true )
	, m_transmissionOperation( AkTransmissionOperation_Default )
	, m_clusteringMinPoints( 2 )
	, m_clusteringMaxDistance( 300.0f )
	, m_clusteringDeadZoneDistance( 3000.0f )
	, m_transmissionLoss( 1.0f )
	, m_enableDiffraction( true )
	, m_enableDiffractionOnBoundaryEdges( false )
	, m_geometrySolid( false )
	, m_geometryPortalSubtraction( false )
	, m_geometryMaxDistance( 0.0f )
	, m_geometryMaxTriangles( 4000 )
{
}

void SpatialAudioSettings::SetInitialized( bool initialized ) { m_initialized = initialized; }

bool SpatialAudioSettings::GetSpatialAudioGeometryEnabled() const { return m_spatialAudioGeometryEnabled; }
void SpatialAudioSettings::SetSpatialAudioGeometryEnabled( bool value ) { m_spatialAudioGeometryEnabled = value; }

bool SpatialAudioSettings::GetSpatialAudioRoomsEnabled() const { return m_spatialAudioRoomsEnabled; }
void SpatialAudioSettings::SetSpatialAudioRoomsEnabled( bool value ) { m_spatialAudioRoomsEnabled = value; }

float SpatialAudioSettings::GetMovementThreshold() const { return m_movementThreshold; }
void SpatialAudioSettings::SetMovementThreshold( float value ) { if( !std::isnan( value ) ) m_movementThreshold = std::max( 0.0f, value ); }

int SpatialAudioSettings::GetNumberOfPrimaryRays() const { return m_numberOfPrimaryRays; }
void SpatialAudioSettings::SetNumberOfPrimaryRays( int value )
{
	if( Store( m_numberOfPrimaryRays, std::max( 0, value ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetNumberOfPrimaryRays( m_numberOfPrimaryRays ), "numberOfPrimaryRays" );
	}
}

int SpatialAudioSettings::GetMaxReflectionOrder() const { return m_maxReflectionOrder; }
void SpatialAudioSettings::SetMaxReflectionOrder( int value )
{
	if( Store( m_maxReflectionOrder, std::clamp( value, 0, AK_MAX_REFLECT_ORDER ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetReflectionsOrder( m_maxReflectionOrder, true ), "maxReflectionOrder" );
	}
}

int SpatialAudioSettings::GetMaxDiffractionOrder() const { return m_maxDiffractionOrder; }
void SpatialAudioSettings::SetMaxDiffractionOrder( int value )
{
	if( Store( m_maxDiffractionOrder, std::clamp( value, 0, AK_MAX_SOUND_PROPAGATION_DEPTH ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetDiffractionOrder( m_maxDiffractionOrder, true ), "maxDiffractionOrder" );
	}
}

int SpatialAudioSettings::GetMaxDiffractionPaths() const { return m_maxDiffractionPaths; }
void SpatialAudioSettings::SetMaxDiffractionPaths( int value )
{
	if( Store( m_maxDiffractionPaths, std::clamp( value, 0, AK_MAX_SOUND_PROPAGATION_WIDTH ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetMaxDiffractionPaths( m_maxDiffractionPaths ), "maxDiffractionPaths" );
	}
}

int SpatialAudioSettings::GetMaxGlobalReflectionPaths() const { return m_maxGlobalReflectionPaths; }
void SpatialAudioSettings::SetMaxGlobalReflectionPaths( int value )
{
	if( Store( m_maxGlobalReflectionPaths, std::max( 0, value ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetMaxGlobalReflectionPaths( m_maxGlobalReflectionPaths ), "maxGlobalReflectionPaths" );
	}
}

int SpatialAudioSettings::GetMaxEmitterRoomAuxSends() const { return m_maxEmitterRoomAuxSends; }
void SpatialAudioSettings::SetMaxEmitterRoomAuxSends( int value )
{
	if( Store( m_maxEmitterRoomAuxSends, std::max( 0, value ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetMaxEmitterRoomAuxSends( m_maxEmitterRoomAuxSends ), "maxEmitterRoomAuxSends" );
	}
}

int SpatialAudioSettings::GetDiffractionOnReflectionsOrder() const { return m_diffractionOnReflectionsOrder; }
void SpatialAudioSettings::SetDiffractionOnReflectionsOrder( int value ) { m_diffractionOnReflectionsOrder = std::max( 0, value ); }

float SpatialAudioSettings::GetMaxDiffractionAngleDegrees() const { return m_maxDiffractionAngleDegrees; }
void SpatialAudioSettings::SetMaxDiffractionAngleDegrees( float value ) { if( !std::isnan( value ) ) m_maxDiffractionAngleDegrees = std::max( MIN_DIFFRACTION_ANGLE_DEGREES, value ); }

float SpatialAudioSettings::GetMaxPathLength() const { return m_maxPathLength; }
void SpatialAudioSettings::SetMaxPathLength( float value ) { if( !std::isnan( value ) ) m_maxPathLength = std::max( 0.0f, value ); }

float SpatialAudioSettings::GetCPULimitPercentage() const { return m_cpuLimitPercentage; }
void SpatialAudioSettings::SetCPULimitPercentage( float value ) { if( !std::isnan( value ) ) m_cpuLimitPercentage = std::clamp( value, 0.0f, 100.0f ); }

int SpatialAudioSettings::GetLoadBalancingSpread() const { return m_loadBalancingSpread; }
void SpatialAudioSettings::SetLoadBalancingSpread( int value )
{
	if( Store( m_loadBalancingSpread, std::max( 1, value ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetLoadBalancingSpread( m_loadBalancingSpread ), "loadBalancingSpread" );
	}
}

float SpatialAudioSettings::GetSmoothingConstantMs() const { return m_smoothingConstantMs; }
void SpatialAudioSettings::SetSmoothingConstantMs( float value )
{
	if( !std::isnan( value ) && Store( m_smoothingConstantMs, std::max( 0.0f, value ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetSmoothingConstant( m_smoothingConstantMs ), "smoothingConstantMs" );
	}
}

float SpatialAudioSettings::GetAdjacentRoomBleed() const { return m_adjacentRoomBleed; }
void SpatialAudioSettings::SetAdjacentRoomBleed( float value )
{
	if( !std::isnan( value ) && Store( m_adjacentRoomBleed, std::max( MIN_ADJACENT_ROOM_BLEED, value ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetAdjacentRoomBleed( m_adjacentRoomBleed ), "adjacentRoomBleed" );
	}
}

int SpatialAudioSettings::GetMaxSoundPropagationDepth() const { return m_maxSoundPropagationDepth; }
void SpatialAudioSettings::SetMaxSoundPropagationDepth( int value ) { m_maxSoundPropagationDepth = std::clamp( value, 0, AK_MAX_SOUND_PROPAGATION_DEPTH ); }

bool SpatialAudioSettings::GetEnableDiffractionAndTransmission() const { return m_enableDiffractionAndTransmission; }
void SpatialAudioSettings::SetEnableDiffractionAndTransmission( bool value ) { m_enableDiffractionAndTransmission = value; }

bool SpatialAudioSettings::GetCalcEmitterVirtualPosition() const { return m_calcEmitterVirtualPosition; }
void SpatialAudioSettings::SetCalcEmitterVirtualPosition( bool value ) { m_calcEmitterVirtualPosition = value; }

int SpatialAudioSettings::GetTransmissionOperation() const { return m_transmissionOperation; }
void SpatialAudioSettings::SetTransmissionOperation( int value )
{
	if( Store( m_transmissionOperation, std::clamp( value, static_cast<int>( AkTransmissionOperation_Add ), static_cast<int>( AkTransmissionOperation_Max ) ) ) && m_initialized )
	{
		LogIfFailed( AK::SpatialAudio::SetTransmissionOperation( static_cast<AkTransmissionOperation>( m_transmissionOperation ) ), "transmissionOperation" );
	}
}

int SpatialAudioSettings::GetClusteringMinPoints() const { return m_clusteringMinPoints; }
void SpatialAudioSettings::SetClusteringMinPoints( int value ) { m_clusteringMinPoints = std::max( 0, value ); }

float SpatialAudioSettings::GetClusteringMaxDistance() const { return m_clusteringMaxDistance; }
void SpatialAudioSettings::SetClusteringMaxDistance( float value ) { if( !std::isnan( value ) ) m_clusteringMaxDistance = std::max( 0.0f, value ); }

float SpatialAudioSettings::GetClusteringDeadZoneDistance() const { return m_clusteringDeadZoneDistance; }
void SpatialAudioSettings::SetClusteringDeadZoneDistance( float value ) { if( !std::isnan( value ) ) m_clusteringDeadZoneDistance = std::max( 0.0f, value ); }

float SpatialAudioSettings::GetTransmissionLoss() const { return m_transmissionLoss; }
void SpatialAudioSettings::SetTransmissionLoss( float value ) { if( !std::isnan( value ) ) m_transmissionLoss = std::clamp( value, 0.0f, 1.0f ); }

bool SpatialAudioSettings::GetEnableDiffraction() const { return m_enableDiffraction; }
void SpatialAudioSettings::SetEnableDiffraction( bool value ) { m_enableDiffraction = value; }

bool SpatialAudioSettings::GetEnableDiffractionOnBoundaryEdges() const { return m_enableDiffractionOnBoundaryEdges; }
void SpatialAudioSettings::SetEnableDiffractionOnBoundaryEdges( bool value ) { m_enableDiffractionOnBoundaryEdges = value; }

bool SpatialAudioSettings::GetGeometrySolid() const { return m_geometrySolid; }
void SpatialAudioSettings::SetGeometrySolid( bool value ) { m_geometrySolid = value; }

bool SpatialAudioSettings::GetGeometryPortalSubtraction() const { return m_geometryPortalSubtraction; }
void SpatialAudioSettings::SetGeometryPortalSubtraction( bool value ) { m_geometryPortalSubtraction = value; }

float SpatialAudioSettings::GetGeometryMaxDistance() const { return m_geometryMaxDistance; }
void SpatialAudioSettings::SetGeometryMaxDistance( float value ) { if( !std::isnan( value ) ) m_geometryMaxDistance = std::max( 0.0f, value ); }

int SpatialAudioSettings::GetGeometryMaxTriangles() const { return m_geometryMaxTriangles; }
void SpatialAudioSettings::SetGeometryMaxTriangles( int value ) { m_geometryMaxTriangles = std::max( 0, value ); }

void SpatialAudioSettings::PopulateInitSettings( AkSpatialAudioInitSettings& out ) const
{
	out.uMaxSoundPropagationDepth                  = m_maxSoundPropagationDepth;
	out.fMovementThreshold                         = m_movementThreshold;
	out.uNumberOfPrimaryRays                       = m_numberOfPrimaryRays;
	out.uMaxReflectionOrder                        = m_maxReflectionOrder;
	out.uMaxDiffractionOrder                       = m_maxDiffractionOrder;
	out.uMaxDiffractionPaths                       = m_maxDiffractionPaths;
	out.uMaxGlobalReflectionPaths                  = m_maxGlobalReflectionPaths;
	out.uMaxEmitterRoomAuxSends                    = m_maxEmitterRoomAuxSends;
	out.uDiffractionOnReflectionsOrder             = m_diffractionOnReflectionsOrder;
	out.fMaxDiffractionAngleDegrees                = m_maxDiffractionAngleDegrees;
	out.fMaxPathLength                             = m_maxPathLength;
	out.fCPULimitPercentage                        = m_cpuLimitPercentage;
	out.uLoadBalancingSpread                       = m_loadBalancingSpread;
	out.fSmoothingConstantMs                       = m_smoothingConstantMs;
	out.fAdjacentRoomBleed                         = m_adjacentRoomBleed;
	out.bEnableGeometricDiffractionAndTransmission = m_enableDiffractionAndTransmission;
	out.bCalcEmitterVirtualPosition                = m_calcEmitterVirtualPosition;
	out.eTransmissionOperation                     = static_cast<AkTransmissionOperation>( m_transmissionOperation );
	out.uClusteringMinPoints                       = m_clusteringMinPoints;
	out.fClusteringMaxDistance                     = m_clusteringMaxDistance;
	out.fClusteringDeadZoneDistance                = m_clusteringDeadZoneDistance;
}
