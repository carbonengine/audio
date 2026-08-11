////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

#include "Vector3.h"

class AudManager;

/**
 * @brief Owns the obstruction and occlusion of every emitter and feeds those values to Wwise.
 *
 * The game tells us how much of an emitter's line of sight to the listener is blocked, and this
 * class turns that into the obstruction and occlusion Wwise applies
 * (see @c AK::SoundEngine::SetObjectObstructionAndOcclusion). Each
 * value fades towards its target so that sounds do not pop when something moves in front of them.
 *
 * There are two ways to drive it, and they are mutually exclusive:
 *
 * - Per-emitter: the game computes blockage itself and pushes it through
 *   @c SetEmitterLineOfSightBlockage / @c SetObstructionOcclusion.
 * - Occluder spheres: the game pushes the spheres that can block sound
 *   (@c SetOccluderSphere), and @c Update() computes the blockage of every
 *   positioned emitter itself by testing the listener-to-emitter segment
 *   against them. While any spheres are registered, per-emitter calls are
 *   rejected so the two drivers cannot fight over the same emitter.
 *
 * Both are skipped while spatial audio geometry (acoustics) is enabled, whose
 * transmission already attenuates; stacking the two would double-muffle.
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
	 * @brief Sets the obstruction and occlusion an emitter fades towards.
	 *
	 * @param emitterID   The emitter to block.
	 * @param obstruction Target obstruction [0.0, 1.0]
	 * @param occlusion   Target occlusion [0.0, 1.0]
	 *
	 * @return True if the emitter exists and the values were accepted.
	 */
	bool SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion);

	/**
	 * @brief Sets how much of an emitter's line of sight to the listener is blocked.
	 *
	 * @param emitterID The emitter to block.
	 * @param blockage  How blocked the line of sight is [0.0, 1.0]. 0 is a clear line of sight.
	 *
	 * @return True if the emitter exists and the value was accepted.
	 */
	bool SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage);

	/**
	 * @brief The occlusion currently applied to an emitter.
	 */
	float GetEmitterOcclusion(AkGameObjectID emitterID) const;

	/// Drops an emitter straight away without fading it out, for when the game object goes away.
	void RemoveEmitter( AkGameObjectID emitterID );

	/// Forgets every emitter and the fade clock, for when audio is disabled.
	void Reset();

	/// Fades every tracked emitter back to clear.
	void ClearAll();

	/**
	 * @brief Controls whether game driven obstruction and occlusion is processed at all.
	 */
	bool IsEnabled() const;
	void SetEnabled(bool value);

	/**
	 * @brief How fast values move towards their target, in units per second.
	 */
	float GetFadeRate() const;
	void SetFadeRate(float value);

	/**
	 * @brief Adds or moves a sphere that blocks the line of sight to emitters behind it.
	 *
	 * While any occluder spheres are registered (and acoustics is off), Update()
	 * computes every positioned emitter's occlusion itself and per-emitter
	 * blockage calls are rejected.
	 *
	 * @param occluderID Stable identifier of the occluder (e.g. a destiny ball ID).
	 * @param centerX    Centre of the sphere in game world space, see SetOccluderOrigin.
	 * @param centerY    "
	 * @param centerZ    "
	 * @param radius     Radius of the sphere. Must be positive.
	 */
	void SetOccluderSphere(uint64_t occluderID, double centerX, double centerY, double centerZ, float radius);

	/**
	 * @brief The game world point that audio space treats as its origin.
	 *
	 * Occluder centres are given in the game's own world space, whose values are
	 * far too large for the float positions Wwise emitters and the listener use
	 * (a solar system reaches ~1e12 metres, where a float has half-kilometre
	 * granularity). The game keeps its render and audio space centred on the
	 * player instead, so it reports that centre here every tick and occluders are
	 * translated into audio space, in double precision, as they are tested.
	 *
	 * Occluders themselves therefore never need re-sending as the player moves.
	 */
	void SetOccluderOrigin(double x, double y, double z);

	/// Removes a single occluder sphere. Emitters it was blocking fade back to clear.
	void RemoveOccluderSphere(uint64_t occluderID);

	/// Removes every occluder sphere and fades all emitters back to clear.
	void ClearOccluderSpheres();

	/**
	 * @brief The occlusion applied to an emitter whose sightline a sphere blocks.
	 *
	 * Kept below 1.0 so occluded sounds stay muffled instead of silenced by the
	 * Wwise occlusion curve.
	 */
	float GetBlockedOcclusion() const;
	void SetBlockedOcclusion(float value);

	/**
	 * @brief What fraction of an occluder's radius counts as solid, in (0.0, 1.0].
	 *
	 * The radii the game registers are bounding spheres, and a bounding sphere around
	 * an irregular object is much larger than the object it bounds: a sightline can
	 * clip the edge of the sphere while passing visibly clear of the rock inside it.
	 * Testing against a shrunken radius means blockage needs real overlap instead of a
	 * graze. 1.0 uses the radii exactly as the game gave them.
	 */
	float GetOccluderRadiusScale() const;
	void SetOccluderRadiusScale(float value);

	/**
	 * @brief How often Update() recomputes sphere line of sight, in seconds.
	 *
	 * Fades still advance every update; only the ray tests are throttled.
	 * 0 recomputes every update.
	 */
	float GetLosRecomputeInterval() const;
	void SetLosRecomputeInterval(float value);

	/// How many occluder spheres are currently registered.
	int GetOccluderSphereCount() const;

	/**
	 * @brief Explains an emitter's sightline in audio space, for diagnostics.
	 *
	 * Reports the listener and emitter positions this system is actually working
	 * with plus the occluder that blocks them, if any. Occluders arrive in game
	 * world space and emitters in audio space, and a mismatch between the two looks
	 * exactly like "occlusion is broken" from the outside, so the numbers being
	 * compared need to be visible from the game.
	 */
	std::string DescribeEmitterOcclusion(AkGameObjectID emitterID) const;

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

	/// Everything we keep for one blocked emitter.
	struct EmitterState
	{
		FadingValue obstruction;
		FadingValue occlusion;

		bool needsSend = true;

	};

	/// A sphere the game registered as able to block sound. The centre stays in the
	/// game's world space, at the precision that space needs, until it is tested.
	struct OccluderSphere
	{
		double centerX = 0.0;
		double centerY = 0.0;
		double centerZ = 0.0;
		float radius = 0.0f;
	};

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	/// Fade every tracked emitter back to clear. Callers must hold m_mutex.
	void ClearAllTargetsLocked();

	/// Whether Update() currently owns the occlusion targets (spheres registered,
	/// processing enabled, acoustics off). Callers must hold m_mutex.
	bool InSphereMode() const;

	/// Recompute the occlusion target of every positioned emitter against the
	/// occluder spheres. Callers must hold m_mutex.
	void ComputeSphereOcclusion(std::chrono::steady_clock::time_point now);

	/// Whether the segment from the listener to an emitter passes through a sphere.
	/// All three positions are in audio space.
	///
	/// Not a pure intersection test at the endpoints, and deliberately asymmetric: a
	/// sphere containing the listener never blocks, while one containing the emitter
	/// blocks only if the emitter is past the deepest point of the crossing. See the
	/// implementation for the two real cases that forces apart.
	static bool SegmentHitsSphere(const Vector3& segmentStart, const Vector3& segmentEnd,
	                              const Vector3& sphereCenter, float sphereRadius);

	// Slow enough that a sound moving behind cover fades rather than switches, tuned by
	// ear in a live client.
	static constexpr float DEFAULT_FADE_RATE = 0.5f;
	static constexpr float DEFAULT_BLOCKED_OCCLUSION = 0.35f;
	static constexpr float DEFAULT_LOS_RECOMPUTE_INTERVAL = 0.25f;
	// Bounding spheres are much larger than what they bound, so half the radius is a
	// closer match to the solid part of a typical occluder than the radius itself.
	// Tuned by ear in a live asteroid field.
	static constexpr float DEFAULT_OCCLUDER_RADIUS_SCALE = 0.5f;

	AudManager* m_audioManager;
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	std::unordered_map<uint64_t, OccluderSphere> m_occluders;
	double m_originX;
	double m_originY;
	double m_originZ;
	float m_fadeRate;
	float m_blockedOcclusion;
	float m_occluderRadiusScale;
	float m_losRecomputeInterval;
	bool m_hasUpdated;
	bool m_hasComputedLos;
	bool m_enabled;
	mutable CcpMutex m_mutex;

	std::chrono::steady_clock::time_point m_lastUpdateTime;
	std::chrono::steady_clock::time_point m_lastLosComputeTime;

};