////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <ITr2AudGeometry.h>

#include <AK/SpatialAudio/Common/AkSpatialAudioTypes.h>

#include <CcpMutex.h>

#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class SpatialAudioSettings;

/**
 * @brief Keeps every geometry set and instance trinity registers and sends them to Wwise once per tick.
 *
 * Trinity calls in from any thread at any time, whether or not Wwise is running. Wwise is only
 * touched from Update(), RemoveAllFromWwise() and ForgetWwiseState(), on the audio tick thread.
 */
class AudGeometryManager
{
public:
	/// Welded geometry in Wwise's left-handed space.
	struct Mesh
	{
		std::vector<AkVertex> vertices;
		std::vector<AkTriangle> triangles;
		/// Bounding sphere in trinity's right-handed mesh space.
		float center[3] = {};
		float radius = 0.0f;
	};

	static AudGeometryManager& Get();

	void SetGeometry( uint64_t geometrySetId, uint64_t instanceId, const Tr2AudGeometryData& geometryData, const Matrix& worldTransform );
	void SetGeometryTransform( uint64_t instanceId, const Matrix& worldTransform );
	void RemoveGeometry( uint64_t instanceId );

	/**
	 * @brief Sends what changed since the last call, keeping only the instances nearest the listener in Wwise.
	 *
	 * @param listenerPosition Listener in trinity's space, or null to skip the distance limit.
	 */
	void Update( const SpatialAudioSettings& settings, const Vector3* listenerPosition );

	/// Removes everything from Wwise but keeps it registered, so the next Update() sends it again.
	void RemoveAllFromWwise();

	/// Call after the sound engine is terminated.
	void ForgetWwiseState();

	/// What Wwise holds as of the last Update().
	size_t GetSetsInWwise() const { return m_setsInWwise; }
	size_t GetInstancesInWwise() const { return m_instancesInWwise; }
	size_t GetTrianglesInWwise() const { return m_trianglesInWwise; }
	/// Every instance trinity registered, in Wwise or not.
	size_t GetInstancesRegistered() const { return m_instancesRegistered; }

	/**
	 * @brief Welds vertices that share a position and drops triangles Wwise can't use.
	 *
	 * Wwise finds diffraction edges through shared vertex indices, so render meshes with split
	 * vertices need welding. The result is empty when the mesh exceeds Wwise's 16-bit limits.
	 */
	static Mesh BuildMesh( const Tr2AudGeometryData& geometryData );

private:
	struct GeometrySet
	{
		Mesh mesh;
		uint32_t instanceCount = 0;
		bool inWwise = false;
		/// Wwise refused it, so it is not sent again until Wwise restarts.
		bool rejected = false;
		bool budgetWarned = false;
	};

	struct Instance
	{
		uint64_t geometrySetId = 0;
		Matrix transform;
		AkGeometryInstanceParams sent;
		bool inWwise = false;
		bool dirty = false;
		bool wanted = false;
		/// Wwise refused it, so it is not sent again until it changes.
		bool rejected = false;
	};

	struct Candidate
	{
		float distance;
		uint32_t triangles;
		uint64_t instanceId;
		Instance* instance;
		GeometrySet* set;
	};

	AudGeometryManager();

	void SelectInstancesLocked( const SpatialAudioSettings& settings, const Vector3* listenerPosition );

	void ReleaseSetLocked( uint64_t geometrySetId );
	bool SendSetLocked( uint64_t geometrySetId, GeometrySet& set );
	void ForgetWwiseStateLocked();
	/// Takes the sets whose instances all left Wwise out of it.
	void RemoveUnusedSetsLocked();
	void UpdateStatsLocked();

	std::unordered_map<uint64_t, GeometrySet> m_sets;
	std::unordered_map<uint64_t, Instance> m_instances;
	/// Instances and sets trinity removed that are still in Wwise.
	std::vector<uint64_t> m_instancesToRemove;
	std::vector<uint64_t> m_setsToRemove;
	std::vector<Candidate> m_candidates;
	std::unordered_set<uint64_t> m_setsInUse;
	/// The settings as Wwise last got them.
	float m_transmissionLoss;
	bool m_enableDiffraction;
	bool m_enableDiffractionOnBoundaryEdges;
	bool m_solid;
	bool m_statsDirty;
	std::atomic<size_t> m_setsInWwise;
	std::atomic<size_t> m_instancesInWwise;
	std::atomic<size_t> m_trianglesInWwise;
	std::atomic<size_t> m_instancesRegistered;
	CcpMutex m_mutex;
};
