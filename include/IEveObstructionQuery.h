////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

struct Vector3;

/**
 * @brief Sightline oracle the game plugs into CarbonAudio so the engine can judge line-of-sight
 *        occlusion itself, at the moment a voice starts and periodically while it plays.
 *
 * The physics module implements this on the object that owns the world (destiny's Ballpark) and the
 * game assigns that object to AudManager::obstructionQuery once per session. CarbonAudio never links
 * against the implementer: Blue resolves the interface at runtime by name.
 *
 * This header is hand-copied verbatim into the implementing module, the way destiny shares its
 * ballpark interface with trinity. It includes nothing itself so it compiles in either module's
 * include order; the includer provides Blue.
 * A Blue interface's identity is its name, so the copies must declare the same method list. Any
 * change to that list, including adding a method, renames the interface (IEveObstructionQuery2):
 * a stale copy then fails the interface cast instead of calling the wrong slot.
 */
BLUE_INTERFACE( IEveObstructionQuery ) : public IRoot
{
	/**
	 * @brief Reports which sightlines from one source to many targets are blocked.
	 *
	 * @param source        Listener position, in audio space (the same frame emitters are positioned in).
	 * @param targets       targetCount emitter positions, audio space.
	 * @param targetCount   Number of entries in targets and outBlockerIDs.
	 * @param outBlockerIDs Pre-zeroed by the caller. On success every entry is written: 0 for a clear
	 *                      sightline, otherwise the id of the first body that blocks it. The
	 *                      implementer excludes bodies that must never occlude, such as the player's own ship.
	 *
	 * @return False when no answer is possible right now (no world loaded, mid-transition). The caller
	 *         then keeps its previous verdicts and asks again next tick.
	 */
	virtual bool QuerySightlines( const Vector3& source, const Vector3* targets, unsigned int targetCount, unsigned long long* outBlockerIDs ) = 0;
};
