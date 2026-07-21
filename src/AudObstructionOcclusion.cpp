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

bool AudObstructionOcclusion::SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const
{
	const AKRESULT result = AK::SoundEngine::SetObjectObstructionAndOcclusion(
		emitterID,
		LISTENER_GAME_OBJ_ID,
		state.obstruction.currentValue,
		state.occlusion.currentValue);
	return result == AK_Success;


void AudObstructionOcclusion::RemoveEmitter(AkGameObjectID emitterID)
{}

void AudObstructionOcclusion::Reset()
{}

void AudObstructionOcclusion::ClearAll()
{}
