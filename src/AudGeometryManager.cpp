////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudGeometryManager.h"

#include "AudWorldFrame.h"
#include "SpatialAudioSettings.h"
#include "Utilities.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
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

	bool SameRotationAndScale( const Matrix& a, const Matrix& b )
	{
		return a._11 == b._11 && a._12 == b._12 && a._13 == b._13
			&& a._21 == b._21 && a._22 == b._22 && a._23 == b._23
			&& a._31 == b._31 && a._32 == b._32 && a._33 == b._33;
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

	/// Distance from a point to the instance's bounding sphere, 0 inside it.
	float DistanceToBounds( const Matrix& m, const AudGeometryManager::Mesh& mesh, float largestScale, const Vector3& point )
	{
		const float* c = mesh.center;
		const float x = c[0] * m._11 + c[1] * m._21 + c[2] * m._31 + m._41 - point.x;
		const float y = c[0] * m._12 + c[1] * m._22 + c[2] * m._32 + m._42 - point.y;
		const float z = c[0] * m._13 + c[1] * m._23 + c[2] * m._33 + m._43 - point.z;
		return std::max( 0.0f, std::sqrt( x * x + y * y + z * z ) - mesh.radius * largestScale );
	}

	/// Furthest the mesh reaches from the instance's origin, so how far a point on it moves when the instance turns by one radian.
	float Extent( const AudGeometryManager::Mesh& mesh, float largestScale )
	{
		const float* c = mesh.center;
		return ( std::sqrt( c[0] * c[0] + c[1] * c[1] + c[2] * c[2] ) + mesh.radius ) * largestScale;
	}

	/// sceneRelative placed in Wwise's frame by adding sceneToWwise.
	AkGeometryInstanceParams MakeInstanceParams( uint64_t geometrySetId, const Matrix& sceneRelative, const AkVector64& sceneToWwise, bool solid, bool portalSubtraction )
	{
		AkGeometryInstanceParams params;
		params.PositionAndOrientation = RH2LH::convertTransform( sceneRelative, sceneToWwise );
		params.Scale = RH2LH::extractScale( sceneRelative );
		// A rotation can't mirror, so one scale axis carries the flip.
		if( Determinant( sceneRelative ) < 0.0f )
		{
			params.Scale.X = -params.Scale.X;
		}
		params.GeometrySetID = geometrySetId;
		params.IsSolid = solid;
		// Wwise re-cuts every mesh a portal overlaps each time it rebuilds portal visibility, on the audio thread.
		params.BypassPortalSubtraction = !portalSubtraction;
		return params;
	}

	/**
	 * @brief How far any point of the mesh placed by a is from where b puts it.
	 *
	 * @param extent Furthest the mesh reaches from the instance's origin, to turn rotation and scale into distance.
	 * @return Infinity when they differ in something only sending again changes.
	 */
	double Mismatch( const AkGeometryInstanceParams& a, const AkGeometryInstanceParams& b, float extent )
	{
		if( a.GeometrySetID != b.GeometrySetID || a.IsSolid != b.IsSolid || a.BypassPortalSubtraction != b.BypassPortalSubtraction
			|| ( a.Scale.X < 0.0f ) != ( b.Scale.X < 0.0f ) )
		{
			return std::numeric_limits<double>::infinity();
		}

		const double moved = ( Ak3DVector64( a.PositionAndOrientation.Position() ) - Ak3DVector64( b.PositionAndOrientation.Position() ) ).Length();
		// For a small turn the chord between the axes is the angle.
		const auto chord = []( const AkVector& x, const AkVector& y ) { return ( Ak3DVector32( x ) - Ak3DVector32( y ) ).Length(); };
		const double turned = std::max( chord( a.PositionAndOrientation.OrientationFront(), b.PositionAndOrientation.OrientationFront() ),
			chord( a.PositionAndOrientation.OrientationTop(), b.PositionAndOrientation.OrientationTop() ) );
		const auto relative = []( float x, float y ) { return std::fabs( x - y ) / std::max( std::fabs( y ), 1.0e-6f ); };
		const double scaled = std::max( { relative( a.Scale.X, b.Scale.X ), relative( a.Scale.Y, b.Scale.Y ), relative( a.Scale.Z, b.Scale.Z ) } );
		return moved + ( turned + scaled ) * extent;
	}
}

AudGeometryManager::AudGeometryManager() :
	m_transmissionLoss( 0.0f ),
	m_enableDiffraction( false ),
	m_enableDiffractionOnBoundaryEdges( false ),
	m_solid( false ),
	m_portalSubtraction( false ),
	m_placedOffsetGeneration( 0 ),
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

	instance.geometrySetId = geometrySetId;
	instance.transform = worldTransform;
	instance.dirty = true;
}

void AudGeometryManager::SetGeometryTransform( uint64_t instanceId, const Matrix& worldTransform )
{
	CcpAutoMutex lock( m_mutex );

	const auto it = m_instances.find( instanceId );
	if( it == m_instances.end() )
	{
		return;
	}

	// The scene origin follows the player, so this changes whenever the player moves. Update() works out
	// whether the instance moved in the world.
	Instance& instance = it->second;
	if( std::memcmp( &instance.transform, &worldTransform, sizeof( Matrix ) ) != 0 )
	{
		instance.transform = worldTransform;
		instance.dirty = true;
	}
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

void AudGeometryManager::SelectInstancesLocked( const SpatialAudioSettings& settings, const Vector3* listenerPosition )
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
		if( listenerPosition )
		{
			// Both are relative to the scene origin, which is as precise as it gets near the player.
			distance = DistanceToBounds( instance.transform, mesh, largestScale, *listenerPosition );
			instance.listenerDistance = distance;
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

	// An instance trinity didn't hand over moved along with the scene origin, so every instance is looked at
	// when the offset changes. Everything in Wwise goes again when the Wwise origin jumps.
	const AudWorldFrame::Offset offset = AudWorldFrame::Get().GetOffset();
	const bool offsetChanged = offset.generation != m_placedOffsetGeneration;
	m_placedOffsetGeneration = offset.generation;

	SelectInstancesLocked( settings, listenerPosition );

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

		if( instance.inWwise && !instance.dirty && !offsetChanged && !instanceSettingsChanged )
		{
			continue;
		}
		instance.dirty = false;

		// Wwise has it if it is within float noise of where it was sent. An instance that holds still in the world
		// only shifts with the offset, which takes a few additions to rule out.
		const double still = AudWorldFrame::StillDistance( instance.transform );
		if( instance.inWwise && !instanceSettingsChanged && SameRotationAndScale( instance.transform, instance.sentTransform )
			&& ( Ak3DVector64( RH2LH::convertPosition( instance.transform, offset.sceneToWwise ) )
				- Ak3DVector64( instance.sent.PositionAndOrientation.Position() ) ).Length() <= still )
		{
			continue;
		}

		// Checked before the set is sent, so a refused instance doesn't put its set in Wwise for nothing.
		const AkGeometryInstanceParams params = MakeInstanceParams( instance.geometrySetId, instance.transform, offset.sceneToWwise, m_solid, m_portalSubtraction );
		if( instance.inWwise || instance.rejected )
		{
			// Wwise has it, or refused it as it is.
			if( Mismatch( params, instance.sent, instance.extent ) <= still )
			{
				continue;
			}
		}

		GeometrySet& set = m_sets.find( instance.geometrySetId )->second;
		if( set.rejected || ( !set.inWwise && !SendSetLocked( instance.geometrySetId, set ) ) )
		{
			continue;
		}

		const AKRESULT result = AK::SpatialAudio::SetGeometryInstance( entry.first, params );
		++m_instanceSends;
		++instance.sends;
		instance.sent = params;
		instance.sentTransform = instance.transform;
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
		snprintf( line, sizeof( line ), "instance %" PRIu64 " set %" PRIu64 " triangles %zu sends %" PRIu64 " listener %.0f extent %.0f%s\n",
			instances[i].first, instance.geometrySetId, triangles, instance.sends, instance.listenerDistance, instance.extent,
			instance.inWwise ? "" : " (not in Wwise)" );
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
