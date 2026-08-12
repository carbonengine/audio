////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#pragma once

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

#include "Vector3.h"

class AudManager;

/**
 * @brief Owns the occlusion of every emitter and feeds those values to Wwise.
 *
 * The game registers the spheres that can block sound (@c SetOccluderSphere), and
 * @c Update() computes the blockage of every positioned emitter by testing the
 * listener-to-emitter segment against them, turning it into the obstruction and
 * occlusion Wwise applies (see @c AK::SoundEngine::SetObjectObstructionAndOcclusion).
 * Each value fades towards its target so that sounds do not pop when something moves
 * in front of them.
 *
 * Processing is skipped while spatial audio geometry (acoustics) is enabled, whose
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
	 * computes every positioned emitter's occlusion against them.
	 *
	 * @param occluderID Stable identifier of the occluder (e.g. a destiny ball ID).
	 * @param centerX    Centre of the sphere in game world space, see SetOccluderOrigin.
	 * @param centerY    Y coordinate of the sphere centre in game world space.
	 * @param centerZ    Z coordinate of the sphere centre in game world space.
	 * @param radius     Radius of the sphere. Values <= 0 remove the occluder.
	 */
	void SetOccluderSphere(uint64_t occluderID, double centerX, double centerY, double centerZ, float radius);

	/**
	 * @brief The game world point that audio space treats as its origin.
	 *
	 * Occluder centres are world-space doubles, too large for the float positions
	 * emitters and the listener use, so the game reports its audio-space centre here
	 * every tick and occluders are translated against it each time line of sight is
	 * computed. They therefore never need re-sending as the player moves.
	 */
	void SetOccluderOrigin(double x, double y, double z);

	/**
	 * @brief Removes a single occluder sphere. Emitters it was blocking fade back to clear.
	 */
	void RemoveOccluderSphere(uint64_t occluderID);

	/**
	 * @brief Removes every occluder sphere and fades all emitters back to clear.
	 */
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
	 * @brief What fraction of an occluder's radius counts as solid from 0.0 -> 1.0.
	 *
	 * The game registers bounding spheres, which for anything but a sphere are much
	 * larger than the object inside them, so a sightline can clip one while passing
	 * well clear of the object itself. Shrinking the radius makes blockage require
	 * real overlap instead of a graze. 1.0 uses the radii as the game gave them.
	 */
	float GetOccluderRadiusScale() const;
	void SetOccluderRadiusScale(float value);

	/**
	 * @brief How often Update() recomputes sphere line of sight, in seconds.
	 *
	 * Fades still advance every update; only the ray tests are throttled.
	 */
	float GetLosRecomputeInterval() const;
	void SetLosRecomputeInterval(float value);

	/// How many occluder spheres are currently registered.
	int GetOccluderSphereCount() const;

	/**
	 * @brief The occluders blocking at least one emitter, as of the last sightline pass.
	 *
	 * A snapshot recorded while the pass runs, so reading it costs nothing per emitter.
	 * The set answers "is this sphere blocking something", not "everything this sphere would block".
	 */
	std::vector<uint64_t> GetBlockingOccluderIDs() const;

	/**
	 * @brief The occluder currently blocking an emitter's sightline, for diagnostics.
	 *
	 * Answers the geometric question directly, with the same translation, radius
	 * scaling and segment test the sightline pass uses, regardless of whether
	 * occlusion processing is enabled or throttled.
	 *
	 * @return The blocking occluder's ID, or 0 if the sightline is clear, the
	 *         emitter or listener is missing or unplaced, or there is no manager.
	 */
	uint64_t GetBlockingOccluder(AkGameObjectID emitterID) const;

	/// The game world point audio space is centred on, as last reported. For diagnostics.
	double GetOriginX() const;
	double GetOriginY() const;
	double GetOriginZ() const;

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

	/// An occluder translated into audio space, with its radius already scaled by
	/// m_occluderRadiusScale so every consumer tests the same size sphere.
	struct TranslatedOccluder
	{
		Vector3 center;
		float radius = 0.0f;
	};

	/// Translate an occluder from game world space into audio space against the
	/// current origin. Callers must hold m_mutex.
	TranslatedOccluder TranslateToAudioSpace(const OccluderSphere& sphere) const;

	bool SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const;

	/// Fade every tracked emitter back to clear. Callers must hold m_mutex.
	void ClearAllTargetsLocked();

	/// Whether sphere occlusion is currently active (spheres registered, processing
	/// enabled, acoustics off). Callers must hold m_mutex.
	bool SphereOcclusionActive() const;

	/// Recompute the occlusion target of every positioned emitter against the
	/// occluder spheres. Callers must hold m_mutex.
	void ComputeSphereOcclusion(std::chrono::steady_clock::time_point now);

	/// Whether the segment from the listener to an emitter passes through a sphere.
	/// All three positions are in audio space.
	///
	/// Not a pure intersection test at the endpoints, and deliberately asymmetric: a
	/// sphere containing the listener never blocks, while one containing the emitter
	/// blocks only if the emitter is past the deepest point of the crossing. 
	static bool SegmentHitsSphere(const Vector3& segmentStart, const Vector3& segmentEnd,
	                              const Vector3& sphereCenter, float sphereRadius);

	static constexpr float DEFAULT_FADE_RATE = 0.5f;
	static constexpr float DEFAULT_BLOCKED_OCCLUSION = 0.35f;
	static constexpr float DEFAULT_LOS_RECOMPUTE_INTERVAL = 0.25f;
	// Bounding spheres are much larger than what they bound, so half the radius is a
	// closer match to the solid part of a typical occluder than the radius itself.
	static constexpr float DEFAULT_OCCLUDER_RADIUS_SCALE = 0.5f;

	AudManager* m_audioManager;
	std::unordered_map<AkGameObjectID, EmitterState> m_emitters;
	std::unordered_map<uint64_t, OccluderSphere> m_occluders;
	// The occluders that blocked at least one emitter during the last sightline pass.
	std::unordered_set<uint64_t> m_blockingOccluderIDs;
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