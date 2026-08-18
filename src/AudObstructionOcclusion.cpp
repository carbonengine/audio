////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#include "AudObstructionOcclusion.h"
#include "AudManager.h"

#include <algorithm>

AudObstructionOcclusion::AudObstructionOcclusion(AudManager* audioManager)	:
	m_audioManager(audioManager),
	m_fadeRate(DEFAULT_FADE_RATE),
	m_blockedOcclusion(DEFAULT_BLOCKED_OCCLUSION),
	m_hasUpdated(false),
	m_enabled(true),
	m_mutex("AudObstructionOcclusion", "m_mutex")
{}

AudObstructionOcclusion::~AudObstructionOcclusion()
{}

void AudObstructionOcclusion::Update()
{

	if (m_audioManager == nullptr || m_audioManager->GetState() != AudioState::Enabled)
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
		const bool exists = m_audioManager->WithCallbackGameObject(emitterID, [&culled](AudGameObjResource* emitter) {
			culled = emitter->IsCulled();
			});

		if (!exists)
		{
			it = m_emitters.erase(it);
			continue;
		}

		const bool occlusionChanged = state.occlusion.Advance(deltaSeconds, m_fadeRate);

		if (culled)
		{
			// The emitter is outside Wwise's view. Keep fading so time stays
			// consistent, and make sure the values are resent once the emitter wakes up.
			state.needsSend = true;
			++it;
			continue;
		}

		if (occlusionChanged || state.needsSend)
		{
			state.needsSend = !SendToWwise(emitterID, state);
		}


		++it;
	}
}

void AudObstructionOcclusion::SetEmitterBlocked(AkGameObjectID emitterID, bool blocked)
{
	// Occlusion is relative to the listener, so blocking the listener is meaningless.
	if (emitterID == LISTENER_GAME_OBJ_ID)
	{
		return;
	}

	if (!m_enabled)
	{
		return;
	}

	// When acoustics is on its transmission already attenuates, so treat everything
	// as clear to avoid stacking. Might change in the future with the addition of volumes.
	if (m_audioManager != nullptr && m_audioManager->GetSpatialAudioGeometryEnabled())
	{
		blocked = false;
	}

	CcpAutoMutex lock(m_mutex);

	auto it = m_emitters.find(emitterID);
	if (it == m_emitters.end())
	{
		// Do not track emitters that have never been blocked; most never are.
		if (!blocked)
		{
			return;
		}

		// If the emitter does not exist yet the entry is dropped by the next
		// Update(); the game's regular feed re-establishes it once it does.
		EmitterState& state = m_emitters[emitterID];
		state.blocked = true;
		// The first value snaps: an emitter that starts out behind cover is
		// muffled immediately instead of fading in from clear.
		state.occlusion.SetTarget(m_blockedOcclusion);
		state.occlusion.SnapToTarget();
		return;
	}

	it->second.blocked = blocked;
	it->second.occlusion.SetTarget(blocked ? m_blockedOcclusion : 0.0f);
}

float AudObstructionOcclusion::GetEmitterOcclusion(AkGameObjectID emitterID) const
{
	CcpAutoMutex lock(m_mutex);

	auto it = m_emitters.find(emitterID);
	if (it == m_emitters.end())
	{
		return 0.0f;
	}

	return it->second.occlusion.currentValue;
}

bool AudObstructionOcclusion::SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const
{
	const AKRESULT result = AK::SoundEngine::SetObjectObstructionAndOcclusion(
		emitterID,
		LISTENER_GAME_OBJ_ID,
		0.0f,
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
	ClearAllTargetsLocked();
}

void AudObstructionOcclusion::ClearAllTargetsLocked()
{
	for (auto& pair : m_emitters)
	{
		pair.second.blocked = false;
		pair.second.occlusion.SetTarget(0.0f);
	}
}

bool AudObstructionOcclusion::IsEnabled() const
{
	return m_enabled;
}

void AudObstructionOcclusion::SetEnabled(bool value)
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

float AudObstructionOcclusion::GetFadeRate() const
{
	return m_fadeRate;
}

void AudObstructionOcclusion::SetFadeRate(float value)
{
	m_fadeRate = std::max(value, 0.0f);
}

float AudObstructionOcclusion::GetBlockedOcclusion() const
{
	return m_blockedOcclusion;
}

void AudObstructionOcclusion::SetBlockedOcclusion(float value)
{
	CcpAutoMutex lock(m_mutex);
	m_blockedOcclusion = std::clamp(value, 0.0f, 1.0f);

	// QA tooling tunes this live, so emitters already blocked follow it.
	for (auto& pair : m_emitters)
	{
		if (pair.second.blocked)
		{
			pair.second.occlusion.SetTarget(m_blockedOcclusion);
		}
	}
}

void AudObstructionOcclusion::FadingValue::SetTarget(float target)
{
	targetValue = std::clamp(target, 0.0f, 1.0f);
}

bool AudObstructionOcclusion::FadingValue::Advance(float deltaSeconds, float fadeRate)
{
	if (currentValue == targetValue)
	{
		return false;
	}

	if (fadeRate <= 0.0f)
	{
		currentValue = targetValue;
		return true;
	}

	const float oldValue = currentValue;
	const float step = fadeRate * deltaSeconds;

	if (oldValue > targetValue)
	{
		currentValue = std::clamp(oldValue - step, targetValue, oldValue);
	}
	else
	{
		currentValue = std::clamp(oldValue + step, oldValue, targetValue);
	}
	return currentValue != oldValue;
}
