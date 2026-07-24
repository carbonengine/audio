////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include <chrono>
#include <unordered_map>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

class AudManager;

class AudObstructionOcclusion
{
public:

	AudObstructionOcclusion(AudManager* audioManager);
	~AudObstructionOcclusion();

	void Update();

	bool SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion);

	bool SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage);

	void RemoveEmitter( AkGameObjectID emitterID );

	void Reset();

	void ClearAll();

	bool GetObstructionOcclusionEnabled() const;
	void SetObstructionOcclusionEnabled(bool value);

	float GetObstructionOcclusionFadeRate() const;
	void SetObstructionOcclusionFadeRate(float value);

private:

	struct FadingValue
	{
		float currentValue = 0.0f;
		float targetValue = 0.0f;

		void SetTarget(float target);
		bool Advance(float deltaSeconds, float fadeRate);
		bool ReachedTarget() const { return currentValue == targetValue; };
	};

	struct EmitterState
	{
		FadingValue obstruction;
		FadingValue occlusion;

		bool needsSend = true;

	};

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	// Fades from clear to fully blocked in one second.
	static constexpr float DEFAULT_FADE_RATE = 1.0f;

	AudManager* m_audioManager;
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	float m_fadeRate;
	bool m_hasUpdated;
	bool m_enabled;
	mutable CcpMutex m_mutex;

	std::chrono::steady_clock::time_point m_lastUpdateTime;

};