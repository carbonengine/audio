#include "stdafx.h"
#include "AudGeometry.h"
#include "Vector3.h"
#include "AudManager.h"
#include <limits>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudGeometry" );

namespace
{
	std::vector<AkVertex> ConvertVertices( const std::vector<Vector3>& vertices )
	{
		std::vector<AkVertex> akVertices( vertices.size() );
		for( size_t i = 0; i < vertices.size(); ++i )
		{
			const Vector3& v = vertices[i];
			akVertices[i] = AkVertex( v.x, v.y, -v.z );
		}
		return akVertices;
	}

	std::vector<AkTriangle> ConvertTriangles( const std::vector<uint32_t>& indices )
	{
		size_t numTriangles = indices.size() / 3;
		std::vector<AkTriangle> akTriangles( numTriangles );
		for( size_t i = 0; i < numTriangles; ++i )
		{
			akTriangles[i] = AkTriangle(
				static_cast<AkVertIdx>( indices[i * 3 + 0] ),
				static_cast<AkVertIdx>( indices[i * 3 + 2] ),
				static_cast<AkVertIdx>( indices[i * 3 + 1] ),
				0
			);
		}
		return akTriangles;
	}

	bool IsTriangleDegenerate( const AkVertex& a, const AkVertex& b, const AkVertex& c )
	{
		constexpr float kEpsilonSq = 1e-12f;

		float e1x = b.X - a.X, e1y = b.Y - a.Y, e1z = b.Z - a.Z;
		float e2x = c.X - a.X, e2y = c.Y - a.Y, e2z = c.Z - a.Z;

		float cx = e1y * e2z - e1z * e2y;
		float cy = e1z * e2x - e1x * e2z;
		float cz = e1x * e2y - e1y * e2x;

		return ( cx * cx + cy * cy + cz * cz ) <= kEpsilonSq;
	}

	AkTransform ConvertTransform( const Matrix& matrix )
	{
		AkTransform transform;

		AkVector position;
		position.X = matrix._41;
		position.Y = matrix._42;
		position.Z = -matrix._43;

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

std::unordered_map<uint64_t, uint32_t> AudGeometry::s_geometrySetRefCounts;
CcpMutex AudGeometry::s_mutex( "AudGeometry", "s_mutex" );

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
	if( geometryData.m_vertices.empty() || geometryData.m_indices.empty() )
	{
		return;
	}

	if( geometryData.m_indices.size() % 3 != 0 )
	{
		CCP_LOGERR_CH( s_ch, "SetGeometry: index count %zu is not a multiple of 3 for instance %llu",
			geometryData.m_indices.size(), instanceId );
		return;
	}

	constexpr size_t kMaxVertices = std::numeric_limits<AkVertIdx>::max();
	constexpr size_t kMaxTriangles = std::numeric_limits<AkTriIdx>::max();

	if( geometryData.m_vertices.size() > kMaxVertices )
	{
		CCP_LOGERR_CH( s_ch, "SetGeometry: vertex count %zu exceeds max %zu for geometry set %llu, skipping",
			geometryData.m_vertices.size(), kMaxVertices, geometrySetId );
		return;
	}

	size_t numTriangles = geometryData.m_indices.size() / 3;
	if( numTriangles > kMaxTriangles )
	{
		CCP_LOGERR_CH( s_ch, "SetGeometry: triangle count %zu exceeds max %zu for geometry set %llu, skipping",
			numTriangles, kMaxTriangles, geometrySetId );
		return;
	}

	for( size_t i = 0; i < geometryData.m_indices.size(); ++i )
	{
		if( geometryData.m_indices[i] >= geometryData.m_vertices.size() )
		{
			CCP_LOGERR_CH( s_ch, "SetGeometry: index[%zu] = %u out of bounds (vertex count: %zu) for instance %llu",
				i, geometryData.m_indices[i], geometryData.m_vertices.size(), instanceId );
			return;
		}
	}

	CcpAutoMutex lock( s_mutex );

	auto it = s_geometrySetRefCounts.find( geometrySetId );
	if( it == s_geometrySetRefCounts.end() )
	{
		std::vector<AkVertex> akVertices = ConvertVertices( geometryData.m_vertices );
		std::vector<AkTriangle> allTriangles = ConvertTriangles( geometryData.m_indices );

		std::vector<AkTriangle> akTriangles;
		akTriangles.reserve( allTriangles.size() );
		for( const AkTriangle& tri : allTriangles )
		{
			if( !IsTriangleDegenerate( akVertices[tri.point0], akVertices[tri.point1], akVertices[tri.point2] ) )
			{
				akTriangles.push_back( tri );
			}
		}

		AkAcousticSurface surface;
		surface.strName = "default";
		surface.textureID = AK_INVALID_UNIQUE_ID;
		surface.transmissionLoss = 1.0f;

		AkGeometryParams params;
		params.Vertices = akVertices.data();
		params.NumVertices = static_cast<AkVertIdx>( akVertices.size() );
		params.Triangles = akTriangles.data();
		params.NumTriangles = static_cast<AkTriIdx>( akTriangles.size() );
		params.Surfaces = &surface;
		params.NumSurfaces = 1;
		params.EnableDiffraction = true;
		params.EnableDiffractionOnBoundaryEdges = false;

		AKRESULT result = AK::SpatialAudio::SetGeometry( geometrySetId, params );
		if( result != AK_Success )
		{
			CCP_LOGERR_CH( s_ch, "Failed to set geometry for set %llu, AKRESULT: %d", geometrySetId, result );
			return;
		}

		s_geometrySetRefCounts[geometrySetId] = 1;
	}
	else
	{
		it->second++;
	}

	AkGeometryInstanceParams instanceParams;
	instanceParams.GeometrySetID = geometrySetId;
	instanceParams.PositionAndOrientation = ConvertTransform( worldTransform );

	AKRESULT instanceResult = AK::SpatialAudio::SetGeometryInstance( instanceId, instanceParams );
	if( instanceResult != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set geometry instance %llu (set %llu), AKRESULT: %d",
			instanceId, geometrySetId, instanceResult );
	}
}

void AudGeometry::SetGeometryTransform(
	uint64_t geometrySetId,
	uint64_t instanceId,
	const Matrix& worldTransform )
{
	{
		CcpAutoMutex lock( s_mutex );
		if( s_geometrySetRefCounts.find( geometrySetId ) == s_geometrySetRefCounts.end() )
		{
			return;
		}
	}

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

	CcpAutoMutex lock( s_mutex );

	auto it = s_geometrySetRefCounts.find( geometrySetId );
	if( it != s_geometrySetRefCounts.end() )
	{
		it->second--;
		if( it->second == 0 )
		{
			AK::SpatialAudio::RemoveGeometry( geometrySetId );
			s_geometrySetRefCounts.erase( it );
		}
	}
}
