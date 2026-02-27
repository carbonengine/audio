#include "stdafx.h"
#include "AudGeometry.h"
#include "Utilities.h"
#include "Vector3.h"
#include "AudManager.h"

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
				static_cast<AkVertIdx>( indices[i * 3 + 1] ),
				static_cast<AkVertIdx>( indices[i * 3 + 2] ),
				0
			);
		}
		return akTriangles;
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
	CcpAutoMutex lock( s_mutex );

	auto it = s_geometrySetRefCounts.find( geometrySetId );
	if( it == s_geometrySetRefCounts.end() )
	{
		std::vector<AkVertex> akVertices = ConvertVertices( geometryData.m_vertices );
		std::vector<AkTriangle> akTriangles = ConvertTriangles( geometryData.m_indices );

		AkAcousticSurface surface;
		surface.strName = "default";
		surface.textureID = AK_INVALID_UNIQUE_ID;
		surface.transmissionLoss = 0.7f;

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
	AkTransform transform;
	RH2LH::convertTransform( worldTransform, transform );
	instanceParams.PositionAndOrientation = transform;
	instanceParams.Scale = RH2LH::extractScale( worldTransform );

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
	AkTransform transform;
	RH2LH::convertTransform( worldTransform, transform );
	instanceParams.PositionAndOrientation = transform;
	instanceParams.Scale = RH2LH::extractScale( worldTransform );

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
