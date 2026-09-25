////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

class AudManager;
struct Vector3;

/**
 * @brief Owns the obstruction and occlusion of every emitter and feeds those values to Wwise.
 *
 * Blockage comes from destiny when a sightline query is set on AudManager (see
 * IEveObstructionQuery.h), otherwise from the game through SetEmitterLineOfSightBlockage. Either
 * way it fades while the emitter is playing so sounds do not pop, and snaps when it is silent.
 */
class AudObstructionOcclusion
{
public:

	AudObstructionOcclusion(AudManager* audioManager);
	~AudObstructionOcclusion();

	/**
	 * @brief Advances every fade and sends the values that changed to Wwise.
	 */
	void Update();

	/**
	 * @brief Sets the obstruction and occlusion an emitter fades towards.
	 *
	 * Silent and culled emitters snap straight to the targets.
	 *
	 * @param emitterID   The emitter to block.
	 * @param obstruction Target obstruction [0.0, 1.0]
	 * @param occlusion   Target occlusion [0.0, 1.0]
	 *
	 * @return True if the emitter exists and the values were accepted.
	 */
	bool SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion);

	/**
	 * @brief Sets how much of an emitter's line of sight to the listener is blocked.
	 *
	 * @param emitterID The emitter to block.
	 * @param blockage  How blocked the line of sight is [0.0, 1.0]. 0 is a clear line of sight.
	 *
	 * @return True if the emitter exists and the value was accepted.
	 */
	bool SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage);

	/**
	 * @brief The occlusion currently applied to an emitter.
	 */
	float GetEmitterOcclusion(AkGameObjectID emitterID) const;

	/// Results of the last sightline pass, emitter id to blocked. Emitters that were not checked are missing.
	std::map<AkGameObjectID, bool> GetLastSightlineResults() const;

	/// Drops an emitter straight away without fading it out, for when the game object goes away.
	void RemoveEmitter( AkGameObjectID emitterID );

	/// Forgets every emitter and the fade clock, for when audio is disabled.
	void Reset();

	/// Fades every tracked emitter back to clear and clears the last sightline results.
	void ClearAll();

	/**
	 * @brief Controls whether game driven obstruction and occlusion is processed at all.
	 */
	bool IsEnabled() const;
	void SetEnabled(bool value);

	/**
	 * @brief How fast values move towards their target, in units per second.
	 */
	float GetFadeRate() const;
	void SetFadeRate(float value);

private:

	/// A single value on its way to a target, and the maths that moves it there.
	struct FadingValue
	{
		float currentValue = 0.0f;
		float targetValue = 0.0f;

		void SetTarget(float target);
		bool Advance(float deltaSeconds, float fadeRate);
		bool ReachedTarget() const { return currentValue == targetValue; };
		void SnapToTarget() { currentValue = targetValue; };
	};

	/// Everything we keep for one tracked emitter.
	struct EmitterState
	{
		FadingValue obstruction;
		FadingValue occlusion;

		bool needsSend = true;

		/// A snap jumps straight to the targets and marks the entry for sending, since Update() only
		/// sends what a fade changed.
		void SetTargets(float obstructionTarget, float occlusionTarget, bool snap)
		{
			obstruction.SetTarget(obstructionTarget);
			occlusion.SetTarget(occlusionTarget);
			if (snap)
			{
				obstruction.SnapToTarget();
				occlusion.SnapToTarget();
				needsSend = true;
			}
		}

		/// Both values are 0 and not fading.
		bool IsClear() const
		{
			return obstruction.currentValue == 0.0f && obstruction.targetValue == 0.0f
				&& occlusion.currentValue == 0.0f && occlusion.targetValue == 0.0f;
		}
	};

	/// An emitter the sightline pass checks this tick.
	struct Candidate
	{
		AkGameObjectID id;
		// First voice after silence, the result is applied without a fade.
		bool onset;
	};

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	/// Runs the game's sightline query for the emitters that need checking this tick and applies the results.
	void RunSightlinePass(std::chrono::steady_clock::time_point now);

	/// Converts a line-of-sight blockage into the occlusion to apply.
	float OcclusionForBlockage(float blockage) const;

	static constexpr float DEFAULT_FADE_RATE = 1.0f;
	/// Seconds between rechecks of audible emitters. Emitters that just started playing are checked every tick.
	static constexpr float REFRESH_INTERVAL = 0.2f;
	/// Occlusion applied when the sightline query reports an emitter blocked.
	static constexpr float BLOCKED_OCCLUSION = 1.0f;

	AudManager* m_audioManager;
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	// Last sightline results, see GetLastSightlineResults. Guarded by m_mutex.
	std::map<AkGameObjectID, bool> m_lastResults;
	float m_fadeRate;
	bool m_hasUpdated;
	bool m_enabled;
	mutable CcpMutex m_mutex;

	std::chrono::steady_clock::time_point m_lastUpdateTime;
	// Timer for rechecking audible emitters, separate from the fade timer.
	bool m_hasRefreshed;
	std::chrono::steady_clock::time_point m_lastRefreshTime;

	// Reused by the sightline pass so it does not allocate every tick. Audio tick only.
	// m_blocked is a plain array because std::vector<bool> can't give out a bool*.
	std::vector<Candidate> m_candidates;
	std::vector<Vector3> m_targets;
	std::unique_ptr<bool[]> m_blocked;
	size_t m_blockedCapacity;

};
