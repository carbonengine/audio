////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include <chrono>
#include <unordered_map>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

class AudManager;

/**
 * @brief Owns the occlusion of every emitter and feeds those values to Wwise.
 *
 * The blocked/clear decision is the game's: it runs its own line-of-sight queries
 * against physics collision data and feeds the result here per emitter
 * (@c SetEmitterBlocked), typically at 5-10 Hz. This class keeps the per-emitter
 * occlusion state machine - a blocked emitter's occlusion fades towards
 * @c GetBlockedOcclusion() and a cleared one back towards 0 at @c GetFadeRate()
 * every audio update - and forwards the live values to Wwise
 * (see @c AK::SoundEngine::SetObjectObstructionAndOcclusion).
 *
 * The engine never times out or auto-clears a blocked emitter on its own; an
 * emitter keeps its last fed state until the game feeds it again, clears
 * everything (@c ClearAll), or the emitter is destroyed. Feeds are skipped while
 * spatial audio geometry (acoustics) is enabled, whose transmission already
 * attenuates; stacking the two would double-muffle.
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
	 * @brief Sets whether the game considers an emitter's line of sight blocked.
	 *
	 * blocked makes the emitter's occlusion target @c GetBlockedOcclusion(); clear
	 * makes it 0. The live value fades towards the target at @c GetFadeRate(). An
	 * emitter's very first blocked value snaps instead of fading, so an emitter that
	 * starts out behind cover is muffled immediately rather than fading in from clear.
	 *
	 * Safe for any emitterID: an emitter the manager does not know is dropped on the
	 * next update, and the game's next feed re-establishes it once the emitter exists.
	 * Ignored while disabled or while acoustics is on.
	 *
	 * @param emitterID The emitter whose blocked state the game determined.
	 * @param blocked   True if the emitter's line of sight to the listener is blocked.
	 */
	void SetEmitterBlocked(AkGameObjectID emitterID, bool blocked);

	/**
	 * @brief The occlusion currently applied to an emitter.
	 */
	float GetEmitterOcclusion(AkGameObjectID emitterID) const;

	/// Drops an emitter straight away without fading it out, for when the game object goes away.
	void RemoveEmitter( AkGameObjectID emitterID );

	/// Forgets every emitter and the fade clock, for when audio is disabled.
	void Reset();

	/// Fades every tracked emitter back to clear, for a session change or system jump.
	void ClearAll();

	/**
	 * @brief Controls whether game driven occlusion is processed at all.
	 *
	 * Disabling fades every tracked emitter back to clear and ignores feeds until
	 * re-enabled; the game's regular feed then re-establishes the blocked states.
	 */
	bool IsEnabled() const;
	void SetEnabled(bool value);

	/**
	 * @brief How fast values move towards their target, in units per second.
	 */
	float GetFadeRate() const;
	void SetFadeRate(float value);

	/**
	 * @brief The occlusion a blocked emitter fades towards.
	 *
	 * Kept below 1.0 so blocked sounds stay muffled instead of silenced by the
	 * Wwise occlusion curve. Changing it re-targets every currently blocked emitter.
	 */
	float GetBlockedOcclusion() const;
	void SetBlockedOcclusion(float value);

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

	/// Everything we keep for one emitter the game has ever reported blocked.
	struct EmitterState
	{
		FadingValue occlusion;

		// The game's last word on this emitter, kept so a blockedOcclusion change
		// can re-target the emitters it applies to.
		bool blocked = false;

		bool needsSend = true;

	};

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	/// Fade every tracked emitter back to clear. Callers must hold m_mutex.
	void ClearAllTargetsLocked();

	static constexpr float DEFAULT_FADE_RATE = 0.5f;
	static constexpr float DEFAULT_BLOCKED_OCCLUSION = 0.35f;

	AudManager* m_audioManager;
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	float m_fadeRate;
	float m_blockedOcclusion;
	bool m_hasUpdated;
	bool m_enabled;
	mutable CcpMutex m_mutex;

	std::chrono::steady_clock::time_point m_lastUpdateTime;

};
