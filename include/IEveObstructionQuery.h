////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include "Blue.h"

struct Vector3;

/**
 * @brief Sightline oracle the game plugs into CarbonAudio so the engine can judge line-of-sight
 *        occlusion itself, at the moment a voice starts and periodically while it plays.
 *
 * The physics module implements this on the object that owns the world (destiny's Ballpark) and the
 * game assigns that object to AudManager::obstructionQuery once per session. CarbonAudio never links
 * against the implementer: Blue resolves the interface at runtime by name.
 *
 * This header is hand-copied into the implementing module, exactly like trinity's IEveBallpark.
 * A Blue interface's identity is its name, so the two copies must declare the same method list.
 * Append-only: never reorder, remove or change an existing method. Add new ones at the end, or
 * introduce IEveObstructionQuery2.
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
