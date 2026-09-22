////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include <chrono>
#include <unordered_map>
#include <vector>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

class AudManager;
struct Vector3;

/**
 * @brief Owns the obstruction and occlusion of every emitter and feeds those values to Wwise.
 *
 * Blockage arrives in one of two ways. When the game has plugged a sightline oracle into
 * AudManager (see IEveObstructionQuery.h) this class asks it itself: every tick for each emitter a
 * voice just started on, and every REFRESH_INTERVAL for each emitter the listener can hear. Emitters
 * out of listener range or playing 2D are not asked about, Wwise does not render them positionally
 * and long rays are the expensive ones. Otherwise the game feeds values directly through
 * SetEmitterLineOfSightBlockage.
 *
 * Either way the value becomes the obstruction and occlusion Wwise applies
 * (see @c AK::SoundEngine::SetObjectObstructionAndOcclusion). A value fades towards its target
 * while the emitter is audible, so sounds do not pop when something moves in front of them, and
 * snaps when nothing is playing, so the next voice starts at the right level. Only emitters that
 * are occluded, fading or waiting to be sent are tracked; an entry that comes to rest at clear is
 * dropped, so the per-tick work scales with what is occluded, not with what has ever played.
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
	 * @param emitterID   The emitter to block.
	 * @param obstruction Target obstruction [0.0, 1.0]
	 * @param occlusion   Target occlusion [0.0, 1.0]
	 * @param snap        Apply the targets at once instead of fading. Silent and culled emitters snap
	 *                    regardless, since there is nothing audible to pop.
	 *
	 * @return True if the emitter exists and the values were accepted.
	 */
	bool SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion, bool snap = false);

	/**
	 * @brief Sets how much of an emitter's line of sight to the listener is blocked.
	 *
	 * @param emitterID The emitter to block.
	 * @param blockage  How blocked the line of sight is [0.0, 1.0]. 0 is a clear line of sight.
	 * @param snap      See SetObstructionOcclusion.
	 *
	 * @return True if the emitter exists and the value was accepted.
	 */
	bool SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage, bool snap = false);

	/**
	 * @brief The occlusion currently applied to an emitter.
	 */
	float GetEmitterOcclusion(AkGameObjectID emitterID) const;

	/// Drops an emitter straight away without fading it out, for when the game object goes away.
	void RemoveEmitter( AkGameObjectID emitterID );

	/// Forgets every emitter and the fade clock, for when audio is disabled.
	void Reset();

	/// Fades every tracked emitter back to clear.
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

	/// Everything we keep for one emitter that is occluded, fading, or waiting to be sent to Wwise.
	struct EmitterState
	{
		FadingValue obstruction;
		FadingValue occlusion;

		bool needsSend = true;

		/// Clear with nothing left to fade: the entry says nothing Wwise does not already assume.
		bool AtRestClear() const
		{
			return obstruction.currentValue == 0.0f && obstruction.targetValue == 0.0f
				&& occlusion.currentValue == 0.0f && occlusion.targetValue == 0.0f;
		}
	};

	/// One emitter the sightline pass is asking the oracle about this tick.
	struct Candidate
	{
		AkGameObjectID id;
		// A voice just started while nothing was live on the emitter: the verdict is applied without a fade.
		bool onset;
	};

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	/// Asks the game's sightline oracle about every emitter that needs judging this tick and applies the answers.
	void RunSightlinePass(std::chrono::steady_clock::time_point now);

	/// Whether an entry exists for the emitter: it is occluded, fading, or waiting to be sent.
	bool IsTracked(AkGameObjectID emitterID) const;

	static constexpr float DEFAULT_FADE_RATE = 1.0f;
	/// Seconds between re-judging audible emitters. Emitters a voice just started on are judged every tick.
	static constexpr float REFRESH_INTERVAL = 0.2f;
	/// Occlusion applied to an emitter whose sightline the oracle reports blocked.
	static constexpr float BLOCKED_OCCLUSION = 1.0f;

	AudManager* m_audioManager;
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	float m_fadeRate;
	bool m_hasUpdated;
	bool m_enabled;
	mutable CcpMutex m_mutex;

	std::chrono::steady_clock::time_point m_lastUpdateTime;
	// Clock of the periodic re-judging of audible emitters, separate from the fade clock.
	bool m_hasRefreshed;
	std::chrono::steady_clock::time_point m_lastRefreshTime;

	// Scratch space for the sightline pass, kept between ticks so a pass allocates nothing once warm.
	// Only touched from the audio tick.
	std::vector<Candidate> m_candidates;
	std::vector<Vector3> m_targets;
	std::vector<unsigned long long> m_blockers;

};