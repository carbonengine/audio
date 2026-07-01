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
			akVertices[i] = AkVertex( static_cast<float>( v.x ), static_cast<float>( v.y ), static_cast<float>( -v.z ) );
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

AudGeometry::AudGeometry( IRoot* lockobj )
{}

AudGeometry::~AudGeometry()
{}

void AudGeometry::ClearAllGeometry()
{
	if( s_geometrySetRefCounts.empty() )
	{
		return;
	}

	if( AK::SoundEngine::IsInitialized() )
	{
		for( const auto& geometrySetEntry : s_geometrySetRefCounts )
		{
			AK::SpatialAudio::RemoveGeometry( geometrySetEntry.first );
		}
	}

	s_geometrySetRefCounts.clear();
}

AkGeometryInstanceParams AudGeometry::MakeInstanceParams(
	uint64_t geometrySetId, const Matrix& worldTransform )
{
	AkGeometryInstanceParams params;
	params.GeometrySetID = geometrySetId;
	AkTransform transform;
	RH2LH::convertTransform( worldTransform, transform );
	params.PositionAndOrientation = transform;
	params.Scale = RH2LH::extractScale( worldTransform );
	return params;
}

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

	if( !g_audioManager || !g_audioManager->GetSpatialAudioGeometryEnabled() )
	{
		return;
	}

	auto it = s_geometrySetRefCounts.find( geometrySetId );
	if( it == s_geometrySetRefCounts.end() )
	{
		std::vector<AkVertex> akVertices = ConvertVertices( geometryData.m_vertices );
		std::vector<AkTriangle> akTriangles = ConvertTriangles( geometryData.m_indices );

		AkAcousticSurface surface;
		surface.strName = "default";
		surface.textureID = AK_INVALID_UNIQUE_ID;
		surface.transmissionLoss = g_audioManager->GetTransmissionLoss();

		AkGeometryParams params;
		params.Vertices = akVertices.data();
		params.NumVertices = static_cast<AkVertIdx>( akVertices.size() );
		params.Triangles = akTriangles.data();
		params.NumTriangles = static_cast<AkTriIdx>( akTriangles.size() );
		params.Surfaces = &surface;
		params.NumSurfaces = 1;
		params.EnableDiffraction = g_audioManager->GetEnableDiffraction();
		params.EnableDiffractionOnBoundaryEdges = g_audioManager->GetEnableDiffractionOnBoundaryEdges();

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

	AKRESULT result = AK::SpatialAudio::SetGeometryInstance(
		instanceId, MakeInstanceParams( geometrySetId, worldTransform ) );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set geometry instance %llu (set %llu), AKRESULT: %d",
			instanceId, geometrySetId, result );
	}
}

void AudGeometry::SetGeometryTransform(
	uint64_t geometrySetId,
	uint64_t instanceId,
	const Matrix& worldTransform )
{
	if( !g_audioManager || !g_audioManager->GetSpatialAudioGeometryEnabled() )
	{
		return;
	}

	if( s_geometrySetRefCounts.find( geometrySetId ) == s_geometrySetRefCounts.end() )
	{
		return;
	}

	AKRESULT result = AK::SpatialAudio::SetGeometryInstance(
		instanceId, MakeInstanceParams( geometrySetId, worldTransform ) );
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
	auto it = s_geometrySetRefCounts.find( geometrySetId );
	if( it == s_geometrySetRefCounts.end() )
	{
		return;
	}

	AK::SpatialAudio::RemoveGeometryInstance( instanceId );

	it->second--;
	if( it->second == 0 )
	{
		AK::SpatialAudio::RemoveGeometry( geometrySetId );
		s_geometrySetRefCounts.erase( it );
	}
}
