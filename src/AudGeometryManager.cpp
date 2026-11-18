////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudGeometryManager.h"

#include "SpatialAudioSettings.h"
#include "Utilities.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudGeometry" );

CCP_STATS_DECLARE( geometrySets, "CarbonAudio/AudGeometry/Sets", false, CST_COUNTER_LOW, "Geometry sets in Wwise." );
CCP_STATS_DECLARE( geometryInstances, "CarbonAudio/AudGeometry/Instances", false, CST_COUNTER_LOW, "Geometry instances in Wwise." );
CCP_STATS_DECLARE( geometryTriangles, "CarbonAudio/AudGeometry/Triangles", false, CST_COUNTER_LOW, "Triangles of every geometry instance in Wwise." );

namespace
{
	/// Wwise indexes vertices and triangles with 16 bits and reserves the last value.
	constexpr size_t MAX_ELEMENTS = AK_INVALID_VERTEX;
	constexpr uint32_t UNMAPPED = ~0u;
	/// Furthest an instance can be from the origin, and the range its scale can have, before it is left out of Wwise.
	/// Trinity parks objects that left the scene at -FLT_MAX and places distant celestials in solar system coordinates.
	constexpr float MAX_COORDINATE = 1.0e7f;
	constexpr float MIN_SCALE = 1.0e-6f;
	constexpr float MAX_SCALE = 1.0e6f;
	/// An instance already in Wwise stays until it is this much further out, and ranks this much nearer, so
	/// instances at the edge of the limits don't flip in and out.
	constexpr float KEEP_DISTANCE = 1.1f;
	constexpr float KEEP_RANK = 0.9f;
	/// A mesh within this of where Wwise has it is where Wwise has it, in game units.
	constexpr double STILL_IN_WWISE = 1.0e-3;
	AkVector64 Translation( const Matrix& m )
	{
		AkVector64 v;
		v.X = m._41;
		v.Y = m._42;
		v.Z = m._43;
		return v;
	}

	bool SameVector64( const AkVector64& a, const AkVector64& b )
	{
		return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
	}

	bool SameRotationAndScale( const Matrix& a, const Matrix& b )
	{
		return a._11 == b._11 && a._12 == b._12 && a._13 == b._13
			&& a._21 == b._21 && a._22 == b._22 && a._23 == b._23
			&& a._31 == b._31 && a._32 == b._32 && a._33 == b._33;
	}

	/// Whether an instance handed over at sceneRelative went from before to after, beyond float noise.
	bool MovedInWorld( const AkVector64& before, const AkVector64& after, const Matrix& sceneRelative )
	{
		const double still = AudWorldFrame::StillDistance( sceneRelative );
		const AkVector64 d = after - before;
		return d.X * d.X + d.Y * d.Y + d.Z * d.Z > still * still;
	}

	struct PositionHash
	{
		size_t operator()( const AkVertex& v ) const
		{
			uint32_t bits[3];
			memcpy( bits, &v.X, sizeof( bits ) );
			uint64_t hash = bits[0];
			hash = hash * 0x9E3779B97F4A7C15ull ^ bits[1];
			hash = hash * 0x9E3779B97F4A7C15ull ^ bits[2];
			return static_cast<size_t>( hash ^ ( hash >> 32 ) );
		}
	};

	struct PositionEqual
	{
		bool operator()( const AkVertex& a, const AkVertex& b ) const
		{
			return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
		}
	};

	bool IsUsableTransform( const Matrix& m, float& largestScale )
	{
		const float translation[3] = { m._41, m._42, m._43 };
		for( const float value : translation )
		{
			if( !std::isfinite( value ) || std::fabs( value ) > MAX_COORDINATE )
			{
				return false;
			}
		}

		largestScale = 0.0f;
		const float axes[3][3] = { { m._11, m._12, m._13 }, { m._21, m._22, m._23 }, { m._31, m._32, m._33 } };
		for( const auto& axis : axes )
		{
			const float length = std::sqrt( axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2] );
			if( !std::isfinite( length ) || length < MIN_SCALE || length > MAX_SCALE )
			{
				return false;
			}
			largestScale = std::max( largestScale, length );
		}
		return true;
	}

	/// Distance from a point to the centre of the instance's bounding sphere.
	float DistanceToCenter( const Matrix& m, const AudGeometryManager::Mesh& mesh, const Vector3& point )
	{
		const float* c = mesh.center;
		const float x = c[0] * m._11 + c[1] * m._21 + c[2] * m._31 + m._41 - point.x;
		const float y = c[0] * m._12 + c[1] * m._22 + c[2] * m._32 + m._42 - point.y;
		const float z = c[0] * m._13 + c[1] * m._23 + c[2] * m._33 + m._43 - point.z;
		return std::sqrt( x * x + y * y + z * z );
	}

	/// Furthest the mesh reaches from the instance's origin, so how far a point on it moves when the instance turns by one radian.
	float Extent( const AudGeometryManager::Mesh& mesh, float largestScale )
	{
		const float* c = mesh.center;
		return ( std::sqrt( c[0] * c[0] + c[1] * c[1] + c[2] * c[2] ) + mesh.radius ) * largestScale;
	}

	AkGeometryInstanceParams MakeInstanceParams( uint64_t geometrySetId, const Matrix& worldTransform, bool solid, bool portalSubtraction )
	{
		AkTransform transform;
		RH2LH::convertTransform( worldTransform, transform );

		AkGeometryInstanceParams params;
		params.PositionAndOrientation = transform;
		params.Scale = RH2LH::extractScale( worldTransform );
		// A rotation can't mirror, so one scale axis carries the flip.
		if( Determinant( worldTransform ) < 0.0f )
		{
			params.Scale.X = -params.Scale.X;
		}
		params.GeometrySetID = geometrySetId;
		params.IsSolid = solid;
		// Wwise re-cuts every mesh a portal overlaps each time it rebuilds portal visibility, on the audio thread.
		params.BypassPortalSubtraction = !portalSubtraction;
		return params;
	}

	bool NearlyEqual( const AkVector& a, const AkVector& b, float epsilon )
	{
		return std::fabs( a.X - b.X ) <= epsilon && std::fabs( a.Y - b.Y ) <= epsilon && std::fabs( a.Z - b.Z ) <= epsilon;
	}

	bool SameInstanceParams( const AkGeometryInstanceParams& a, const AkGeometryInstanceParams& b )
	{
		constexpr AkReal64 positionEpsilon = 1e-3;
		constexpr float orientationEpsilon = 1e-4f;
		constexpr float scaleEpsilon = 1e-4f;

		const AkVector64 pa = a.PositionAndOrientation.Position();
		const AkVector64 pb = b.PositionAndOrientation.Position();
		const float scaleTolerance = scaleEpsilon * std::max( { 1.0f, std::fabs( a.Scale.X ), std::fabs( a.Scale.Y ), std::fabs( a.Scale.Z ) } );

		return a.GeometrySetID == b.GeometrySetID
			&& a.IsSolid == b.IsSolid
			&& a.BypassPortalSubtraction == b.BypassPortalSubtraction
			&& std::fabs( pa.X - pb.X ) <= positionEpsilon
			&& std::fabs( pa.Y - pb.Y ) <= positionEpsilon
			&& std::fabs( pa.Z - pb.Z ) <= positionEpsilon
			&& NearlyEqual( a.PositionAndOrientation.OrientationFront(), b.PositionAndOrientation.OrientationFront(), orientationEpsilon )
			&& NearlyEqual( a.PositionAndOrientation.OrientationTop(), b.PositionAndOrientation.OrientationTop(), orientationEpsilon )
			&& NearlyEqual( a.Scale, b.Scale, scaleTolerance );
	}

	float Chord( const AkVector& a, const AkVector& b )
	{
		const float x = a.X - b.X;
		const float y = a.Y - b.Y;
		const float z = a.Z - b.Z;
		return std::sqrt( x * x + y * y + z * z );
	}

	/**
	 * @brief Whether an instance at a is near enough to where Wwise has it, at b, that sending it makes no audible difference.
	 *
	 * @param extent Furthest the mesh reaches from the instance's origin, to turn rotation into distance.
	 * @param tolerance Furthest any point of the mesh may be from where Wwise has it.
	 */
	bool CloseEnough( const AkGeometryInstanceParams& a, const AkGeometryInstanceParams& b, float extent, double tolerance )
	{
		if( a.GeometrySetID != b.GeometrySetID || a.IsSolid != b.IsSolid || a.BypassPortalSubtraction != b.BypassPortalSubtraction
			|| ( a.Scale.X < 0.0f ) != ( b.Scale.X < 0.0f ) )
		{
			return false;
		}

		const AkVector64 d = a.PositionAndOrientation.Position() - b.PositionAndOrientation.Position();
		const double moved = std::sqrt( d.X * d.X + d.Y * d.Y + d.Z * d.Z );
		// For a small turn the chord between the axes is the angle.
		const double turned = std::max( Chord( a.PositionAndOrientation.OrientationFront(), b.PositionAndOrientation.OrientationFront() ),
			Chord( a.PositionAndOrientation.OrientationTop(), b.PositionAndOrientation.OrientationTop() ) );
		const auto relative = []( float x, float y ) { return std::fabs( x - y ) / std::max( std::fabs( y ), 1.0e-6f ); };
		const double scaled = std::max( { relative( a.Scale.X, b.Scale.X ), relative( a.Scale.Y, b.Scale.Y ), relative( a.Scale.Z, b.Scale.Z ) } );
		return moved + ( turned + scaled ) * extent <= tolerance;
	}
}

AudGeometryManager::AudGeometryManager() :
	m_transmissionLoss( 0.0f ),
	m_enableDiffraction( false ),
	m_enableDiffractionOnBoundaryEdges( false ),
	m_solid( false ),
	m_portalSubtraction( false ),
	m_wwiseOrigin(),
	m_statsDirty( false ),
	m_instanceSends( 0 ),
	m_setsInWwise( 0 ),
	m_instancesInWwise( 0 ),
	m_trianglesInWwise( 0 ),
	m_instancesRegistered( 0 ),
	m_mutex( "AudGeometryManager", "m_mutex" )
{
}

AudGeometryManager& AudGeometryManager::Get()
{
	static AudGeometryManager instance;
	return instance;
}

AudGeometryManager::Mesh AudGeometryManager::BuildMesh( const Tr2AudGeometryData& geometryData )
{
	Mesh mesh;

	const size_t vertexCount = geometryData.m_vertices.size();
	std::vector<uint32_t> remap( vertexCount, UNMAPPED );
	std::unordered_map<AkVertex, uint32_t, PositionHash, PositionEqual> welded;
	welded.reserve( vertexCount );

	for( size_t i = 0; i + 2 < geometryData.m_indices.size(); i += 3 )
	{
		uint32_t corners[3];
		bool valid = true;
		for( int c = 0; c < 3; ++c )
		{
			const uint32_t source = geometryData.m_indices[i + c];
			if( source >= vertexCount )
			{
				valid = false;
				break;
			}

			if( remap[source] == UNMAPPED )
			{
				const Vector3& v = geometryData.m_vertices[source];
				if( !std::isfinite( v.x ) || !std::isfinite( v.y ) || !std::isfinite( v.z ) )
				{
					valid = false;
					break;
				}

				// Adding zero turns -0 into +0, so both weld.
				const AkVertex position( v.x + 0.0f, v.y + 0.0f, -v.z + 0.0f );
				const auto inserted = welded.try_emplace( position, static_cast<uint32_t>( mesh.vertices.size() ) );
				if( inserted.second )
				{
					mesh.vertices.push_back( position );
				}
				remap[source] = inserted.first->second;
			}
			corners[c] = remap[source];
		}

		if( !valid || corners[0] == corners[1] || corners[1] == corners[2] || corners[0] == corners[2] )
		{
			continue;
		}

		mesh.triangles.emplace_back(
			static_cast<AkVertIdx>( corners[0] ),
			static_cast<AkVertIdx>( corners[1] ),
			static_cast<AkVertIdx>( corners[2] ),
			static_cast<AkSurfIdx>( 0 ) );
	}

	if( mesh.triangles.empty() || mesh.vertices.size() > MAX_ELEMENTS || mesh.triangles.size() > MAX_ELEMENTS )
	{
		return {};
	}

	AkVertex low = mesh.vertices.front();
	AkVertex high = low;
	for( const AkVertex& v : mesh.vertices )
	{
		low = AkVertex( std::min( low.X, v.X ), std::min( low.Y, v.Y ), std::min( low.Z, v.Z ) );
		high = AkVertex( std::max( high.X, v.X ), std::max( high.Y, v.Y ), std::max( high.Z, v.Z ) );
	}
	const AkVertex center( ( low.X + high.X ) * 0.5f, ( low.Y + high.Y ) * 0.5f, ( low.Z + high.Z ) * 0.5f );
	float radiusSquared = 0.0f;
	for( const AkVertex& v : mesh.vertices )
	{
		const float x = v.X - center.X;
		const float y = v.Y - center.Y;
		const float z = v.Z - center.Z;
		radiusSquared = std::max( radiusSquared, x * x + y * y + z * z );
	}
	mesh.center[0] = center.X;
	mesh.center[1] = center.Y;
	mesh.center[2] = -center.Z;
	mesh.radius = std::sqrt( radiusSquared );
	return mesh;
}

void AudGeometryManager::SetGeometry( uint64_t geometrySetId, uint64_t instanceId, const Tr2AudGeometryData& geometryData, const Matrix& worldTransform )
{
	bool known;
	{
		CcpAutoMutex lock( m_mutex );
		known = m_sets.find( geometrySetId ) != m_sets.end();
	}

	const auto build = [&]()
	{
		Mesh mesh = BuildMesh( geometryData );
		if( mesh.triangles.empty() )
		{
			CCP_LOGWARN_CH( s_ch, "Geometry set %llu is not usable as audio geometry (%zu vertices, %zu indices). Wwise takes at most %zu vertices and triangles per set.",
				geometrySetId, geometryData.m_vertices.size(), geometryData.m_indices.size(), MAX_ELEMENTS );
		}
		return mesh;
	};

	// Welding is the expensive part, so it runs outside the lock.
	Mesh mesh;
	if( !known )
	{
		mesh = build();
	}

	CcpAutoMutex lock( m_mutex );

	const auto setEntry = m_sets.try_emplace( geometrySetId );
	if( setEntry.second )
	{
		// The set can lose its last instance while the lock is released.
		setEntry.first->second.mesh = known ? build() : std::move( mesh );
	}

	const auto instanceEntry = m_instances.try_emplace( instanceId );
	Instance& instance = instanceEntry.first->second;
	if( instanceEntry.second )
	{
		++setEntry.first->second.instanceCount;
	}
	else if( instance.geometrySetId != geometrySetId )
	{
		++setEntry.first->second.instanceCount;
		if( instance.inWwise )
		{
			m_instancesToRemove.push_back( instanceId );
			instance.inWwise = false;
			m_statsDirty = true;
		}
		ReleaseSetLocked( instance.geometrySetId );
	}

	const AudWorldFrame::State frame = AudWorldFrame::Get().GetState();
	instance.geometrySetId = geometrySetId;
	instance.transform = worldTransform;
	instance.position = frame.sceneOrigin + Translation( worldTransform );
	instance.positionBeforeOrigin = !frame.hasSceneOrigin;
	instance.dirty = true;
}

void AudGeometryManager::SetGeometryTransform( uint64_t instanceId, const Matrix& worldTransform )
{
	const AudWorldFrame::State frame = AudWorldFrame::Get().GetState();

	CcpAutoMutex lock( m_mutex );

	const auto it = m_instances.find( instanceId );
	if( it == m_instances.end() )
	{
		return;
	}

	Instance& instance = it->second;
	ResolvePosition( instance, frame );

	// The scene origin follows the player, so the transform changes whenever the player moves. Only a move in the world counts.
	const AkVector64 position = frame.sceneOrigin + Translation( worldTransform );
	const bool moved = !SameRotationAndScale( instance.transform, worldTransform )
		|| MovedInWorld( instance.position, position, worldTransform );

	// Kept either way, it says whether trinity parked the instance.
	instance.transform = worldTransform;
	if( moved )
	{
		instance.position = position;
		instance.positionBeforeOrigin = !frame.hasSceneOrigin;
		instance.dirty = true;
	}
}

void AudGeometryManager::ResolvePosition( Instance& instance, const AudWorldFrame::State& frame )
{
	if( instance.positionBeforeOrigin && frame.hasSceneOrigin )
	{
		instance.position = instance.position + frame.firstSceneOrigin;
		instance.positionBeforeOrigin = false;
		instance.dirty = true;
	}
}

Matrix AudGeometryManager::InWwiseFrame( const Instance& instance, const AkVector64& wwiseOrigin )
{
	const AkVector64 translation = instance.position - wwiseOrigin;
	Matrix m = instance.transform;
	m._41 = static_cast<float>( translation.X );
	m._42 = static_cast<float>( translation.Y );
	m._43 = static_cast<float>( translation.Z );
	return m;
}

void AudGeometryManager::RemoveGeometry( uint64_t instanceId )
{
	CcpAutoMutex lock( m_mutex );

	const auto it = m_instances.find( instanceId );
	if( it == m_instances.end() )
	{
		return;
	}

	if( it->second.inWwise )
	{
		m_instancesToRemove.push_back( instanceId );
		m_statsDirty = true;
	}

	const uint64_t ownSetId = it->second.geometrySetId;
	m_instances.erase( it );
	ReleaseSetLocked( ownSetId );
}

void AudGeometryManager::ReleaseSetLocked( uint64_t geometrySetId )
{
	const auto it = m_sets.find( geometrySetId );
	if( it == m_sets.end() )
	{
		return;
	}

	if( --it->second.instanceCount > 0 )
	{
		return;
	}

	if( it->second.inWwise )
	{
		m_setsToRemove.push_back( geometrySetId );
		m_statsDirty = true;
	}
	m_sets.erase( it );
}

bool AudGeometryManager::SendSetLocked( uint64_t geometrySetId, GeometrySet& set )
{
	AkAcousticSurface surface;
	surface.strName = "trinity";
	surface.transmissionLoss = m_transmissionLoss;

	AkGeometryParams params;
	params.Vertices = set.mesh.vertices.data();
	params.NumVertices = static_cast<AkVertIdx>( set.mesh.vertices.size() );
	params.Triangles = set.mesh.triangles.data();
	params.NumTriangles = static_cast<AkTriIdx>( set.mesh.triangles.size() );
	params.Surfaces = &surface;
	params.NumSurfaces = 1;
	params.EnableDiffraction = m_enableDiffraction;
	params.EnableDiffractionOnBoundaryEdges = m_enableDiffractionOnBoundaryEdges;

	const AKRESULT result = AK::SpatialAudio::SetGeometry( geometrySetId, params );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set geometry set %llu (%zu vertices, %zu triangles), AKRESULT: %d",
			geometrySetId, set.mesh.vertices.size(), set.mesh.triangles.size(), result );
		set.rejected = true;
		return false;
	}

	if( !set.inWwise )
	{
		set.inWwise = true;
		m_statsDirty = true;
	}
	return true;
}

void AudGeometryManager::SelectInstancesLocked( const SpatialAudioSettings& settings, const AkVector64& wwiseOrigin, const Vector3* listenerInWwise )
{
	const float maxDistance = settings.GetGeometryMaxDistance();
	const size_t maxTriangles = static_cast<size_t>( settings.GetGeometryMaxTriangles() );

	m_candidates.clear();
	for( auto& entry : m_instances )
	{
		Instance& instance = entry.second;
		instance.wanted = false;

		if( instance.rejected && !instance.dirty )
		{
			continue;
		}

		const auto setIt = m_sets.find( instance.geometrySetId );
		if( setIt == m_sets.end() || setIt->second.mesh.triangles.empty() || setIt->second.rejected )
		{
			continue;
		}

		float largestScale;
		if( !IsUsableTransform( instance.transform, largestScale ) )
		{
			continue;
		}

		const Mesh& mesh = setIt->second.mesh;
		instance.extent = Extent( mesh, largestScale );
		instance.listenerDistance = -1.0f;
		float distance = 0.0f;
		if( listenerInWwise )
		{
			instance.listenerDistance = DistanceToCenter( InWwiseFrame( instance, wwiseOrigin ), mesh, *listenerInWwise );
			distance = std::max( 0.0f, instance.listenerDistance - mesh.radius * largestScale );
			if( maxDistance > 0.0f && distance > maxDistance * ( instance.inWwise ? KEEP_DISTANCE : 1.0f ) )
			{
				continue;
			}
		}

		m_candidates.push_back( { instance.inWwise ? distance * KEEP_RANK : distance, static_cast<uint32_t>( mesh.triangles.size() ), entry.first, &instance, &setIt->second } );
	}

	if( maxTriangles == 0 )
	{
		for( const Candidate& candidate : m_candidates )
		{
			candidate.instance->wanted = true;
		}
		return;
	}

	std::sort( m_candidates.begin(), m_candidates.end(), []( const Candidate& a, const Candidate& b )
	{
		if( a.distance != b.distance )
		{
			return a.distance < b.distance;
		}
		if( a.instance->inWwise != b.instance->inWwise )
		{
			return a.instance->inWwise;
		}
		return a.instanceId < b.instanceId;
	} );

	// A mesh that doesn't fit is skipped, so one big mesh doesn't keep the smaller ones behind it out.
	size_t triangles = 0;
	for( const Candidate& candidate : m_candidates )
	{
		if( triangles + candidate.triangles <= maxTriangles )
		{
			triangles += candidate.triangles;
			candidate.instance->wanted = true;
		}
		else if( candidate.triangles > maxTriangles && !candidate.set->budgetWarned )
		{
			candidate.set->budgetWarned = true;
			CCP_LOGWARN_CH( s_ch, "Geometry set %llu has %u triangles, more than geometryMaxTriangles (%zu), so it stays out of Wwise.",
				candidate.instance->geometrySetId, candidate.triangles, maxTriangles );
		}
	}
}

void AudGeometryManager::Update( const SpatialAudioSettings& settings, const Vector3* listenerPosition )
{
	CcpAutoMutex lock( m_mutex );

	// A set takes its instances with it, so instances go first.
	bool setsMayBeUnused = !m_instancesToRemove.empty();
	for( const uint64_t instanceId : m_instancesToRemove )
	{
		AK::SpatialAudio::RemoveGeometryInstance( instanceId );
	}
	m_instancesToRemove.clear();

	for( const uint64_t geometrySetId : m_setsToRemove )
	{
		AK::SpatialAudio::RemoveGeometry( geometrySetId );
	}
	m_setsToRemove.clear();

	if( m_transmissionLoss != settings.GetTransmissionLoss()
		|| m_enableDiffraction != settings.GetEnableDiffraction()
		|| m_enableDiffractionOnBoundaryEdges != settings.GetEnableDiffractionOnBoundaryEdges() )
	{
		m_transmissionLoss = settings.GetTransmissionLoss();
		m_enableDiffraction = settings.GetEnableDiffraction();
		m_enableDiffractionOnBoundaryEdges = settings.GetEnableDiffractionOnBoundaryEdges();
		for( auto& entry : m_sets )
		{
			if( entry.second.inWwise )
			{
				SendSetLocked( entry.first, entry.second );
			}
		}
	}

	const bool instanceSettingsChanged = m_solid != settings.GetGeometrySolid()
		|| m_portalSubtraction != settings.GetGeometryPortalSubtraction();
	m_solid = settings.GetGeometrySolid();
	m_portalSubtraction = settings.GetGeometryPortalSubtraction();

	const float updateTolerance = settings.GetGeometryUpdateTolerance();

	// Every instance in Wwise is placed from the Wwise origin, so they all go again when it moves.
	const AudWorldFrame::State frame = AudWorldFrame::Get().GetState();
	const bool wwiseOriginMoved = !SameVector64( frame.wwiseOrigin, m_wwiseOrigin );
	m_wwiseOrigin = frame.wwiseOrigin;
	for( auto& entry : m_instances )
	{
		ResolvePosition( entry.second, frame );
	}

	Vector3 listenerInWwise;
	if( listenerPosition )
	{
		const AkVector64 sceneToWwise = frame.sceneOrigin - frame.wwiseOrigin;
		listenerInWwise = Vector3(
			static_cast<float>( listenerPosition->x + sceneToWwise.X ),
			static_cast<float>( listenerPosition->y + sceneToWwise.Y ),
			static_cast<float>( listenerPosition->z + sceneToWwise.Z ) );
	}

	SelectInstancesLocked( settings, frame.wwiseOrigin, listenerPosition ? &listenerInWwise : nullptr );

	const auto removeFromWwise = [&]( uint64_t instanceId, Instance& instance )
	{
		if( instance.inWwise )
		{
			AK::SpatialAudio::RemoveGeometryInstance( instanceId );
			instance.inWwise = false;
			setsMayBeUnused = true;
			m_statsDirty = true;
		}
	};

	for( auto& entry : m_instances )
	{
		Instance& instance = entry.second;
		if( !instance.wanted )
		{
			removeFromWwise( entry.first, instance );
			continue;
		}

		const bool dirty = instance.dirty || instance.heldBack || instanceSettingsChanged || wwiseOriginMoved;
		instance.dirty = false;
		if( instance.inWwise && !dirty )
		{
			continue;
		}

		GeometrySet& set = m_sets.find( instance.geometrySetId )->second;
		if( set.rejected || ( !set.inWwise && !SendSetLocked( instance.geometrySetId, set ) ) )
		{
			continue;
		}

		const AkGeometryInstanceParams params = MakeInstanceParams( instance.geometrySetId, InWwiseFrame( instance, frame.wwiseOrigin ), m_solid, m_portalSubtraction );
		if( instance.inWwise )
		{
			if( SameInstanceParams( params, instance.sent ) )
			{
				instance.heldBack = false;
				continue;
			}

			// Moving the Wwise origin or changing a setting leaves every instance in Wwise off, near or far.
			if( !wwiseOriginMoved && !instanceSettingsChanged )
			{
				const double tolerance = instance.listenerDistance < 0.0f
					? STILL_IN_WWISE
					: std::max( STILL_IN_WWISE, static_cast<double>( updateTolerance ) * instance.listenerDistance );
				if( CloseEnough( params, instance.sent, instance.extent, tolerance ) )
				{
					// Looked at again every update, the listener may come close enough to hear the difference.
					instance.heldBack = true;
					continue;
				}
			}
		}

		const AKRESULT result = AK::SpatialAudio::SetGeometryInstance( entry.first, params );
		++m_instanceSends;
		++instance.sends;
		instance.heldBack = false;
		if( result != AK_Success )
		{
			if( !instance.rejected )
			{
				CCP_LOGERR_CH( s_ch, "Failed to set geometry instance %llu (set %llu), AKRESULT: %d", entry.first, instance.geometrySetId, result );
				instance.rejected = true;
			}
			removeFromWwise( entry.first, instance );
			// The set may have been sent for this instance alone.
			setsMayBeUnused = true;
			continue;
		}

		instance.rejected = false;
		instance.sent = params;
		if( !instance.inWwise )
		{
			instance.inWwise = true;
			m_statsDirty = true;
		}
	}

	if( setsMayBeUnused )
	{
		RemoveUnusedSetsLocked();
	}

	m_instancesRegistered = m_instances.size();
	UpdateStatsLocked();
}

void AudGeometryManager::RemoveUnusedSetsLocked()
{
	m_setsInUse.clear();
	for( const auto& entry : m_instances )
	{
		if( entry.second.inWwise )
		{
			m_setsInUse.insert( entry.second.geometrySetId );
		}
	}

	for( auto& entry : m_sets )
	{
		if( entry.second.inWwise && m_setsInUse.find( entry.first ) == m_setsInUse.end() )
		{
			AK::SpatialAudio::RemoveGeometry( entry.first );
			entry.second.inWwise = false;
			m_statsDirty = true;
		}
	}
}

void AudGeometryManager::RemoveAllFromWwise()
{
	CcpAutoMutex lock( m_mutex );

	if( AK::SoundEngine::IsInitialized() )
	{
		for( const uint64_t geometrySetId : m_setsToRemove )
		{
			AK::SpatialAudio::RemoveGeometry( geometrySetId );
		}
		for( const auto& entry : m_sets )
		{
			if( entry.second.inWwise )
			{
				AK::SpatialAudio::RemoveGeometry( entry.first );
			}
		}
	}

	ForgetWwiseStateLocked();
}

void AudGeometryManager::ForgetWwiseState()
{
	CcpAutoMutex lock( m_mutex );
	ForgetWwiseStateLocked();
}

void AudGeometryManager::ForgetWwiseStateLocked()
{
	m_instancesToRemove.clear();
	m_setsToRemove.clear();

	for( auto& entry : m_sets )
	{
		entry.second.inWwise = false;
		entry.second.rejected = false;
	}
	for( auto& entry : m_instances )
	{
		entry.second.inWwise = false;
		entry.second.rejected = false;
		entry.second.dirty = true;
	}

	m_statsDirty = true;
	UpdateStatsLocked();
}

std::string AudGeometryManager::GetSendReport( size_t count ) const
{
	CcpAutoMutex lock( m_mutex );

	std::vector<std::pair<uint64_t, const Instance*>> instances;
	instances.reserve( m_instances.size() );
	for( const auto& entry : m_instances )
	{
		instances.emplace_back( entry.first, &entry.second );
	}
	std::sort( instances.begin(), instances.end(), []( const auto& a, const auto& b )
	{
		return a.second->sends != b.second->sends ? a.second->sends > b.second->sends : a.first < b.first;
	} );

	std::string report;
	char line[256];
	for( size_t i = 0; i < std::min( count, instances.size() ); ++i )
	{
		const Instance& instance = *instances[i].second;
		const auto setIt = m_sets.find( instance.geometrySetId );
		const size_t triangles = setIt != m_sets.end() ? setIt->second.mesh.triangles.size() : 0;
		snprintf( line, sizeof( line ), "instance %" PRIu64 " set %" PRIu64 " triangles %zu sends %" PRIu64 " listener %.0f extent %.0f%s%s\n",
			instances[i].first, instance.geometrySetId, triangles, instance.sends, instance.listenerDistance, instance.extent,
			instance.inWwise ? "" : " (not in Wwise)", instance.heldBack ? " (held back)" : "" );
		report += line;
	}
	return report;
}

void AudGeometryManager::UpdateStatsLocked()
{
	if( !m_statsDirty )
	{
		return;
	}
	m_statsDirty = false;

	size_t sets = 0;
	for( const auto& entry : m_sets )
	{
		sets += entry.second.inWwise ? 1 : 0;
	}

	size_t instances = 0;
	size_t triangles = 0;
	for( const auto& entry : m_instances )
	{
		if( !entry.second.inWwise )
		{
			continue;
		}
		++instances;
		const auto setIt = m_sets.find( entry.second.geometrySetId );
		if( setIt != m_sets.end() )
		{
			triangles += setIt->second.mesh.triangles.size();
		}
	}

	m_setsInWwise = sets;
	m_instancesInWwise = instances;
	m_trianglesInWwise = triangles;

	CCP_STATS_SET( geometrySets, sets );
	CCP_STATS_SET( geometryInstances, instances );
	CCP_STATS_SET( geometryTriangles, triangles );
}
