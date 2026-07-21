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
};