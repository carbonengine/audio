////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 CCP Games
//

#include "AudObstructionOcclusion.h"
#include "AudManager.h"

AudObstructionOcclusion::AudObstructionOcclusion()	:
	m_fadeRate(0.0f),
	m_enabled(false),
	m_hasUpdated(false),
	m_mutex("AudObstructionOcclusion;", "m_mutex")
{}

AudObstructionOcclusion::~AudObstructionOcclusion()
{}

void AudObstructionOcclusion::Update()
{}

bool AudObstructionOcclusion::SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion)
{
	// We need to ask AudioManager about emitters that actually exist.
	if (!g_audioManager->WithCallbackGameObject(emitterID, [](AudGameObjResource*) {}))
	{
		return false;
	}

	CcpAutoMutex lock(m_mutex);
	EmitterState& state = m_emitters[emitterID];

	state.obstruction.SetTarget(obstruction, m_fadeRate);
	state.occlusion.SetTarget(occlusion, m_fadeRate);

	return true;
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
}

void AudObstructionOcclusion::RemoveEmitter(AkGameObjectID emitterID)
{}

void AudObstructionOcclusion::Reset()
{}

void AudObstructionOcclusion::ClearAll()
{}

void AudObstructionOcclusion::FadingValue::SetTarget(float target, float fadeRate)
{

}
