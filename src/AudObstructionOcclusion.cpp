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

	// Before the fade loop, so a voice that started this frame gets its occlusion sent to Wwise
	// before its first buffer.
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

		// Wwise already has it as clear, and a culled emitter comes back clear from Wake(), so stop tracking it.
		if (state.IsClear() && (!state.needsSend || culled))
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

	// We need to ask AudioManager about emitters that actually exist.
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

	// Only fade while something is playing, silent or culled emitters snap.
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

	// Keep our own reference, script can reassign the query at any time.
	IEveObstructionQueryPtr query = m_audioManager->GetObstructionQuery();
	if (query == nullptr)
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

	// Copy out ids and positions under the prioritization lock so no emitter pointer is kept after it.
	// Emitters that just started playing are checked every tick, everything audible on the refresh
	// interval. Out of range and 2D emitters are skipped since Wwise doesn't render them positionally.
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
			// No position yet, keep the onset flag so it gets checked once it has one.
			return;
		}
		if (emitter->IsPlaying2DSound())
		{
			// Nothing to occlude on a 2D sound, drop the onset flag.
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
			// The voice ended between the two reads of the flag.
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
			// Nothing audible on a refresh, so clear the last results.
			CcpAutoMutex lock(m_mutex);
			m_lastResults.clear();
		}
		return;
	}

	if (count > m_blockedCapacity)
	{
		m_blocked = std::make_unique<bool[]>(count);
		m_blockedCapacity = count;
	}

	// No audio lock is held while the game runs its ray tests.
	const bool answered = query->QuerySightlines(source, m_targets.data(), static_cast<unsigned int>(count), m_blocked.get());
	if (!answered)
	{
		// No answer this tick. Keep the last results and flag the onsets again so they get checked next tick.
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

	// A refresh checked everything audible so it replaces the last results, an onset-only tick adds to them.
	// An emitter destroyed since it was collected leaves an entry that the fade loop drops.
	CcpAutoMutex lock(m_mutex);
	if (refreshDue)
	{
		m_lastResults.clear();
	}
	for (size_t i = 0; i < count; ++i)
	{
		const Candidate& candidate = m_candidates[i];
		const bool blocked = m_blocked[i];
		m_lastResults[candidate.id] = blocked;
		// A clear result only updates emitters we already track, Wwise has the rest as clear.
		const auto entry = blocked ? m_emitters.try_emplace(candidate.id).first : m_emitters.find(candidate.id);
		if (entry != m_emitters.end())
		{
			// Only onsets snap, anything else was already playing when collected.
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

std::map<AkGameObjectID, bool> AudObstructionOcclusion::GetLastSightlineResults() const
{
	CcpAutoMutex lock(m_mutex);
	return m_lastResults;
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
	m_lastResults.erase(emitterID);
}

void AudObstructionOcclusion::Reset()
{
	CcpAutoMutex lock(m_mutex);
	m_emitters.clear();
	m_lastResults.clear();
	m_hasUpdated = false;
	m_hasRefreshed = false;
}

void AudObstructionOcclusion::ClearAll()
{
	CcpAutoMutex lock(m_mutex);
	m_lastResults.clear();
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
