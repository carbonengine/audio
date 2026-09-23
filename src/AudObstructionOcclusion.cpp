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

AudObstructionOcclusion::AudObstructionOcclusion(AudManager* audioManager)	:
	m_audioManager(audioManager),
	m_fadeRate(DEFAULT_FADE_RATE),
	m_hasUpdated(false),
	m_enabled(true),
	m_mutex("AudObstructionOcclusion", "m_mutex"),
	m_hasRefreshed(false),
	m_blockedCapacity(0)
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
		}
		else if (obstructionChanged || occlusionChanged || state.needsSend)
		{
			state.needsSend = !SendToWwise(emitterID, state);
		}

		// A clear entry with nothing left to send carries no information: Wwise already holds clear
		// for it, and a culled game object comes back from Wake() registered clear. Drop it so the
		// walk above only ever visits emitters that are occluded or fading.
		if (state.AtRestClear() && (!state.needsSend || culled))
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

	// Fade only while something audible could pop. A first value on an emitter that is already
	// playing fades in like any other change.
	m_emitters[emitterID].SetTargets(obstruction, occlusion, culled || !playing);

	return true;
}

bool AudObstructionOcclusion::SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage)
{
	return SetObstructionOcclusion(emitterID, 0.0f, OcclusionForBlockage(blockage));
}

float AudObstructionOcclusion::OcclusionForBlockage(float blockage) const
{
	// When Acoustics is On its transmission already attenuates, so skip occlusion to avoid stacking.
	// Might change in the future with the addition of volumes.
	const bool acousticsEnabled = m_audioManager != nullptr && m_audioManager->GetSpatialAudioGeometryEnabled();
	return acousticsEnabled ? 0.0f : blockage;
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
	if (refreshDue)
	{
		m_lastRefreshTime = now;
		m_hasRefreshed = true;
	}

	// Collect under the prioritization lock, copying out only what the oracle needs, so no emitter
	// pointer outlives the lock. Emitters a voice just started on are judged every tick; everything
	// the listener can hear is re-judged on the refresh interval. Silent emitters are left alone,
	// they are judged the moment they start playing. Culled emitters are not awake, so they keep
	// their stored value until they wake and play. Emitters out of listener range or playing 2D are
	// skipped: Wwise renders neither positionally, and rays to far emitters cross the most geometry.
	// An out-of-range onset keeps its flag and is judged the tick it comes into range.
	m_candidates.clear();
	m_targets.clear();
	m_audioManager->ForEachAwakeAudioEmitter([&](AudGameObjResource* emitter)
	{
		const bool refresh = refreshDue && emitter->HasPlayingVoices();
		if (!refresh && !emitter->IsOcclusionOnsetPending())
		{
			return;
		}
		if (!emitter->HasUsableWorldPosition())
		{
			// Leave any onset flag armed; the emitter is judged once it has a position.
			return;
		}
		if (emitter->IsPlaying2DSound())
		{
			// Nothing positional to occlude. Drop the onset; a later 3D voice starting on silence re-arms it.
			emitter->TakeOcclusionOnsetPending();
			return;
		}
		if (!emitter->IsListenerInRange(source))
		{
			return;
		}
		const bool onset = emitter->TakeOcclusionOnsetPending();
		if (!onset && !refresh)
		{
			// The flag was cleared between the peek and the take: the voice already ended.
			return;
		}
		m_candidates.push_back({ emitter->GetID(), onset });
		m_targets.push_back(emitter->GetPosition());
	});

	const size_t count = m_candidates.size();
	if (count == 0)
	{
		if (refreshDue)
		{
			// A refresh with nothing audible to judge: the record says so.
			CcpAutoMutex lock(m_mutex);
			m_lastVerdicts.clear();
		}
		return;
	}

	if (count > m_blockedCapacity)
	{
		m_blocked = std::make_unique<bool[]>(count);
		m_blockedCapacity = count;
	}

	// No CarbonAudio lock is held while the game does its geometry.
	const bool answered = oracle->QuerySightlines(source, m_targets.data(), static_cast<unsigned int>(count), m_blocked.get());
	if (!answered)
	{
		// Keep the previous verdicts. Re-arm the onsets so they are judged on the next tick that can answer.
		for (const Candidate& candidate : m_candidates)
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

	const float blockedOcclusion = OcclusionForBlockage(BLOCKED_OCCLUSION);

	// Record and apply under one hold. A refresh judged everything audible, so it replaces the record;
	// an onset-only tick adds to it. Every candidate was awake and existed when collected; one destroyed
	// since leaves an entry that the fade loop, which runs next, drops.
	CcpAutoMutex lock(m_mutex);
	if (refreshDue)
	{
		m_lastVerdicts.clear();
	}
	for (size_t i = 0; i < count; ++i)
	{
		const Candidate& candidate = m_candidates[i];
		const bool blocked = m_blocked[i];
		m_lastVerdicts[candidate.id] = blocked;
		// A clear verdict only reaches emitters that already have an entry: Wwise holds clear for the rest.
		const auto entry = blocked ? m_emitters.try_emplace(candidate.id).first : m_emitters.find(candidate.id);
		if (entry != m_emitters.end())
		{
			// A candidate that is not an onset was playing when collected, so only an onset snaps.
			entry->second.SetTargets(0.0f, blocked ? blockedOcclusion : 0.0f, candidate.onset);
		}
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

std::map<AkGameObjectID, bool> AudObstructionOcclusion::GetLastSightlineVerdicts() const
{
	CcpAutoMutex lock(m_mutex);
	return m_lastVerdicts;
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
	m_lastVerdicts.erase(emitterID);
}

void AudObstructionOcclusion::Reset()
{
	CcpAutoMutex lock(m_mutex);
	m_emitters.clear();
	m_lastVerdicts.clear();
	m_hasUpdated = false;
	m_hasRefreshed = false;
}

void AudObstructionOcclusion::ClearAll()
{
	CcpAutoMutex lock(m_mutex);
	// The verdicts belong to whatever was judged before; the next pass that runs records its own.
	m_lastVerdicts.clear();
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
