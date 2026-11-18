////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <AK/SoundEngine/Common/Ak3DObjects.h>

#include <CcpMutex.h>

#include <cstdint>

/**
 * @brief Moves positions from trinity's scene into Wwise's world.
 *
 * Trinity's scene origin follows the player, so every position it hands audio shifts whenever the
 * player moves, including the positions of things that hold still. Sent to Wwise as they are, a mesh
 * that holds still looks like it moved on every tick, and Wwise rebuilds its scene for it.
 *
 * Audio adds the scene origin back and measures from a Wwise origin of its own, which only jumps to
 * the scene origin once the player is further than the rebase distance from it. Between jumps, a still
 * object keeps its Wwise position, and the coordinates Wwise gets stay small.
 *
 * A position is always the last one trinity handed over plus the current offset, so something trinity
 * stops handing over, because it moves along with the player, moves along in Wwise too.
 *
 * Right-handed, like trinity. Thread safe.
 */
class AudWorldFrame
{
public:
	/// How far the scene origin gets from the Wwise origin before the Wwise origin moves to it, in game units.
	static constexpr double DEFAULT_REBASE_DISTANCE = 50000.0;

	/// A scene origin step longer than this, from one frame to the next, is a warp or a jump. It is far beyond any
	/// ship's speed at any frame rate and far below warp speed. The Wwise origin waits for the player to slow down,
	/// since following would send everything still in the world again every few frames.
	static constexpr double TRANSIT_STEP = 2000.0;

	/// The shortest move that counts as one, in game units.
	static constexpr double MIN_STILL_DISTANCE = 1.0e-3;

	struct Offset
	{
		/// What to add to a scene-relative position to get its Wwise position.
		AkVector64 sceneToWwise;
		/// Changes whenever sceneToWwise does.
		uint64_t generation;
		/// Times the Wwise origin jumped to the scene origin.
		uint64_t rebaseCount;
	};

	static AudWorldFrame& Get();

	/**
	 * @brief Sets where the scene origin is, in absolute world coordinates.
	 *
	 * Trinity calls it once per frame, before it hands over that frame's positions. The first call
	 * puts the Wwise origin there too, since every position so far was relative to the scene origin.
	 */
	void SetSceneOrigin( const AkVector64& origin );

	Offset GetOffset() const;

	/**
	 * @brief The shortest move that counts as one, for something trinity hands over at sceneRelative.
	 *
	 * Trinity's positions are floats relative to the scene origin, so far things jitter by a few float
	 * steps whenever the player moves, and a far thing moving a little sounds the same.
	 * MIN_STILL_DISTANCE, or 0.01% of the distance from the scene origin, whichever is longer.
	 */
	static double StillDistance( const Matrix& sceneRelative );

	/// 0 moves the Wwise origin to the scene origin on every frame the player isn't in transit, so Wwise gets
	/// positions relative to the player. Infinity never moves it from where the first scene origin was.
	double GetRebaseDistance() const;
	void SetRebaseDistance( double distance );

private:
	AudWorldFrame();

	mutable CcpMutex m_mutex;
	AkVector64 m_sceneOrigin;
	AkVector64 m_wwiseOrigin;
	bool m_hasSceneOrigin;
	double m_rebaseDistance;
	uint64_t m_offsetGeneration;
	uint64_t m_rebaseCount;
};
