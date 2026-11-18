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
 * object keeps its Wwise position, and the coordinates Wwise gets stay small enough for its single
 * precision paths.
 *
 * Right-handed, like trinity. Thread safe.
 */
class AudWorldFrame
{
public:
	/// How far the scene origin gets from the Wwise origin before the Wwise origin moves to it, in game units.
	static constexpr double DEFAULT_REBASE_DISTANCE = 50000.0;

	/// A scene origin step longer than this is a warp or a jump, not flight. The Wwise origin waits for the
	/// player to arrive, since following would send everything still in the world again on every frame.
	static constexpr double TRANSIT_STEP = 20000.0;

	struct State
	{
		AkVector64 sceneOrigin;
		AkVector64 wwiseOrigin;
		/// Whether trinity has set a scene origin yet. Until it does, both origins are zero.
		bool hasSceneOrigin;
		/// The first scene origin trinity set. Positions handed over before it are relative to it.
		AkVector64 firstSceneOrigin;
	};

	static AudWorldFrame& Get();

	/**
	 * @brief Sets where the scene origin is, in absolute world coordinates.
	 *
	 * Trinity calls it once per frame, before it hands over that frame's positions. The first call
	 * puts the Wwise origin there too, since every position so far was relative to the scene origin.
	 */
	void SetSceneOrigin( const AkVector64& origin );

	State GetState() const;

	/// What to add to a scene-relative position to get its Wwise position.
	AkVector64 GetSceneToWwise() const;

	/// A scene-relative position in Wwise's frame, still right-handed.
	AkVector64 ToWwise( const Vector3& sceneRelative ) const;

	/// A scene-relative transform with its translation moved into Wwise's frame.
	Matrix ToWwise( const Matrix& sceneRelative ) const;

	/**
	 * @brief The shortest move that counts as one, for something trinity hands over at sceneRelative.
	 *
	 * Trinity's positions are floats relative to the scene origin, so far things jitter by a few float
	 * steps whenever the player moves, and a far thing moving a little sounds the same. 1 mm, or 0.01%
	 * of the distance from the scene origin, whichever is longer.
	 */
	static double StillDistance( const Matrix& sceneRelative );

	/// 0 moves the Wwise origin along with the scene origin, so Wwise gets positions as trinity hands them.
	/// Infinity never moves it, so Wwise gets absolute coordinates.
	double GetRebaseDistance() const;
	void SetRebaseDistance( double distance );

	/// Changes whenever the scene-to-Wwise offset changes, so positions sent before need sending again.
	uint64_t GetOffsetGeneration() const;

	/// Times the Wwise origin jumped to the scene origin.
	uint64_t GetRebaseCount() const;

private:
	AudWorldFrame();

	mutable CcpMutex m_mutex;
	AkVector64 m_sceneOrigin;
	AkVector64 m_wwiseOrigin;
	AkVector64 m_firstSceneOrigin;
	bool m_hasSceneOrigin;
	double m_rebaseDistance;
	uint64_t m_offsetGeneration;
	uint64_t m_rebaseCount;
};
