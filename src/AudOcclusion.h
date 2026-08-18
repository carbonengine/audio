// Copyright © 2026 CCP ehf.

#pragma once

#include "Audio2.h"

#include <AK/SoundEngine/Common/AkTypes.h>

class AudObstructionOcclusion;

/**
 * @brief Blue facade for Carbon Audio's game-fed line-of-sight occlusion.
 *
 * The game runs its own line-of-sight queries against physics collision data and
 * feeds the per-emitter result here (@c SetEmitterBlocked), typically at 5-10 Hz.
 * The engine keeps the per-emitter occlusion state machine: a blocked emitter's
 * occlusion fades towards @c blockedOcclusion and a cleared one back towards 0 at
 * @c fadeRate every audio update (see @c AudObstructionOcclusion).
 *
 * This is a view onto the subsystem @c AudManager owns and updates, not a second
 * instance: every call forwards to the manager's @c AudObstructionOcclusion. Calls
 * made before a manager exists are safe no-ops.
 *
 * @see AudObstructionOcclusion
 */
BLUE_CLASS( AudOcclusion ) : public IRoot
{
public:
	AudOcclusion( IRoot* lockobj = NULL );

	EXPOSE_TO_BLUE();

	/**
	 * @brief Whether occlusion processing happens at all.
	 *
	 * Disabling fades every tracked emitter back to clear and ignores feeds until
	 * re-enabled; the game's regular feed then re-establishes the blocked states.
	 */
	bool GetEnabled() const;
	void SetEnabled( bool value );

	/**
	 * @brief How fast occlusion values move towards their target, in units per second.
	 *
	 * 0 applies new values instantaneously.
	 */
	float GetFadeRate() const;
	void SetFadeRate( float value );

	/**
	 * @brief The occlusion a blocked emitter fades towards.
	 *
	 * Kept below 1.0 so blocked sounds stay muffled instead of silenced by the
	 * Wwise occlusion curve. Changing it re-targets every currently blocked emitter.
	 */
	float GetBlockedOcclusion() const;
	void SetBlockedOcclusion( float value );

	/**
	 * @brief Sets whether the game considers an emitter's line of sight blocked.
	 *
	 * The emitter keeps this state until the next feed, @c ClearAllBlocked(), or its
	 * destruction; the engine never times it out or clears it on its own. Safe for
	 * emitter IDs the engine does not know (yet); the game's next feed re-establishes
	 * the state once the emitter exists.
	 *
	 * @param emitterID The emitter whose blocked state the game determined.
	 * @param blocked   True if the emitter's line of sight to the listener is blocked.
	 */
	void SetEmitterBlocked( AkGameObjectID emitterID, bool blocked );

	/**
	 * @brief Fades every emitter back to clear, for a session change or system jump.
	 */
	void ClearAllBlocked();

	/**
	 * @brief The occlusion currently applied to an emitter.
	 *
	 * This is the live, mid-fade value rather than the target, so it can be used to
	 * observe a fade in progress.
	 *
	 * @return The emitter's occlusion, or 0.0 if it is clear or is not tracked.
	 */
	float GetEmitterOcclusion( AkGameObjectID emitterID ) const;

private:
	/**
	 * @brief The manager-owned subsystem this forwards to.
	 *
	 * @return The subsystem, or nullptr before an audio manager exists.
	 */
	static AudObstructionOcclusion* Occlusion();
};

TYPEDEF_BLUECLASS( AudOcclusion );
