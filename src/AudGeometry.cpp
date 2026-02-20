#include "stdafx.h"
#include "AudGeometry.h"
#include "Vector3.h"
#include "AudManager.h"
#include <cmath>
#include <limits>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudGeometry" );

namespace
{
	AkVector MakeAkVector( float x, float y, float z )
	{
		AkVector v;
		v.X = x;
		v.Y = y;
		v.Z = z;
		return v;
	}

	bool IsFiniteAkVector( const AkVector& v )
	{
		return std::isfinite( v.X ) && std::isfinite( v.Y ) && std::isfinite( v.Z );
	}

	float Dot( const AkVector& a, const AkVector& b )
	{
		return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
	}

	float LengthSq( const AkVector& v )
	{
		return Dot( v, v );
	}

	AkVector Cross( const AkVector& a, const AkVector& b )
	{
		return MakeAkVector(
			a.Y * b.Z - a.Z * b.Y,
			a.Z * b.X - a.X * b.Z,
			a.X * b.Y - a.Y * b.X
		);
	}

	bool NormalizeSafe( AkVector& v )
	{
		const float lenSq = LengthSq( v );
		if( lenSq <= 1e-10f || !std::isfinite( lenSq ) )
		{
			return false;
		}

		const float invLen = 1.0f / std::sqrt( lenSq );
		v.X *= invLen;
		v.Y *= invLen;
		v.Z *= invLen;
		return IsFiniteAkVector( v );
	}

	float AxisScaleOrDefault( float x, float y, float z )
	{
		const float lenSq = x * x + y * y + z * z;
		if( lenSq <= 1e-10f || !std::isfinite( lenSq ) )
		{
			return 1.0f;
		}

		const float len = std::sqrt( lenSq );
		return std::isfinite( len ) ? len : 1.0f;
	}

	AkVector ExtractScale( const Matrix& matrix )
	{
		return MakeAkVector(
			AxisScaleOrDefault( matrix._11, matrix._12, matrix._13 ),
			AxisScaleOrDefault( matrix._21, matrix._22, matrix._23 ),
			AxisScaleOrDefault( matrix._31, matrix._32, matrix._33 )
		);
	}

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

	void ConvertTransform( const Matrix& matrix, AkTransform& transformOut )
	{
		AkVector position = MakeAkVector( matrix._41, matrix._42, -matrix._43 );
		if( !IsFiniteAkVector( position ) )
		{
			position = MakeAkVector( 0.0f, 0.0f, 0.0f );
		}

		AkVector front = MakeAkVector( matrix._31, matrix._32, -matrix._33 );
		AkVector up = MakeAkVector( matrix._21, matrix._22, -matrix._23 );

		if( !IsFiniteAkVector( front ) || !IsFiniteAkVector( up ) )
		{
			front = MakeAkVector( 0.0f, 0.0f, 1.0f );
			up = MakeAkVector( 0.0f, 1.0f, 0.0f );
		}

		if( !NormalizeSafe( front ) )
		{
			front = MakeAkVector( 0.0f, 0.0f, 1.0f );
		}

		const float upDotFront = Dot( up, front );
		up.X -= front.X * upDotFront;
		up.Y -= front.Y * upDotFront;
		up.Z -= front.Z * upDotFront;

		if( !NormalizeSafe( up ) )
		{
			AkVector reference = ( std::fabs( front.Y ) < 0.99f ) ?
				MakeAkVector( 0.0f, 1.0f, 0.0f ) :
				MakeAkVector( 1.0f, 0.0f, 0.0f );

			AkVector right = Cross( reference, front );
			if( !NormalizeSafe( right ) )
			{
				reference = MakeAkVector( 0.0f, 0.0f, 1.0f );
				right = Cross( reference, front );
			}

			if( !NormalizeSafe( right ) )
			{
				right = MakeAkVector( 1.0f, 0.0f, 0.0f );
			}

			up = Cross( front, right );
			if( !NormalizeSafe( up ) )
			{
				up = MakeAkVector( 0.0f, 1.0f, 0.0f );
			}
		}

		transformOut.SetPosition( position );
		transformOut.SetOrientation( front, up );
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
	ConvertTransform( worldTransform, transform );
	instanceParams.PositionAndOrientation = transform;
	instanceParams.Scale = ExtractScale( worldTransform );

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
	ConvertTransform( worldTransform, transform );
	instanceParams.PositionAndOrientation = transform;
	instanceParams.Scale = ExtractScale( worldTransform );

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
