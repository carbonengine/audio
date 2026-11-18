////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include "IEveAcousticGeometrySource.h"

#include <AK/SpatialAudio/Common/AkSpatialAudioTypes.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class AudWorldFrame;
class SpatialAudioSettings;

/**
 * @brief Sends destiny's convex collision to Wwise as acoustic geometry.
 *
 * Bodies, their components and the hulls of each mesh are pulled from an IEveAcousticGeometrySource. Every collision
 * component of a body is one or more Wwise geometry instances, placed in the world frame, so a static body is sent once
 * and again only when it really moves or the frame's origin does. Components are taken nearest the listener first up to
 * the triangle budget; meshes are fetched only once a component of theirs is wanted.
 *
 * Everything runs on the tick thread; the counters can be read from anywhere.
 */
class AudAcousticGeometry
{
public:
	AudAcousticGeometry();

	/**
	 * @param listenerScene Listener in trinity's scene space, or null to rank without distances.
	 */
	void Update( IEveAcousticGeometrySource* source, const AudWorldFrame& frame, const SpatialAudioSettings& settings, const Vector3* listenerScene );

	/// Takes everything out of Wwise and forgets what destiny reported.
	void RemoveAllFromWwise();
	/// Call after the sound engine is terminated.
	void ForgetWwiseState();

	bool HasInstancesInWwise() const { return m_instancesInWwise.load( std::memory_order_relaxed ) > 0; }

	size_t GetSetsInWwise() const { return m_setsInWwise; }
	size_t GetInstancesInWwise() const { return m_instancesInWwise; }
	size_t GetTrianglesInWwise() const { return m_trianglesInWwise; }
	size_t GetBodies() const { return m_bodyCount; }
	size_t GetSetsSent() const { return m_setsSent; }
	size_t GetInstancesSent() const { return m_instancesSent; }
	size_t GetMeshesFetched() const { return m_meshesFetched; }

	/// Where Wwise has the first instance of a body, in Wwise's left-handed space. False when none of it is in Wwise.
	bool GetInstanceWwisePosition( uint64_t bodyId, double out[3] ) const;

private:
	using Clock = std::chrono::steady_clock;

	struct Chunk
	{
		std::vector<AkVertex> vertices;
		std::vector<AkTriangle> triangles;
		uint64_t setId = 0;
		uint64_t lastUsed = 0;
		bool inWwise = false;
	};

	enum class MeshState
	{
		Unfetched,
		Ready,
		Unusable,
	};

	struct Mesh
	{
		MeshState state = MeshState::Unfetched;
		uint32_t hullCount = 0;
		uint32_t vertexCount = 0;
		uint32_t triangleCount = 0;
		std::vector<Chunk> chunks;
		bool budgetWarned = false;
	};

	struct Body
	{
		uint64_t shapeKey = 0;
		double position[3] = {};
		double rotation[4] = {};
		double boundingRadius = 0.0;
		uint64_t seen = 0;
	};

	struct ChunkInstance
	{
		uint64_t id = 0;
		AkGeometryInstanceParams sent;
		bool inWwise = false;
	};

	/// One collision component of one body.
	struct Unit
	{
		uint64_t meshKey = 0;
		/// Component to world, relative to nothing: absolute.
		double position[3] = {};
		double rotation[4] = {};
		double boundsMin[3] = {};
		double boundsMax[3] = {};
		double radius = 0.0;
		uint32_t triangles = 0;
		std::vector<ChunkInstance> instances;
		bool wanted = false;
		bool inWwise = false;
	};

	using UnitKey = std::pair<uint64_t, uint32_t>;

	struct Candidate
	{
		double rank;
		uint32_t triangles;
		UnitKey key;
		Unit* unit;
	};

	void Flush();
	void Select( IEveAcousticGeometrySource* source, const AudWorldFrame& frame, const SpatialAudioSettings& settings, const double* listener );
	void SyncBodies( IEveAcousticGeometrySource* source, const double center[3], double radius );
	const std::vector<EveAcousticComponent>* GetShape( IEveAcousticGeometrySource* source, uint64_t shapeKey );
	void PlaceUnits( uint64_t bodyId, const Body& body, const std::vector<EveAcousticComponent>& components );
	void RemoveBodyUnits( uint64_t bodyId );
	void FetchMeshes( IEveAcousticGeometrySource* source, Clock::time_point start );
	void BuildChunks( Mesh& mesh, const std::vector<EveAcousticHull>& hulls, const std::vector<double>& vertices, const std::vector<uint32_t>& triangles );
	void Apply( const AudWorldFrame& frame, bool solid );
	bool SendSet( Chunk& chunk );
	void LeaveWwise( Unit& unit );
	void EvictUnusedSets( size_t budget );
	void UpdateStats();
	uint64_t NextId();

	IEveAcousticGeometrySource* m_lastSource;
	uint64_t m_sourceGeneration;
	uint64_t m_lastEpoch;
	uint64_t m_nextId;
	uint64_t m_selections;
	Clock::time_point m_selectedAt;
	double m_selectedListener[3];
	float m_selectedRadius;
	int m_selectedBudget;
	bool m_selectionDirty;
	bool m_applyDirty;

	// The mesh settings as Wwise last got them.
	float m_transmissionLoss;
	bool m_enableDiffraction;
	bool m_enableDiffractionOnBoundaryEdges;
	bool m_solid;

	std::unordered_map<uint64_t, Body> m_bodies;
	std::unordered_map<uint64_t, std::vector<EveAcousticComponent>> m_shapes;
	std::unordered_map<uint64_t, Mesh> m_meshes;
	std::map<UnitKey, Unit> m_units;
	std::vector<uint64_t> m_meshesToFetch;
	std::vector<EveAcousticBody> m_bodyScratch;
	std::vector<Candidate> m_candidates;

	std::atomic<size_t> m_setsInWwise;
	std::atomic<size_t> m_instancesInWwise;
	std::atomic<size_t> m_trianglesInWwise;
	std::atomic<size_t> m_bodyCount;
	std::atomic<size_t> m_setsSent;
	std::atomic<size_t> m_instancesSent;
	std::atomic<size_t> m_meshesFetched;
};
