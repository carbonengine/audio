////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#include "AudObstructionOcclusion.h"
#include "AudGameObjResource.h"
#include "AudManager.h"
#include "IEveObstructionQuery.h"

#include <algorithm>
#include <vector>

namespace
{
	/// One emitter the sightline pass is about to ask the oracle about.
	struct Candidate
	{
		AkGameObjectID id;
		Vector3 position;
		// A voice just started on a silent emitter: judged every tick and applied without a fade.
		bool onset;
	};
}

AudObstructionOcclusion::AudObstructionOcclusion(AudManager* audioManager)	:
	m_audioManager(audioManager),
	m_fadeRate(DEFAULT_FADE_RATE),
	m_hasUpdated(false),
	m_enabled(true),
	m_mutex("AudObstructionOcclusion", "m_mutex"),
	m_hasRefreshed(false)
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

	// Runs before the fade loop and before AudManager renders the frame, so a voice that started
	// this frame is judged, snapped and sent to Wwise ahead of its first buffer.
	RunSightlinePass(now);

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

		const bool obstructionChanged = state.obstruction.Advance(deltaSeconds, m_fadeRate);
		const bool occlusionChanged = state.occlusion.Advance(deltaSeconds, m_fadeRate);

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


		++it;
	}
}

bool AudObstructionOcclusion::SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion, bool snap)
{
	if (!m_enabled)
	{
		return false;
	}

	if (m_audioManager == nullptr || m_audioManager->GetState() != AudioState::Enabled)
	{
		return false;
	}

	// Values are relative to the listener, so setting them on the listener itself is meaningless.
	if (emitterID == LISTENER_GAME_OBJ_ID)
	{
		return false;
	}

	// We need to ask AudioManager about emitters that actually exist. While we have it, learn whether
	// anything is audible on it: a fade only makes sense when there is a sound to protect from popping.
	bool culled = false;
	bool playing = false;
	const bool exists = m_audioManager->WithCallbackGameObject(emitterID, [&culled, &playing](AudGameObjResource* emitter) {
		culled = emitter->IsCulled();
		playing = emitter->HasPlayingVoices();
		});
	if (!exists)
	{
		return false;
	}

	CcpAutoMutex lock(m_mutex);

	const auto [entry, isNewEmitter] = m_emitters.try_emplace(emitterID);
	EmitterState& state = entry->second;

	state.obstruction.SetTarget(obstruction);
	state.occlusion.SetTarget(occlusion);

	if (isNewEmitter || snap || culled || !playing)
	{
		state.obstruction.SnapToTarget();
		state.occlusion.SnapToTarget();
		// Update() only sends a value a fade has moved, and a snapped value has nothing left to move.
		// Flag it so the next Update() hands it to Wwise anyway.
		state.needsSend = true;
	}

	return true;
}

bool AudObstructionOcclusion::SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage, bool snap)
{
	// When Acoustics is On its transmission already attenuates, so skip occlusion to avoid stacking.
	// Might change in the future with the addition of volumes.
	const bool acousticsEnabled = m_audioManager != nullptr && m_audioManager->GetSpatialAudioGeometryEnabled();

	float occlusion = 0.0f;
	if (!acousticsEnabled)
	{
		occlusion = blockage;
	}

	return SetObstructionOcclusion(emitterID, 0.0f, occlusion, snap);
}

void AudObstructionOcclusion::RunSightlinePass(std::chrono::steady_clock::time_point now)
{
	if (!m_enabled || m_audioManager == nullptr)
	{
		return;
	}

	// Hold our own reference for the duration of the pass: script may reassign the oracle at any time.
	IEveObstructionQueryPtr oracle = m_audioManager->GetObstructionQuery();
	if (oracle == nullptr)
	{
		return;
	}

	AudListenerPtr listener = m_audioManager->GetListener();
	if (listener == nullptr || !listener->HasUsableWorldPosition())
	{
		return;
	}
	const Vector3 source = listener->GetPosition();

	const bool refreshDue = !m_hasRefreshed
		|| std::chrono::duration<float>(now - m_lastRefreshTime).count() >= REFRESH_INTERVAL;

	// Emitters a voice just started on are judged every tick; everything audible is re-judged on the
	// refresh interval. Silent emitters are left alone, they are judged the moment they start playing.
	// Culled emitters are not awake, so they keep their stored value until they wake and play.
	std::vector<Candidate> candidates;
	for (AudGameObjResource* emitter : m_audioManager->GetAwakeAudioEmitters())
	{
		if (!emitter->HasUsableWorldPosition())
		{
			// Leave any onset flag armed; the emitter is judged once it has a position.
			continue;
		}

		const bool onset = emitter->TakeOcclusionOnsetPending();
		if (onset || (refreshDue && emitter->HasPlayingVoices()))
		{
			candidates.push_back({ emitter->GetID(), emitter->GetPosition(), onset });
		}
	}

	if (refreshDue)
	{
		m_lastRefreshTime = now;
		m_hasRefreshed = true;
	}

	if (candidates.empty())
	{
		return;
	}

	std::vector<Vector3> targets;
	targets.reserve(candidates.size());
	for (const Candidate& candidate : candidates)
	{
		targets.push_back(candidate.position);
	}
	std::vector<unsigned long long> blockers(candidates.size(), 0);

	// No CarbonAudio lock is held while the game does its geometry.
	const bool answered = oracle->QuerySightlines(source, targets.data(), static_cast<unsigned int>(targets.size()), blockers.data());
	if (!answered)
	{
		// Keep the previous verdicts. Re-arm the onsets so they are judged on the next tick that can answer.
		for (const Candidate& candidate : candidates)
		{
			if (candidate.onset)
			{
				m_audioManager->WithCallbackGameObject(candidate.id, [](AudGameObjResource* emitter) {
					emitter->MarkOcclusionOnsetPending();
					});
			}
		}
		return;
	}

	for (size_t i = 0; i < candidates.size(); ++i)
	{
		const float blockage = blockers[i] != 0 ? BLOCKED_OCCLUSION : 0.0f;
		// An emitter destroyed since the snapshot is rejected as unknown, which is fine.
		SetEmitterLineOfSightBlockage(candidates[i].id, blockage, candidates[i].onset);
	}
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
	m_hasRefreshed = false;
}

void AudObstructionOcclusion::ClearAll()
{
	CcpAutoMutex lock(m_mutex);
	for (auto& pair : m_emitters)
	{
		pair.second.obstruction.SetTarget(0.0f);
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
