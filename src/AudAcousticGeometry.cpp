////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudAcousticGeometry.h"

#include "AudWorldFrame.h"
#include "SpatialAudioSettings.h"
#include "Utilities.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SpatialAudio/Common/AkSpatialAudio.h>

#include <algorithm>
#include <cmath>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudAcousticGeometry" );

CCP_STATS_DECLARE( acousticSets, "CarbonAudio/AudAcousticGeometry/Sets", false, CST_COUNTER_LOW, "Acoustic geometry sets in Wwise." );
CCP_STATS_DECLARE( acousticInstances, "CarbonAudio/AudAcousticGeometry/Instances", false, CST_COUNTER_LOW, "Acoustic geometry instances in Wwise." );
CCP_STATS_DECLARE( acousticTriangles, "CarbonAudio/AudAcousticGeometry/Triangles", false, CST_COUNTER_LOW, "Triangles of every acoustic geometry instance in Wwise." );

namespace
{
	/// Tags the set and instance IDs of this class, apart from trinity's small ones and the rooms' bit 62.
	constexpr uint64_t ID_TAG = 1ull << 61;
	/// Wwise indexes vertices and triangles with 16 bits and reserves the last value.
	constexpr size_t MAX_ELEMENTS = 0xFFFF;
	constexpr auto SELECTION_INTERVAL = std::chrono::milliseconds( 250 );
	constexpr auto FETCH_BUDGET = std::chrono::milliseconds( 1 );
	/// A component already in Wwise stays until it is this much further out, and ranks this much nearer.
	constexpr double KEEP_DISTANCE = 1.1;
	constexpr double KEEP_RANK_SQUARED = 0.81;
	/// A body moved when a point on it moved more than this, in metres.
	constexpr double MOVE_TOLERANCE = 1e-3;
	/// Sets whose instances all left stay in Wwise up to this many times the triangle budget.
	constexpr size_t UNUSED_TRIANGLES_PER_BUDGET = 8;

	bool IsFinite( const double* v, int count )
	{
		for( int i = 0; i < count; ++i )
		{
			if( !std::isfinite( v[i] ) )
			{
				return false;
			}
		}
		return true;
	}

	/// q * v * q^-1 for a unit quaternion x y z w.
	void Rotate( const double q[4], const double v[3], double out[3] )
	{
		const double x = q[0], y = q[1], z = q[2], w = q[3];
		const double cx = y * v[2] - z * v[1];
		const double cy = z * v[0] - x * v[2];
		const double cz = x * v[1] - y * v[0];
		out[0] = v[0] + 2.0 * ( w * cx + y * cz - z * cy );
		out[1] = v[1] + 2.0 * ( w * cy + z * cx - x * cz );
		out[2] = v[2] + 2.0 * ( w * cz + x * cy - y * cx );
	}

	/// a * b, so b is applied first.
	void Multiply( const double a[4], const double b[4], double out[4] )
	{
		out[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
		out[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
		out[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
		out[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	}

	void Normalize( double q[4] )
	{
		const double length = std::sqrt( q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3] );
		if( length > 0.0 && std::isfinite( length ) )
		{
			for( int i = 0; i < 4; ++i )
			{
				q[i] /= length;
			}
		}
		else
		{
			q[0] = q[1] = q[2] = 0.0;
			q[3] = 1.0;
		}
	}

	/// Whether a point radius from the pivot moved more than the tolerance between two poses.
	bool PoseMoved( const double positionA[3], const double rotationA[4], const double positionB[3], const double rotationB[4], double radius )
	{
		const double dx = positionA[0] - positionB[0];
		const double dy = positionA[1] - positionB[1];
		const double dz = positionA[2] - positionB[2];
		if( dx * dx + dy * dy + dz * dz > MOVE_TOLERANCE * MOVE_TOLERANCE )
		{
			return true;
		}

		const double dot = std::fabs( rotationA[0] * rotationB[0] + rotationA[1] * rotationB[1] + rotationA[2] * rotationB[2] + rotationA[3] * rotationB[3] );
		const double angle = 2.0 * std::acos( std::min( 1.0, dot ) );
		return angle * std::max( radius, 1.0 ) > MOVE_TOLERANCE;
	}

	/// Squared distance from a point to a box in a rotated, translated frame.
	double BoxDistanceSquared( const double position[3], const double rotation[4], const double boundsMin[3], const double boundsMax[3], const double point[3] )
	{
		const double offset[3] = { point[0] - position[0], point[1] - position[1], point[2] - position[2] };
		const double inverse[4] = { -rotation[0], -rotation[1], -rotation[2], rotation[3] };
		double local[3];
		Rotate( inverse, offset, local );

		double total = 0.0;
		for( int i = 0; i < 3; ++i )
		{
			const double outside = local[i] - std::clamp( local[i], boundsMin[i], boundsMax[i] );
			total += outside * outside;
		}
		return total;
	}

	bool SameInstanceParams( const AkGeometryInstanceParams& a, const AkGeometryInstanceParams& b )
	{
		constexpr double positionEpsilon = 1e-4;
		constexpr float orientationEpsilon = 1e-6f;
		const AkVector64 pa = a.PositionAndOrientation.Position();
		const AkVector64 pb = b.PositionAndOrientation.Position();
		const AkVector fa = a.PositionAndOrientation.OrientationFront();
		const AkVector fb = b.PositionAndOrientation.OrientationFront();
		const AkVector ua = a.PositionAndOrientation.OrientationTop();
		const AkVector ub = b.PositionAndOrientation.OrientationTop();
		return a.GeometrySetID == b.GeometrySetID
			&& a.IsSolid == b.IsSolid
			&& std::fabs( pa.X - pb.X ) <= positionEpsilon && std::fabs( pa.Y - pb.Y ) <= positionEpsilon && std::fabs( pa.Z - pb.Z ) <= positionEpsilon
			&& std::fabs( fa.X - fb.X ) <= orientationEpsilon && std::fabs( fa.Y - fb.Y ) <= orientationEpsilon && std::fabs( fa.Z - fb.Z ) <= orientationEpsilon
			&& std::fabs( ua.X - ub.X ) <= orientationEpsilon && std::fabs( ua.Y - ub.Y ) <= orientationEpsilon && std::fabs( ua.Z - ub.Z ) <= orientationEpsilon;
	}
}

AudAcousticGeometry::AudAcousticGeometry() :
	m_lastSource( nullptr ),
	m_sourceGeneration( 0 ),
	m_lastEpoch( 0 ),
	m_nextId( ID_TAG ),
	m_selections( 0 ),
	m_selectedListener{},
	m_selectedRadius( -1.0f ),
	m_selectedBudget( -1 ),
	m_selectionDirty( true ),
	m_applyDirty( false ),
	m_transmissionLoss( -1.0f ),
	m_enableDiffraction( false ),
	m_enableDiffractionOnBoundaryEdges( false ),
	m_solid( false ),
	m_setsInWwise( 0 ),
	m_instancesInWwise( 0 ),
	m_trianglesInWwise( 0 ),
	m_bodyCount( 0 ),
	m_setsSent( 0 ),
	m_instancesSent( 0 ),
	m_meshesFetched( 0 )
{
}

uint64_t AudAcousticGeometry::NextId()
{
	return ++m_nextId;
}

void AudAcousticGeometry::Update( IEveAcousticGeometrySource* source, const AudWorldFrame& frame, const SpatialAudioSettings& settings, const Vector3* listenerScene )
{
	CCP_STATS_ZONE( "AudAcousticGeometry_Update" );

	if( source == nullptr || !frame.IsActive() )
	{
		if( m_lastSource != nullptr || !m_units.empty() )
		{
			Flush();
		}
		return;
	}

	// Keys only mean something to the source and generation that gave them.
	if( source != m_lastSource || frame.GetSourceGeneration() != m_sourceGeneration )
	{
		Flush();
		m_lastSource = source;
		m_sourceGeneration = frame.GetSourceGeneration();
	}

	if( m_transmissionLoss != settings.GetTransmissionLoss()
		|| m_enableDiffraction != settings.GetEnableDiffraction()
		|| m_enableDiffractionOnBoundaryEdges != settings.GetEnableDiffractionOnBoundaryEdges() )
	{
		m_transmissionLoss = settings.GetTransmissionLoss();
		m_enableDiffraction = settings.GetEnableDiffraction();
		m_enableDiffractionOnBoundaryEdges = settings.GetEnableDiffractionOnBoundaryEdges();
		for( auto& entry : m_meshes )
		{
			for( Chunk& chunk : entry.second.chunks )
			{
				if( chunk.inWwise )
				{
					SendSet( chunk );
				}
			}
		}
	}

	if( m_solid != settings.GetGeometrySolid() )
	{
		m_solid = settings.GetGeometrySolid();
		m_applyDirty = true;
	}

	if( m_lastEpoch != frame.GetEpoch() )
	{
		m_lastEpoch = frame.GetEpoch();
		m_selectionDirty = true;
		m_applyDirty = true;
	}

	const AudWorldFrame::Offset offset = frame.GetOffset();
	double listener[3] = {};
	if( listenerScene )
	{
		listener[0] = listenerScene->x + offset.value[0];
		listener[1] = listenerScene->y + offset.value[1];
		listener[2] = listenerScene->z + offset.value[2];
	}

	const Clock::time_point now = Clock::now();
	const float radius = settings.GetGeometryMaxDistance();
	bool select = m_selectionDirty
		|| now - m_selectedAt >= SELECTION_INTERVAL
		|| radius != m_selectedRadius
		|| settings.GetGeometryMaxTriangles() != m_selectedBudget;
	if( !select && radius > 0.0f )
	{
		const double dx = listener[0] - m_selectedListener[0];
		const double dy = listener[1] - m_selectedListener[1];
		const double dz = listener[2] - m_selectedListener[2];
		select = dx * dx + dy * dy + dz * dz > 0.01 * radius * radius;
	}

	if( select )
	{
		Select( source, frame, settings, listenerScene ? listener : nullptr );
		m_selectedAt = now;
		std::copy( listener, listener + 3, m_selectedListener );
	}

	if( !m_meshesToFetch.empty() )
	{
		FetchMeshes( source, now );
	}

	if( m_applyDirty )
	{
		m_applyDirty = false;
		Apply( frame, m_solid );
		if( select )
		{
			EvictUnusedSets( static_cast<size_t>( std::max( 0, settings.GetGeometryMaxTriangles() ) ) );
		}
		UpdateStats();
	}
}

void AudAcousticGeometry::SyncBodies( IEveAcousticGeometrySource* source, const double center[3], double radius )
{
	if( m_bodyScratch.empty() )
	{
		m_bodyScratch.resize( 256 );
	}

	uint32_t total = source->GetAcousticBodies( center, radius, m_bodyScratch.data(), static_cast<uint32_t>( m_bodyScratch.size() ) );
	if( total > m_bodyScratch.size() )
	{
		m_bodyScratch.resize( total + total / 4 );
		total = source->GetAcousticBodies( center, radius, m_bodyScratch.data(), static_cast<uint32_t>( m_bodyScratch.size() ) );
	}
	total = std::min( total, static_cast<uint32_t>( m_bodyScratch.size() ) );

	for( uint32_t i = 0; i < total; ++i )
	{
		EveAcousticBody reported = m_bodyScratch[i];
		if( reported.shapeKey == 0 || !IsFinite( reported.position, 3 ) || !IsFinite( reported.rotation, 4 ) )
		{
			continue;
		}
		Normalize( reported.rotation );

		const auto inserted = m_bodies.try_emplace( reported.bodyId );
		Body& body = inserted.first->second;
		const bool fresh = inserted.second || body.shapeKey != reported.shapeKey;
		const bool moved = !fresh && PoseMoved( body.position, body.rotation, reported.position, reported.rotation, reported.boundingRadius );
		body.seen = m_selections;

		if( !fresh && !moved )
		{
			continue;
		}

		if( fresh && !inserted.second )
		{
			RemoveBodyUnits( reported.bodyId );
		}

		body.shapeKey = reported.shapeKey;
		std::copy( reported.position, reported.position + 3, body.position );
		std::copy( reported.rotation, reported.rotation + 4, body.rotation );
		body.boundingRadius = reported.boundingRadius;

		const std::vector<EveAcousticComponent>* components = GetShape( source, body.shapeKey );
		if( components )
		{
			PlaceUnits( reported.bodyId, body, *components );
		}
	}

	for( auto it = m_bodies.begin(); it != m_bodies.end(); )
	{
		if( it->second.seen != m_selections )
		{
			RemoveBodyUnits( it->first );
			it = m_bodies.erase( it );
		}
		else
		{
			++it;
		}
	}
	m_bodyCount = m_bodies.size();
}

const std::vector<EveAcousticComponent>* AudAcousticGeometry::GetShape( IEveAcousticGeometrySource* source, uint64_t shapeKey )
{
	const auto found = m_shapes.find( shapeKey );
	if( found != m_shapes.end() )
	{
		return &found->second;
	}

	std::vector<EveAcousticComponent> components( 16 );
	uint32_t total = source->GetAcousticComponents( shapeKey, components.data(), static_cast<uint32_t>( components.size() ) );
	if( total > components.size() )
	{
		components.resize( total );
		total = source->GetAcousticComponents( shapeKey, components.data(), total );
	}
	components.resize( std::min<size_t>( total, components.size() ) );

	// A component the audio can't use is dropped here, so nothing later has to check it again.
	components.erase( std::remove_if( components.begin(), components.end(), []( const EveAcousticComponent& c )
	{
		return c.meshKey == 0 || c.triangleCount == 0 || !IsFinite( c.translation, 3 ) || !IsFinite( c.rotation, 4 )
			|| !IsFinite( c.boundsMin, 3 ) || !IsFinite( c.boundsMax, 3 );
	} ), components.end() );

	for( EveAcousticComponent& component : components )
	{
		Normalize( component.rotation );
		Mesh& mesh = m_meshes[component.meshKey];
		if( mesh.state == MeshState::Unfetched && mesh.hullCount == 0 )
		{
			mesh.hullCount = component.hullCount;
			mesh.vertexCount = component.vertexCount;
			mesh.triangleCount = component.triangleCount;
		}
	}

	return &m_shapes.emplace( shapeKey, std::move( components ) ).first->second;
}

void AudAcousticGeometry::PlaceUnits( uint64_t bodyId, const Body& body, const std::vector<EveAcousticComponent>& components )
{
	for( uint32_t i = 0; i < components.size(); ++i )
	{
		const EveAcousticComponent& component = components[i];
		Unit& unit = m_units[UnitKey( bodyId, i )];
		unit.meshKey = component.meshKey;

		double translated[3];
		Rotate( body.rotation, component.translation, translated );
		for( int axis = 0; axis < 3; ++axis )
		{
			unit.position[axis] = body.position[axis] + translated[axis];
			unit.boundsMin[axis] = component.boundsMin[axis];
			unit.boundsMax[axis] = component.boundsMax[axis];
		}
		Multiply( body.rotation, component.rotation, unit.rotation );
		Normalize( unit.rotation );

		double radiusSquared = 0.0;
		for( int axis = 0; axis < 3; ++axis )
		{
			const double extent = std::max( std::fabs( component.boundsMin[axis] ), std::fabs( component.boundsMax[axis] ) );
			radiusSquared += extent * extent;
		}
		unit.radius = std::sqrt( radiusSquared );
		unit.triangles = component.triangleCount;
	}
	m_applyDirty = true;
}

void AudAcousticGeometry::RemoveBodyUnits( uint64_t bodyId )
{
	const auto first = m_units.lower_bound( UnitKey( bodyId, 0 ) );
	auto last = first;
	while( last != m_units.end() && last->first.first == bodyId )
	{
		LeaveWwise( last->second );
		++last;
	}
	m_units.erase( first, last );
	m_applyDirty = true;
}

void AudAcousticGeometry::Select( IEveAcousticGeometrySource* source, const AudWorldFrame& frame, const SpatialAudioSettings& settings, const double* listener )
{
	CCP_STATS_ZONE( "AudAcousticGeometry_Select" );

	m_selectionDirty = false;
	++m_selections;

	const double* origin = frame.GetOrigin();
	const float radius = settings.GetGeometryMaxDistance();
	const int budgetSetting = settings.GetGeometryMaxTriangles();
	const size_t budget = static_cast<size_t>( std::max( 0, budgetSetting ) );
	m_selectedRadius = radius;
	m_selectedBudget = budgetSetting;

	// Destiny works in absolute positions; the listener is in the frame, relative to the origin.
	double center[3] = { origin[0], origin[1], origin[2] };
	if( listener )
	{
		for( int axis = 0; axis < 3; ++axis )
		{
			center[axis] += listener[axis];
		}
	}
	SyncBodies( source, center, listener ? radius : 0.0 );

	m_candidates.clear();
	for( auto& entry : m_units )
	{
		Unit& unit = entry.second;
		unit.wanted = false;

		const auto meshIt = m_meshes.find( unit.meshKey );
		if( meshIt == m_meshes.end() || meshIt->second.state == MeshState::Unusable )
		{
			continue;
		}

		double distanceSquared = 0.0;
		if( listener )
		{
			const double relative[3] = { unit.position[0] - origin[0], unit.position[1] - origin[1], unit.position[2] - origin[2] };
			distanceSquared = BoxDistanceSquared( relative, unit.rotation, unit.boundsMin, unit.boundsMax, listener );
			const double limit = radius * ( unit.inWwise ? KEEP_DISTANCE : 1.0 );
			if( radius > 0.0f && distanceSquared > limit * limit )
			{
				continue;
			}
		}

		if( budget > 0 && unit.triangles > budget )
		{
			Mesh& mesh = meshIt->second;
			if( !mesh.budgetWarned )
			{
				mesh.budgetWarned = true;
				CCP_LOGWARN_CH( s_ch, "A collision mesh has %u triangles, more than geometryMaxTriangles (%zu), so it stays out of Wwise.", unit.triangles, budget );
			}
			continue;
		}

		const double rank = unit.inWwise ? distanceSquared * KEEP_RANK_SQUARED : distanceSquared;
		m_candidates.push_back( { rank, unit.triangles, entry.first, &unit } );
	}

	std::sort( m_candidates.begin(), m_candidates.end(), []( const Candidate& a, const Candidate& b )
	{
		if( a.rank != b.rank )
		{
			return a.rank < b.rank;
		}
		if( a.unit->inWwise != b.unit->inWwise )
		{
			return a.unit->inWwise;
		}
		return a.key < b.key;
	} );

	// Nearest first. A component that doesn't fit is skipped, so one big one doesn't keep the smaller ones behind it out.
	size_t triangles = 0;
	for( const Candidate& candidate : m_candidates )
	{
		if( budget > 0 && triangles + candidate.triangles > budget )
		{
			continue;
		}
		triangles += candidate.triangles;
		candidate.unit->wanted = true;

		Mesh& mesh = m_meshes[candidate.unit->meshKey];
		if( mesh.state == MeshState::Unfetched )
		{
			m_meshesToFetch.push_back( candidate.unit->meshKey );
		}
	}

	std::sort( m_meshesToFetch.begin(), m_meshesToFetch.end() );
	m_meshesToFetch.erase( std::unique( m_meshesToFetch.begin(), m_meshesToFetch.end() ), m_meshesToFetch.end() );
	m_applyDirty = true;
}

void AudAcousticGeometry::FetchMeshes( IEveAcousticGeometrySource* source, Clock::time_point start )
{
	CCP_STATS_ZONE( "AudAcousticGeometry_Fetch" );

	std::vector<EveAcousticHull> hulls;
	std::vector<double> vertices;
	std::vector<uint32_t> triangles;

	size_t done = 0;
	for( ; done < m_meshesToFetch.size(); ++done )
	{
		if( done > 0 && Clock::now() - start >= FETCH_BUDGET )
		{
			break;
		}

		const uint64_t meshKey = m_meshesToFetch[done];
		const auto found = m_meshes.find( meshKey );
		if( found == m_meshes.end() || found->second.state != MeshState::Unfetched )
		{
			continue;
		}

		Mesh& mesh = found->second;
		hulls.resize( mesh.hullCount );
		vertices.resize( static_cast<size_t>( mesh.vertexCount ) * 3 );
		triangles.resize( static_cast<size_t>( mesh.triangleCount ) * 3 );
		const bool fetched = mesh.hullCount > 0
			&& source->GetAcousticMesh( meshKey, hulls.data(), mesh.hullCount, vertices.data(), mesh.vertexCount, triangles.data(), mesh.triangleCount );
		++m_meshesFetched;

		if( fetched )
		{
			BuildChunks( mesh, hulls, vertices, triangles );
		}
		if( !fetched || mesh.chunks.empty() )
		{
			mesh.state = MeshState::Unusable;
			CCP_LOGWARN_CH( s_ch, "A collision mesh (%u hulls, %u vertices, %u triangles) is not usable as acoustic geometry.",
				mesh.hullCount, mesh.vertexCount, mesh.triangleCount );
			continue;
		}

		mesh.state = MeshState::Ready;
		m_applyDirty = true;
	}

	m_meshesToFetch.erase( m_meshesToFetch.begin(), m_meshesToFetch.begin() + done );
}

void AudAcousticGeometry::BuildChunks( Mesh& mesh, const std::vector<EveAcousticHull>& hulls, const std::vector<double>& vertices, const std::vector<uint32_t>& triangles )
{
	mesh.chunks.clear();
	mesh.chunks.emplace_back();

	const size_t vertexCount = vertices.size() / 3;
	const size_t triangleCount = triangles.size() / 3;
	for( const EveAcousticHull& hull : hulls )
	{
		const size_t firstVertex = hull.firstVertex;
		const size_t lastVertex = firstVertex + hull.vertexCount;
		const size_t firstTriangle = hull.firstTriangle;
		const size_t lastTriangle = firstTriangle + hull.triangleCount;
		if( hull.vertexCount == 0 || hull.triangleCount == 0 || lastVertex > vertexCount || lastTriangle > triangleCount
			|| hull.vertexCount > MAX_ELEMENTS || hull.triangleCount > MAX_ELEMENTS
			|| !IsFinite( &vertices[firstVertex * 3], static_cast<int>( hull.vertexCount * 3 ) ) )
		{
			continue;
		}

		// Hulls are never split, so a chunk ends before the hull that would take it past Wwise's limits.
		Chunk* chunk = &mesh.chunks.back();
		if( chunk->vertices.size() + hull.vertexCount > MAX_ELEMENTS || chunk->triangles.size() + hull.triangleCount > MAX_ELEMENTS )
		{
			mesh.chunks.emplace_back();
			chunk = &mesh.chunks.back();
		}

		const size_t base = chunk->vertices.size();
		for( size_t v = firstVertex; v < lastVertex; ++v )
		{
			const double* p = &vertices[v * 3];
			// Wwise's space mirrors Z.
			chunk->vertices.emplace_back( static_cast<AkReal32>( p[0] ), static_cast<AkReal32>( p[1] ), static_cast<AkReal32>( -p[2] ) );
		}
		for( size_t t = firstTriangle; t < lastTriangle; ++t )
		{
			const uint32_t* corners = &triangles[t * 3];
			if( corners[0] < firstVertex || corners[0] >= lastVertex || corners[1] < firstVertex || corners[1] >= lastVertex
				|| corners[2] < firstVertex || corners[2] >= lastVertex
				|| corners[0] == corners[1] || corners[1] == corners[2] || corners[0] == corners[2] )
			{
				continue;
			}
			chunk->triangles.emplace_back(
				static_cast<AkVertIdx>( corners[0] - firstVertex + base ),
				static_cast<AkVertIdx>( corners[1] - firstVertex + base ),
				static_cast<AkVertIdx>( corners[2] - firstVertex + base ),
				static_cast<AkSurfIdx>( 0 ) );
		}
	}

	mesh.chunks.erase( std::remove_if( mesh.chunks.begin(), mesh.chunks.end(), []( const Chunk& c ) { return c.triangles.empty(); } ), mesh.chunks.end() );
	for( Chunk& chunk : mesh.chunks )
	{
		chunk.setId = NextId();
	}
}

bool AudAcousticGeometry::SendSet( Chunk& chunk )
{
	AkAcousticSurface surface;
	surface.strName = "collision";
	surface.transmissionLoss = m_transmissionLoss;

	AkGeometryParams params;
	params.Vertices = chunk.vertices.data();
	params.NumVertices = static_cast<AkVertIdx>( chunk.vertices.size() );
	params.Triangles = chunk.triangles.data();
	params.NumTriangles = static_cast<AkTriIdx>( chunk.triangles.size() );
	params.Surfaces = &surface;
	params.NumSurfaces = 1;
	params.EnableDiffraction = m_enableDiffraction;
	params.EnableDiffractionOnBoundaryEdges = m_enableDiffractionOnBoundaryEdges;

	const AKRESULT result = AK::SpatialAudio::SetGeometry( chunk.setId, params );
	++m_setsSent;
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set acoustic geometry set %llu (%zu vertices, %zu triangles), AKRESULT: %d",
			chunk.setId, chunk.vertices.size(), chunk.triangles.size(), result );
		return false;
	}
	chunk.inWwise = true;
	return true;
}

void AudAcousticGeometry::LeaveWwise( Unit& unit )
{
	for( ChunkInstance& instance : unit.instances )
	{
		if( instance.inWwise )
		{
			if( AK::SoundEngine::IsInitialized() )
			{
				AK::SpatialAudio::RemoveGeometryInstance( instance.id );
			}
			instance.inWwise = false;
		}
	}
	unit.inWwise = false;
}

void AudAcousticGeometry::Apply( const AudWorldFrame& frame, bool solid )
{
	CCP_STATS_ZONE( "AudAcousticGeometry_Apply" );

	const double* origin = frame.GetOrigin();
	for( auto& entry : m_units )
	{
		Unit& unit = entry.second;
		if( !unit.wanted )
		{
			if( unit.inWwise )
			{
				LeaveWwise( unit );
			}
			continue;
		}

		const auto meshIt = m_meshes.find( unit.meshKey );
		if( meshIt == m_meshes.end() || meshIt->second.state != MeshState::Ready )
		{
			continue;
		}
		Mesh& mesh = meshIt->second;

		if( unit.instances.size() != mesh.chunks.size() )
		{
			LeaveWwise( unit );
			unit.instances.resize( mesh.chunks.size() );
			for( ChunkInstance& instance : unit.instances )
			{
				if( instance.id == 0 )
				{
					instance.id = NextId();
				}
			}
		}

		const double relative[3] = { unit.position[0] - origin[0], unit.position[1] - origin[1], unit.position[2] - origin[2] };
		AkWorldTransform transform;
		RH2LH::convertRigid( relative, unit.rotation, transform );

		bool anyInWwise = false;
		for( size_t i = 0; i < mesh.chunks.size(); ++i )
		{
			Chunk& chunk = mesh.chunks[i];
			chunk.lastUsed = m_selections;
			if( !chunk.inWwise && !SendSet( chunk ) )
			{
				continue;
			}

			ChunkInstance& instance = unit.instances[i];
			AkGeometryInstanceParams params;
			params.PositionAndOrientation = transform;
			params.GeometrySetID = chunk.setId;
			params.UseForReflectionAndDiffraction = true;
			params.IsSolid = solid;

			if( instance.inWwise && SameInstanceParams( params, instance.sent ) )
			{
				anyInWwise = true;
				continue;
			}

			const AKRESULT result = AK::SpatialAudio::SetGeometryInstance( instance.id, params );
			++m_instancesSent;
			if( result != AK_Success )
			{
				CCP_LOGERR_CH( s_ch, "Failed to set acoustic geometry instance %llu (set %llu), AKRESULT: %d", instance.id, chunk.setId, result );
				instance.inWwise = false;
				continue;
			}
			instance.sent = params;
			instance.inWwise = true;
			anyInWwise = true;
		}
		unit.inWwise = anyInWwise;
	}
}

void AudAcousticGeometry::EvictUnusedSets( size_t budget )
{
	std::unordered_set<uint64_t> used;
	size_t usedTriangles = 0;
	for( const auto& entry : m_units )
	{
		if( entry.second.inWwise )
		{
			used.insert( entry.second.meshKey );
		}
	}

	std::vector<std::pair<uint64_t, Chunk*>> unused;
	size_t unusedTriangles = 0;
	for( auto& entry : m_meshes )
	{
		const bool inUse = used.count( entry.first ) > 0;
		for( Chunk& chunk : entry.second.chunks )
		{
			if( !chunk.inWwise )
			{
				continue;
			}
			if( inUse )
			{
				usedTriangles += chunk.triangles.size();
			}
			else
			{
				unused.emplace_back( chunk.lastUsed, &chunk );
				unusedTriangles += chunk.triangles.size();
			}
		}
	}

	const size_t limit = UNUSED_TRIANGLES_PER_BUDGET * ( budget > 0 ? budget : usedTriangles );
	if( unusedTriangles <= limit )
	{
		return;
	}

	std::sort( unused.begin(), unused.end(), []( const auto& a, const auto& b ) { return a.first < b.first; } );
	for( const auto& entry : unused )
	{
		Chunk& chunk = *entry.second;
		AK::SpatialAudio::RemoveGeometry( chunk.setId );
		chunk.inWwise = false;
		unusedTriangles -= chunk.triangles.size();
		if( unusedTriangles <= limit )
		{
			break;
		}
	}
}

void AudAcousticGeometry::Flush()
{
	RemoveAllFromWwise();
}

void AudAcousticGeometry::RemoveAllFromWwise()
{
	if( AK::SoundEngine::IsInitialized() )
	{
		for( auto& entry : m_meshes )
		{
			for( const Chunk& chunk : entry.second.chunks )
			{
				// A set takes its instances with it.
				if( chunk.inWwise )
				{
					AK::SpatialAudio::RemoveGeometry( chunk.setId );
				}
			}
		}
	}

	m_bodies.clear();
	m_shapes.clear();
	m_meshes.clear();
	m_units.clear();
	m_meshesToFetch.clear();
	m_lastSource = nullptr;
	m_selectionDirty = true;
	m_bodyCount = 0;
	UpdateStats();
}

void AudAcousticGeometry::ForgetWwiseState()
{
	for( auto& entry : m_meshes )
	{
		for( Chunk& chunk : entry.second.chunks )
		{
			chunk.inWwise = false;
		}
	}
	for( auto& entry : m_units )
	{
		for( ChunkInstance& instance : entry.second.instances )
		{
			instance.inWwise = false;
		}
		entry.second.inWwise = false;
	}
	m_selectionDirty = true;
	m_applyDirty = true;
	UpdateStats();
}

void AudAcousticGeometry::UpdateStats()
{
	size_t sets = 0;
	for( const auto& entry : m_meshes )
	{
		for( const Chunk& chunk : entry.second.chunks )
		{
			sets += chunk.inWwise ? 1 : 0;
		}
	}

	size_t instances = 0;
	size_t triangles = 0;
	for( const auto& entry : m_units )
	{
		const Unit& unit = entry.second;
		const auto meshIt = m_meshes.find( unit.meshKey );
		for( size_t i = 0; i < unit.instances.size(); ++i )
		{
			if( !unit.instances[i].inWwise )
			{
				continue;
			}
			++instances;
			if( meshIt != m_meshes.end() && i < meshIt->second.chunks.size() )
			{
				triangles += meshIt->second.chunks[i].triangles.size();
			}
		}
	}

	m_setsInWwise = sets;
	m_instancesInWwise = instances;
	m_trianglesInWwise = triangles;
	CCP_STATS_SET( acousticSets, sets );
	CCP_STATS_SET( acousticInstances, instances );
	CCP_STATS_SET( acousticTriangles, triangles );
}

bool AudAcousticGeometry::GetInstanceWwisePosition( uint64_t bodyId, double out[3] ) const
{
	for( auto it = m_units.lower_bound( UnitKey( bodyId, 0 ) ); it != m_units.end() && it->first.first == bodyId; ++it )
	{
		for( const ChunkInstance& instance : it->second.instances )
		{
			if( instance.inWwise )
			{
				const AkVector64 position = instance.sent.PositionAndOrientation.Position();
				out[0] = position.X;
				out[1] = position.Y;
				out[2] = position.Z;
				return true;
			}
		}
	}
	return false;
}
