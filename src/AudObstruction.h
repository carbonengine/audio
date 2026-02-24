////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Feb 2026
// Copyright (c) 2026 CCP Games
//
// Smooth obstruction and occlusion using Wwise's own spatial
// audio raycasting.
//
// Each frame, queries QueryDiffractionPaths() to read the
// transmission/diffraction values that Wwise already computed
// from registered geometry, then exponentially smooths them
// over time and feeds the result to SetObjectObstructionAndOcclusion().
//
// Obstruction (direct path only) is driven by diffraction.
// Occlusion (direct + reverb) is driven by transmissionLoss.
// The Wwise project curves decide how each one affects sound.
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

private:
	struct SmoothedValues
	{
		float obstruction = 0.0f;
		float occlusion = 0.0f;
	};

	std::unordered_map<AkGameObjectID, SmoothedValues> m_smoothed;
	std::chrono::steady_clock::time_point m_lastUpdateTime;

	// Smoothing rate. Higher = faster convergence. 2.0 gives ~1 second transitions.
	static constexpr float kSmoothingRate = 2.0f;
};
