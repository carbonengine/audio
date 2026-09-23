////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

struct Vector3;

/**
 * @brief Sightline query the game gives CarbonAudio so the engine can check line-of-sight
 *        occlusion itself, when a voice starts and periodically while it plays.
 *
 * The physics module implements this on the object that owns the world (destiny's Ballpark) and the
 * game assigns that object to AudManager::obstructionQuery once per session. CarbonAudio never links
 * against the implementer: Blue resolves the interface at runtime by name.
 *
 * This header is copied as-is into the implementing module, the same way destiny shares IEveBallpark
 * with trinity. It includes nothing so it compiles in either module; the includer provides Blue.
 * Blue identifies an interface by its name, so both copies must have the same methods. Any change to
 * the method list, including adding one, needs a new name (IEveObstructionQuery2) so an old copy
 * fails the interface cast instead of calling the wrong method.
 */
BLUE_INTERFACE( IEveObstructionQuery ) : public IRoot
{
	/**
	 * @brief Reports which sightlines from one source to many targets are blocked.
	 *
	 * @param source      Listener position, in audio space (the same frame emitters are positioned in).
	 * @param targets     targetCount emitter positions, audio space.
	 * @param targetCount Number of entries in targets and outBlocked.
	 * @param outBlocked  On success every entry is written: true when a body lies on the sightline, false
	 *                    when it is clear. The implementer excludes bodies that must never occlude, such
	 *                    as the player's own ship.
	 *
	 * @return False when there is no answer right now (no world loaded, mid-transition). The caller
	 *         keeps its previous results and asks again next tick.
	 */
	virtual bool QuerySightlines( const Vector3& source, const Vector3* targets, unsigned int targetCount, bool* outBlocked ) = 0;
};
