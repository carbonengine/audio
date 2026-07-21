////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h> 

class AudObstructionOcclusion
{
public:

	AudObstructionOcclusion();
	~AudObstructionOcclusion();

	void Update();

	bool SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion);

	bool SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage);

	void RemoveEmitter( AkGameObjectID emitterID );

	void Reset();

	void ClearAll();

private:

	struct FadingValue
	{
		float currentValue = 0.0f;
		float targetValue = 0.0f;
		float rate = 0.0f;

		void SetTarget(float target, float fadeRate);
		bool Advance(float deltaSeconds);
		bool ReachedTarget() const { return currentValue == targetValue; };
	};

	struct EmitterState
	{
		FadingValue obstruction;
		FadingValue occlusion;

		bool needsSend = true;

	};

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	float m_fadeRate;
	bool m_enabled;
	bool m_hasUpdated;
	mutable CcpMutex m_mutex;

};