#include "stdafx.h"
#include "AudGeometry.h"
#include "Vector3.h"
#include "AudManager.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudGeometry" );

namespace
{
	// Convert Vector3 array to AkVertex array
	// Applies RH to LH coordinate conversion (negate Z)
	std::vector<AkVertex> ConvertVertices( const std::vector<Vector3>& vertices )
	{
		std::vector<AkVertex> akVertices( vertices.size() );
		for( size_t i = 0; i < vertices.size(); ++i )
		{
			const Vector3& v = vertices[i];
			// RH to LH: negate Z
			akVertices[i] = AkVertex( v.x, v.y, -v.z );
		}
		return akVertices;
	}

	// Convert index array to AkTriangle array
	// Every 3 indices form one triangle
	// Winding order is reversed due to coordinate system change
	std::vector<AkTriangle> ConvertTriangles( const std::vector<uint32_t>& indices )
	{
		size_t numTriangles = indices.size() / 3;
		std::vector<AkTriangle> akTriangles( numTriangles );
		for( size_t i = 0; i < numTriangles; ++i )
		{
			// Reverse winding order (0,1,2 -> 0,2,1) for RH to LH conversion
			akTriangles[i] = AkTriangle(
				static_cast<AkVertIdx>( indices[i * 3 + 0] ),
				static_cast<AkVertIdx>( indices[i * 3 + 2] ),
				static_cast<AkVertIdx>( indices[i * 3 + 1] ),
				0  // default surface index
			);
		}
		return akTriangles;
	}

	// Convert Matrix to AkTransform for geometry instance placement
	// Applies RH to LH coordinate conversion
	AkTransform ConvertTransform( const Matrix& matrix )
	{
		AkTransform transform;

		// Extract position from matrix
		AkVector position;
		position.X = matrix._41;
		position.Y = matrix._42;
		position.Z = -matrix._43;

		// Extract forward
		AkVector orientationFront;
		orientationFront.X = matrix._31;
		orientationFront.Y = matrix._32;
		orientationFront.Z = -matrix._33;

		AkVector orientationTop;
		orientationTop.X = matrix._21;
		orientationTop.Y = matrix._22;
		orientationTop.Z = -matrix._23;

		transform.SetPosition( position );
		transform.SetOrientation( orientationFront, orientationTop );
		return transform;
	}
}

AudGeometry::AudGeometry( IRoot* lockobj )
	: m_mutex( "AudGeometry", "m_mutex" )
{}

AudGeometry::~AudGeometry()
{
	for( const auto& pair : m_geometrySetRefCounts )
	{
		AK::SpatialAudio::RemoveGeometry( pair.first );
		CCP_LOGWARN_CH( s_ch, "Cleanup: removed orphaned geometry set %llu with %u remaining refs",
			pair.first, pair.second );
	}
	m_geometrySetRefCounts.clear();
}

void AudGeometry::SetGeometry(
	uint64_t geometrySetId,
	uint64_t instanceId,
	const Tr2AudGeometryData& geometryData,
	const Matrix& worldTransform )
{
	if( geometryData.m_vertices.empty() || geometryData.m_indices.empty() )
	{
		CCP_LOGWARN_CH( s_ch, "SetGeometry called with empty geometry data for instance %llu", instanceId );
		return;
	}

	if( geometryData.m_indices.size() % 3 != 0 )
	{
		CCP_LOGERR_CH( s_ch, "SetGeometry: index count %zu is not a multiple of 3 for instance %llu",
			geometryData.m_indices.size(), instanceId );
		return;
	}

	// Validate that all triangle indices are within vertex bounds
	for( size_t i = 0; i < geometryData.m_indices.size(); ++i )
	{
		if( geometryData.m_indices[i] >= geometryData.m_vertices.size() )
		{
			CCP_LOGERR_CH( s_ch, "SetGeometry: index[%zu] = %u is out of bounds (vertex count: %zu) for instance %llu",
				i, geometryData.m_indices[i], geometryData.m_vertices.size(), instanceId );
			return;
		}
	}

	CcpAutoMutex lock( m_mutex );

	// Only register the geometry set with Wwise if this is the first instance using it
	auto it = m_geometrySetRefCounts.find( geometrySetId );
	if( it == m_geometrySetRefCounts.end() )
	{
		// Convert vertices and triangles to Wwise format
		std::vector<AkVertex> akVertices = ConvertVertices( geometryData.m_vertices );
		std::vector<AkTriangle> akTriangles = ConvertTriangles( geometryData.m_indices );

		// Default acoustic surface - all triangles share the same surface properties
		AkAcousticSurface surface;
		surface.strName = "default";
		surface.textureID = AK_INVALID_UNIQUE_ID;
		surface.transmissionLoss = 1.0f;

		// Set up geometry parameters
		AkGeometryParams params;
		params.Vertices = akVertices.data();
		params.NumVertices = static_cast<AkVertIdx>( akVertices.size() );
		params.Triangles = akTriangles.data();
		params.NumTriangles = static_cast<AkTriIdx>( akTriangles.size() );
		params.Surfaces = &surface;
		params.NumSurfaces = 1;
		params.EnableDiffraction = true;
		params.EnableDiffractionOnBoundaryEdges = true;

		CCP_LOG_CH( s_ch, "Registering geometry set %llu: %u vertices, %u triangles",
			geometrySetId, params.NumVertices, params.NumTriangles );

		AKRESULT result = AK::SpatialAudio::SetGeometry( geometrySetId, params );
		if( result != AK_Success )
		{
			CCP_LOGERR_CH( s_ch, "Failed to set geometry for set %llu, AKRESULT: %d", geometrySetId, result );
			return;
		}

		m_geometrySetRefCounts[geometrySetId] = 1;
	}
	else
	{
		it->second++;
		CCP_LOG_CH( s_ch, "Geometry set %llu ref count incremented to %u", geometrySetId, it->second );
	}

	// Create a geometry instance for this specific object
	AkGeometryInstanceParams instanceParams;
	instanceParams.GeometrySetID = geometrySetId;
	instanceParams.PositionAndOrientation = ConvertTransform( worldTransform );

	AKRESULT instanceResult = AK::SpatialAudio::SetGeometryInstance( instanceId, instanceParams );
	if( instanceResult != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set geometry instance %llu (set %llu), AKRESULT: %d",
			instanceId, geometrySetId, instanceResult );
		return;
	}

	CCP_LOG_CH( s_ch, "Placed geometry instance %llu referencing set %llu", instanceId, geometrySetId );
}

void AudGeometry::SetGeometryTransform(
	uint64_t geometrySetId,
	uint64_t instanceId,
	const Matrix& worldTransform )
{
	AkGeometryInstanceParams instanceParams;
	instanceParams.GeometrySetID = geometrySetId;
	instanceParams.PositionAndOrientation = ConvertTransform( worldTransform );

	AKRESULT result = AK::SpatialAudio::SetGeometryInstance( instanceId, instanceParams );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to update geometry instance transform for instance %llu (set %llu), AKRESULT: %d",
			instanceId, geometrySetId, result );
	}
}

void AudGeometry::RemoveGeometry(
	uint64_t geometrySetId,
	uint64_t instanceId )
{
	AK::SpatialAudio::RemoveGeometryInstance( instanceId );
	CCP_LOG_CH( s_ch, "Removed geometry instance %llu", instanceId );

	CcpAutoMutex lock( m_mutex );

	auto it = m_geometrySetRefCounts.find( geometrySetId );
	if( it != m_geometrySetRefCounts.end() )
	{
		it->second--;
		if( it->second == 0 )
		{
			AK::SpatialAudio::RemoveGeometry( geometrySetId );
			m_geometrySetRefCounts.erase( it );
			CCP_LOG_CH( s_ch, "Removed geometry set %llu (last instance removed)", geometrySetId );
		}
		else
		{
			CCP_LOG_CH( s_ch, "Geometry set %llu ref count decremented to %u", geometrySetId, it->second );
		}
	}
	else
	{
		CCP_LOGWARN_CH( s_ch, "RemoveGeometry called for unknown geometry set %llu", geometrySetId );
	}
}
