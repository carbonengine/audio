// Copyright © 2026 CCP ehf.

#pragma once

#include "Audio2.h"

#include <AK/SoundEngine/Common/AkTypes.h>

#include <vector>

class AudObstructionOcclusion;

/**
 * @brief Blue facade for Carbon Audio's sphere-based line-of-sight occlusion.
 *
 * The game registers the bounding spheres that can block sound plus the game world
 * position of the audio-space origin, and the engine computes each positioned
 * emitter's occlusion from the listener-emitter sightline
 * (see @c AudObstructionOcclusion).
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
	 * Disabling fades every tracked emitter back to clear and stops the sightline
	 * pass, leaving any registered occluders in place.
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
	 * @brief The occlusion applied to an emitter whose sightline a sphere blocks.
	 *
	 * Kept below 1.0 so occluded sounds stay muffled instead of silenced by the
	 * Wwise occlusion curve.
	 */
	float GetBlockedOcclusion() const;
	void SetBlockedOcclusion( float value );

	/**
	 * @brief What fraction of an occluder's radius counts as solid, in (0.0, 1.0].
	 *
	 * The game registers bounding spheres, which for anything but a sphere are much
	 * larger than the object inside them, so a sightline can clip one while passing
	 * well clear of the object itself. Shrinking the radius makes blockage require
	 * real overlap instead of a graze. 1.0 uses the radii as the game gave them.
	 */
	float GetRadiusScale() const;
	void SetRadiusScale( float value );

	/**
	 * @brief How often sphere line of sight is recomputed, in seconds.
	 *
	 * Fades still advance every update; only the ray tests are throttled.
	 * 0 recomputes every update.
	 */
	float GetLosRecomputeInterval() const;
	void SetLosRecomputeInterval( float value );

	/**
	 * @brief Adds or moves a sphere that blocks the line of sight to emitters behind it.
	 *
	 * @param occluderID Stable identifier of the occluder (e.g. a destiny ball ID).
	 * @param x          Centre of the sphere in game world space, see @c SetOrigin.
	 * @param y          "
	 * @param z          "
	 * @param radius     Radius of the sphere. Values <= 0 remove the occluder.
	 */
	void SetOccluderSphere( uint64_t occluderID, double x, double y, double z, float radius );

	/**
	 * @brief The game world point that audio space treats as its origin.
	 *
	 * Occluder centres are world-space doubles, too large for the float positions
	 * emitters and the listener use, so the game reports its audio-space centre here
	 * every tick and occluders are translated against it each time line of sight is
	 * computed. They therefore never need re-sending as the player moves.
	 */
	void SetOrigin( double x, double y, double z );

	/**
	 * @brief Removes a single occluder sphere. Emitters it was blocking fade back to clear.
	 */
	void RemoveOccluderSphere( uint64_t occluderID );

	/**
	 * @brief Removes every occluder sphere and fades all emitters back to clear.
	 */
	void ClearOccluderSpheres();

	/**
	 * @brief The occlusion currently applied to an emitter.
	 *
	 * This is the live, mid-fade value rather than the target, so it can be used to
	 * observe a fade in progress.
	 *
	 * @return The emitter's occlusion, or 0.0 if it is clear or is not tracked.
	 */
	float GetEmitterOcclusion( AkGameObjectID emitterID ) const;

	/**
	 * @brief How many occluder spheres are currently registered. For diagnostics.
	 */
	int GetOccluderSphereCount() const;

	/**
	 * @brief The occluders blocking at least one emitter, as of the last sightline pass.
	 *
	 * A snapshot recorded while the pass runs, so reading it costs nothing per emitter.
	 * Empty while sphere occlusion is inactive (no spheres, disabled, or acoustics on).
	 */
	std::vector<uint64_t> GetBlockingOccluderIDs() const;

	/**
	 * @brief The occluder currently blocking an emitter's sightline, for diagnostics.
	 *
	 * Answers the geometric question with the same translation, radius scaling and
	 * segment test the engine uses, regardless of whether occlusion is enabled.
	 *
	 * @return The blocking occluder's ID, or 0 if the sightline is clear or cannot
	 *         be computed (missing or unplaced listener or emitter).
	 */
	uint64_t GetBlockingOccluder( AkGameObjectID emitterID ) const;

	/// The game world point audio space is centred on, as last reported. For diagnostics.
	double GetOriginX() const;
	double GetOriginY() const;
	double GetOriginZ() const;

private:
	/**
	 * @brief The manager-owned subsystem this forwards to.
	 *
	 * @return The subsystem, or nullptr before an audio manager exists.
	 */
	static AudObstructionOcclusion* Occlusion();
};

TYPEDEF_BLUECLASS( AudOcclusion );
