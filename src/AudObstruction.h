////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Feb 2026
// Copyright (c) 2026 CCP Games
//
// Queries QueryDiffractionPaths() to read the
// transmission/diffraction values from registered geometry, then linearly fades toward them
// over time and feeds the result to SetObjectObstructionAndOcclusion().

#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>
#include "IPrioritizedObject.h"

class AudObstruction
{
public:
	AudObstruction();

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
		float nextQueryTime = 0.0f;
		bool initialized = false;
	};

	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;

	float m_time = 0.0f;
	std::chrono::steady_clock::time_point m_lastUpdateTime;

	float m_refreshInterval = 0.2f;

	// Linear fade rate in units per second
	float m_fadeRate = 2.0f;

	void QueryEmitter( AkGameObjectID emitterID, EmitterState& state );

	void CleanupStaleEmitters( const std::vector<IPrioritizedObject*>& gameObjects );

	int m_cleanupCounter = 0;
	static constexpr int kCleanupEveryNFrames = 120;
};
