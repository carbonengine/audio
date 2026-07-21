////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 CCP Games
//

#include "AudObstructionOcclusion.h"
#include "AudManager.h"

#include <algorithm>

AudObstructionOcclusion::AudObstructionOcclusion()	:
	m_fadeRate(DEFAULT_FADE_RATE),
	m_hasUpdated(false),
	m_enabled(true),
	m_mutex("AudObstructionOcclusion", "m_mutex")
{}

AudObstructionOcclusion::~AudObstructionOcclusion()
{}

void AudObstructionOcclusion::Update()
{

	if (g_audioManager == nullptr || g_audioManager->GetState() != AudioState::Enabled)
	{
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	float deltaSeconds = 0.0f;

	if (m_hasUpdated)
	{
		deltaSeconds = std::chrono::duration<float>(now - m_lastUpdateTime).count();
	}
	m_lastUpdateTime = now;
	m_hasUpdated = true;

	CcpAutoMutex lock( m_mutex );

	for (auto it = m_emitters.begin(); it != m_emitters.end();)
	{
		const AkGameObjectID emitterID = it->first;
		EmitterState& state = it->second;

		bool culled = false;
		const bool exists = g_audioManager->WithCallbackGameObject(emitterID, [&culled](AudGameObjResource* emitter) {
			culled = emitter->IsCulled();
			});

		if (!exists)
		{
			it = m_emitters.erase(it);
			continue;
		}

		const bool obstructionChanged = state.obstruction.Advance(deltaSeconds);
		const bool occlusionChanged = state.occlusion.Advance(deltaSeconds);

		if (culled)
		{
			// The emitter is outside Wwise's view. Keep fading so time stays
			// consistent, and make sure the values are resent once the emitter wakes up.
			state.needsSend = true;
			++it;
			continue;
		}

		if (obstructionChanged || occlusionChanged || state.needsSend)
		{
			state.needsSend = !SendToWwise(emitterID, state);
		}

		// Remove the emitter from the list if it has reached its target values and is no longer needed.
		if (!state.needsSend &&
			state.obstruction.ReachedTarget() && state.obstruction.targetValue == 0.0f &&
			state.occlusion.ReachedTarget() && state.occlusion.targetValue == 0.0f)
		{
			it = m_emitters.erase(it);
			continue;
		}

		++it;
	}
}

bool AudObstructionOcclusion::SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion)
{
	if (!m_enabled)
	{
		return false;
	}

	if (g_audioManager == nullptr || g_audioManager->GetState() != AudioState::Enabled)
	{
		return false;
	}

	// Values are relative to the listener, so setting them on the listener itself is meaningless.
	if (emitterID == LISTENER_GAME_OBJ_ID)
	{
		return false;
	}

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
	// When Acoustics is On its transmission already attenuates, so skip occlusion to avoid stacking.
	// Might change in the future with the addition of volumes.
	const bool acousticsEnabled = g_audioManager != nullptr && g_audioManager->GetSpatialAudioGeometryEnabled();

	float occlusion = 0.0f;
	if (!acousticsEnabled)
	{
		occlusion = blockage;
	}

	return SetObstructionOcclusion(emitterID, 0.0f, occlusion);
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
{
	CcpAutoMutex lock(m_mutex);
	m_emitters.erase(emitterID);
}

void AudObstructionOcclusion::Reset()
{
	CcpAutoMutex lock(m_mutex);
	m_emitters.clear();
	m_hasUpdated = false;
}

void AudObstructionOcclusion::ClearAll()
{
	CcpAutoMutex lock(m_mutex);
	for (auto& pair : m_emitters)
	{
		pair.second.obstruction.SetTarget(0.0f, m_fadeRate);
		pair.second.occlusion.SetTarget(0.0f, m_fadeRate);
	}
}

bool AudObstructionOcclusion::GetObstructionOcclusionEnabled() const
{
	return m_enabled;
}

void AudObstructionOcclusion::SetObstructionOcclusionEnabled(bool value)
{
	if (m_enabled == value)
	{
		return;
	}

	m_enabled = value;
	if (!m_enabled)
	{
		ClearAll();
	}
}

float AudObstructionOcclusion::GetObstructionOcclusionFadeRate() const
{
	return m_fadeRate;
}

void AudObstructionOcclusion::SetObstructionOcclusionFadeRate(float value)
{
	m_fadeRate = std::max(value, 0.0f);
}

void AudObstructionOcclusion::FadingValue::SetTarget(float target, float fadeRate)
{
	targetValue = std::clamp(target, 0.0f, 1.0f);

	if (targetValue >= currentValue)
	{
		rate = fadeRate;
	}
	else
	{
		rate = -fadeRate;
	}
}

bool AudObstructionOcclusion::FadingValue::Advance(float deltaSeconds)
{
	if (currentValue == targetValue)
	{
		return false;
	}

	if (rate == 0.0f)
	{
		currentValue = targetValue;
		return true;
	}

	const float oldValue = currentValue;
	const float newValue = oldValue + rate * deltaSeconds;

	if (oldValue > targetValue)
	{
		currentValue = std::clamp(newValue, targetValue, oldValue);
	}
	else
	{
		currentValue = std::clamp(newValue, oldValue, targetValue);
	}
	return currentValue != oldValue;
}
