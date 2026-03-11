////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Feb 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>
#include "IPrioritizedObject.h"

/**
 * @brief Tracks and applies per-emitter obstruction/occlusion values.
 *
 * This class queries Wwise Spatial Audio diffraction paths, derives target
 * obstruction/occlusion values, smooths them over time, and writes the
 * smoothed values back to Wwise each tick.
 */
class AudObstructionOcclusion
{
public:
	/**
	 * @brief Constructor and init timing state.
	 */
	AudObstructionOcclusion();

	/**
	 * @brief Updates obstruction/occlusion for the current active emitter set.
	 *
	 * @param listenerID  Game object ID of the active listener.
	 * @param gameObjects Active prioritized audio objects to evaluate.
	 */
	void Update( AkGameObjectID listenerID,
	             const std::vector<IPrioritizedObject*>& gameObjects );

	/**
	 * @brief Sets how often each emitter is re-queried from Spatial Audio.
	 *
	 * @param seconds Query interval in seconds.
	 */
	void SetRefreshInterval( float seconds );

	/**
	 * @brief Gets the per-emitter Spatial Audio query interval in seconds.
	 *
	 * @return Current refresh interval in seconds.
	 */
	float GetRefreshInterval() const;

	/**
	 * @brief Sets linear smoothing speed toward target values.
	 *
	 * @param unitsPerSecond Fade speed in units per second.
	 */
	void SetFadeRate( float unitsPerSecond );

	/**
	 * @brief Gets linear smoothing speed toward target values.
	 *
	 * @return Current fade rate in units per second.
	 */
	float GetFadeRate() const;

	/**
	 * @brief Sets the fixed occlusion value applied when an emitter is blocked.
	 *
	 * @param value Occlusion level [0.0-1.0].
	 */
	void SetOcclusionValue( float value );

	/**
	 * @brief Gets the fixed occlusion value applied when an emitter is blocked.
	 *
	 * @return Current occlusion value.
	 */
	float GetOcclusionValue() const;

private:
	/**
	 * @brief Cached runtime state for one emitter.
	 */
	struct EmitterState
	{
		float obstruction = 0.0f;
		float occlusion = 0.0f;    
		float targetObstruction = 0.0f;
		float targetOcclusion = 0.0f;
		float nextQueryTime = 0.0f;
		bool initialized = false;
	};

	/// Active emitter state indexed by game object ID.
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;

	/// local time accumulator in seconds.
	float m_time = 0.0f;

	/// Timestamp from previous update.
	std::chrono::steady_clock::time_point m_lastUpdateTime;

	/// Per-emitter query cadence in seconds.
	float m_refreshInterval = 0.2f;

	/// Linear fade rate in second.
	float m_fadeRate = 2.0f;

	/// Fixed occlusion value applied when emitter is blocked.
	float m_occlusionValue = 0.7f;

	/**
	 * @brief Queries Wwise Spatial Audio and updates emitter target values.
	 *
	 * @param emitterID Emitter game object ID.
	 * @param state     Mutable state to update with query results.
	 */
	void QueryEmitter( AkGameObjectID emitterID, EmitterState& state );

	/**
	 * @brief Removes cached emitter states not present in the active set.
	 *
	 * @param gameObjects Active prioritized audio objects.
	 */
	void CleanupStaleEmitters( const std::vector<IPrioritizedObject*>& gameObjects );

	/// Frame counter used to schedule periodic stale-state cleanup.
	int m_cleanupCounter = 0;

	/// Number of update frames between stale-state cleanup passes.
	static constexpr int kCleanupEveryNFrames = 120;
};
