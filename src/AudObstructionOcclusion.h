////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h> 

struct FadingValue
{
	float currentValue = 0.0f;
	float targetValue = 0.0f;
	float rate = 0.0f;
};

struct EmitterState
{
	FadingValue obstruction;
	FadingValue occlusion;

};

class AudObstructionOcclusion
{
public:

	AudObstructionOcclusion();
	~AudObstructionOcclusion();

	void Update();

	bool SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion);

	bool SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage);

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	void RemoveEmitter( AkGameObjectID emitterID );

	void Reset();

	void ClearAll();
};