////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 CCP Games
//

#include "AudObstructionOcclusion.h"
#include "AudManager.h"

AudObstructionOcclusion::AudObstructionOcclusion()
{}

AudObstructionOcclusion::~AudObstructionOcclusion()
{}

void AudObstructionOcclusion::Update()
{}

bool AudObstructionOcclusion::SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion)
{
	return false;
}

bool AudObstructionOcclusion::SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage)
{
	return false;
}

void AudObstructionOcclusion::RemoveEmitter(AkGameObjectID emitterID)
{}

void AudObstructionOcclusion::Reset()
{}

void AudObstructionOcclusion::ClearAll()
{}
