////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

struct Vector3;

/**
 * @brief Interface destiny implements for CarbonAudio's line of sight occlusion.
 *
 * CarbonAudio sends the listener position, the emitter positions and the ball each emitter is mounted on
 * straight to destiny and gets back whether each emitter is blocked.
 *
 * Destiny's ballpark is the implementer, set from script as AudManager::obstructionQuery. Destiny has a copy
 * of this header. Blue matches interfaces by name only, so the method list must never change under this
 * name: a mismatched pair of builds would still connect and call through the wrong vtable slot. Changing the
 * signature means a new interface name, the way this one replaced IEveObstructionQuery.
 */
BLUE_INTERFACE( IEveObstructionQuery2 ) : public IRoot
{
	/**
	 * @brief Reports which sightlines from one source to many targets are blocked.
	 *
	 * @param source        Listener position, audio space.
	 * @param targets       targetCount emitter positions, audio space.
	 * @param targetParents targetCount entries, the ball each emitter is mounted on as trinity handed it to
	 *                      audio, or null when the emitter is free. That ball's geometry never blocks its own
	 *                      emitter's sightline. Only valid for the duration of the call.
	 * @param targetCount   Number of entries in targets, targetParents and outBlocked.
	 * @param outBlocked    On success every entry is written: true when a body blocks the sightline. Bodies
	 *                      that must never occlude, such as the player's own ship, are skipped.
	 *
	 * @return False when there is no answer right now (no world loaded, mid-transition). It means "ask
	 *         again", not "nothing is blocked": the caller keeps its previous results and retries.
	 */
	virtual bool QuerySightlines( const Vector3& source, const Vector3* targets, IRoot* const* targetParents, unsigned int targetCount, bool* outBlocked ) = 0;
};
