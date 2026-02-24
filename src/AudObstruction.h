////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Feb 2026
// Copyright (c) 2026 CCP Games
//
// Smooth obstruction and occlusion using Wwise's own spatial
// audio raycasting.
//
// Periodically queries QueryDiffractionPaths() to read the
// transmission/diffraction values that Wwise already computed
// from registered geometry, then linearly fades toward them
// over time and feeds the result to SetObjectObstructionAndOcclusion().
//
// Obstruction (direct path only) is driven by diffraction.
// Occlusion (direct + reverb) is driven by transmissionLoss.
// The Wwise project curves decide how each one affects sound.
//
// Queries are throttled to a configurable refresh interval
// (default 0.2s) and staggered across emitters to avoid
// CPU spikes from all emitters querying on the same frame.
//

#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>
#include "IPrioritizedObject.h"

class AudObstruction
{
public:
	AudObstruction();

	// Call once per frame from AudManager::Process(), after CullAudio() and before RenderAudio().
	void Update( AkGameObjectID listenerID,
	             const std::vector<IPrioritizedObject*>& gameObjects );

	void SetRefreshInterval( float seconds );
	float GetRefreshInterval() const;

	void SetFadeRate( float unitsPerSecond );
	float GetFadeRate() const;

private:
	struct EmitterState
	{
		float obstruction = 0.0f;
		float occlusion = 0.0f;
		float targetObstruction = 0.0f;
		float targetOcclusion = 0.0f;
		float nextQueryTime = 0.0f; // Staggered per-emitter query time
		bool initialized = false;
	};

	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;

	// Accumulated time since start, used for scheduling queries.
	float m_time = 0.0f;
	std::chrono::steady_clock::time_point m_lastUpdateTime;

	// How often to re-query Wwise per emitter (seconds). Default 0.2s = 5 queries/sec.
	float m_refreshInterval = 0.2f;

	// Linear fade rate in units per second. 2.0 = full 0-to-1 transition in 0.5 seconds.
	float m_fadeRate = 2.0f;

	// Query Wwise for the current diffraction/transmission values for an emitter.
	void QueryEmitter( AkGameObjectID emitterID, EmitterState& state );

	// Remove entries for emitters that are no longer in the active set.
	void CleanupStaleEmitters( const std::vector<IPrioritizedObject*>& gameObjects );

	// Counter to throttle cleanup (not every frame).
	int m_cleanupCounter = 0;
	static constexpr int kCleanupEveryNFrames = 120;
};
