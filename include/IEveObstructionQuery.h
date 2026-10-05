////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

struct Vector3;

/**
 * @brief Interface destiny implements for CarbonAudio's line of sight occlusion.
 *
 * CarbonAudio sends the listener position and the emitter positions straight to destiny and gets
 * back whether each emitter is blocked.
 *
 * Destiny's ballpark is the implementer, set from script as AudManager::obstructionQuery.
 */
BLUE_INTERFACE( IEveObstructionQuery ) : public IRoot
{
	/**
	 * @brief Reports which sightlines from one source to many targets are blocked.
	 *
	 * @param source      Listener position, audio space.
	 * @param targets     targetCount emitter positions, audio space.
	 * @param targetCount Number of entries in targets and outBlocked.
	 * @param outBlocked  On success every entry is written: true when a body blocks the sightline. Bodies
	 *                    that must never occlude, such as the player's own ship, are skipped.
	 *
	 * @return False when there is no answer right now (no world loaded, mid-transition). It means "ask
	 *         again", not "nothing is blocked": the caller keeps its previous results and retries.
	 */
	virtual bool QuerySightlines( const Vector3& source, const Vector3* targets, unsigned int targetCount, bool* outBlocked ) = 0;
};
