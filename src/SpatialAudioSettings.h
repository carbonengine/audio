////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Mar 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

enum class AudOcclusion : int
{
	Off = 0, // Occlusion disabled, no geometry registered, spatial audio geometry not initialized
	On  = 1  // Wwise Spatial Audio handles diffraction + transmission
};

/**
 * @brief Configuration for Wwise Spatial Audio initialization.
 *
 * Holds all parameters that map to AkSpatialAudioInitSettings.
 * Values are set before AudManager::Enable() and consumed by
 * InitSpatialAudioGeometry().
 */
class SpatialAudioSettings
{
public:
	SpatialAudioSettings();

	/**
	 * @brief Controls whether spatial audio occlusion is On or Off.
	 *
	 * When On, Wwise Spatial Audio handles diffraction and transmission.
	 * When Off, no geometry is registered and spatial audio geometry is
	 * not initialized.
	 */
	AudOcclusion GetOcclusionMode() const;
	void SetOcclusionMode( AudOcclusion value );

	/**
	 * @brief Maximum number of portals that sound can propagate through.
	 *
	 * Must be less than or equal to AK_MAX_SOUND_PROPAGATION_DEPTH.
	 */
	int GetMaxSoundPropagationDepth() const;
	void SetMaxSoundPropagationDepth( int value );

	/**
	 * @brief Amount that an emitter or listener has to move to trigger a
	 *        re-validation of reflections/diffraction.
	 *
	 * Larger values reduce CPU load at the cost of reduced accuracy.
	 * The ray tracing itself is not affected by this value — rays are
	 * cast each time a Spatial Audio update is executed.
	 */
	float GetMovementThreshold() const;
	void SetMovementThreshold( float value );

	/**
	 * @brief Number of primary rays used in the ray tracing engine.
	 *
	 * A larger number of rays increases the chances of finding reflection
	 * and diffraction paths, but results in higher CPU usage. When CPU
	 * limit is active (see @c cpuLimitPercentage), this setting represents
	 * the maximum allowed number of primary rays.
	 */
	int GetNumberOfPrimaryRays() const;
	void SetNumberOfPrimaryRays( int value );

	/**
	 * @brief Maximum reflection order [1, 4] — the number of 'bounces'
	 *        in a reflection path.
	 *
	 * A high reflection order renders more details at the expense of
	 * higher CPU usage.
	 */
	int GetMaxReflectionOrder() const;
	void SetMaxReflectionOrder( int value );

	/**
	 * @brief Maximum diffraction order [1, 8] — the number of 'bends'
	 *        in a diffraction path.
	 *
	 * A high diffraction order accommodates more complex geometry at the
	 * expense of higher CPU usage. Set to 0 to disable diffraction on
	 * all geometry. Diffraction must be enabled on the geometry itself
	 * (see AkGeometryParams).
	 *
	 * This limits the recursion depth of diffraction rays cast from the
	 * listener and the depth of the diffraction search between emitter
	 * and listener. To optimize CPU, set it to the maximum number of
	 * edges you expect geometry to traverse (e.g. 2 for a single box).
	 *
	 * A search starts from the listener; when the maximum order is
	 * exceeded, remaining geometry between the path's end and the emitter
	 * is ignored, causing the diffraction coefficient to be underestimated.
	 */
	int GetMaxDiffractionOrder() const;
	void SetMaxDiffractionOrder( int value );

	/**
	 * @brief Maximum number of game-defined auxiliary sends that can
	 *        originate from a single emitter.
	 *
	 * An emitter can send to its own room and to all adjacent rooms if
	 * the emitter and listener are in the same room. If a limit is set,
	 * the most prominent sends are kept based on spread to the adjacent
	 * portal from the emitter's perspective. Set to 1 to only allow
	 * emitters to send directly to their current room. Set to 0 to
	 * disable the limit.
	 */
	int GetMaxEmitterRoomAuxSends() const;
	void SetMaxEmitterRoomAuxSends( int value );

	/**
	 * @brief Maximum number of diffraction points at each end of a
	 *        reflection path.
	 *
	 * Diffraction on reflections allows reflections to fade in and out
	 * smoothly as the listener or emitter moves in and out of a
	 * reflection's shadow zone. When greater than zero, diffraction rays
	 * are sent from the listener to search for reflections around corners.
	 * Set to 0 to disable diffraction on reflections. Set to 2 or greater
	 * to allow reflections to propagate through portals without being
	 * cut off.
	 */
	int GetDiffractionOnReflectionsOrder() const;
	void SetDiffractionOnReflectionsOrder( int value );

	/**
	 * @brief Maximum total length of a path composed of a sequence of
	 *        segments (rays).
	 *
	 * High values compute longer paths but increase CPU cost. Each
	 * individual sound is also affected by its maximum attenuation
	 * distance specified in the Wwise Authoring tool. Reflection or
	 * diffraction paths will never exceed a sound's maximum attenuation
	 * distance (unless the furthest point is above the audibility
	 * threshold, in which case attenuation is considered infinite).
	 */
	float GetMaxPathLength() const;
	void SetMaxPathLength( float value );

	/**
	 * @brief Targeted computation time allocated for the ray tracing
	 *        engine, as a percentage [0, 100] of the current audio frame.
	 *
	 * The ray tracing engine dynamically adapts the number of primary
	 * rays to target the specified computation time. The computed number
	 * of primary rays will never exceed @c numberOfPrimaryRays.
	 * A value of 0 indicates no target has been set — the number of
	 * primary rays is then fixed at @c numberOfPrimaryRays.
	 */
	float GetCPULimitPercentage() const;
	void SetCPULimitPercentage( float value );

	/**
	 * @brief Spread path computation over N frames [1, ..].
	 *
	 * When set to 1, no load balancing is done. Values greater than 1
	 * spread the computation of paths across that many frames.
	 */
	int GetLoadBalancingSpread() const;
	void SetLoadBalancingSpread( int value );

	/**
	 * @brief Enable computation of geometric diffraction and transmission
	 *        paths for all sources that have "Enable Diffraction and
	 *        Transmission" checked in the Wwise Positioning tab.
	 *
	 * This flag enables sound paths around (diffraction) and through
	 * (transmission) geometry. Setting to false implies geometry is only
	 * used for reflection calculation. If false but a sound has diffraction
	 * enabled in Wwise, the sound will diffract through portals but pass
	 * through geometry as if it were not there.
	 */
	bool GetEnableDiffractionAndTransmission() const;
	void SetEnableDiffractionAndTransmission( bool value );

	/**
	 * @brief Calculate virtual positions for emitters diffracted through
	 *        a portal or around geometry.
	 *
	 * The apparent or virtual position is calculated by Wwise Spatial
	 * Audio and passed on to the sound engine.
	 */
	bool GetCalcEmitterVirtualPosition() const;
	void SetCalcEmitterVirtualPosition( bool value );

	/**
	 * @brief Enable diffraction on geometry edges.
	 *
	 * When enabled, sound can bend around edges of geometry. This adds
	 * realism but increases CPU usage. Diffraction must also be enabled
	 * globally via @c EnableDiffractionAndTransmission.
	 */
	bool GetEnableDiffraction() const;
	void SetEnableDiffraction( bool value );

	/**
	 * @brief Enable diffraction on boundary edges.
	 *
	 * Boundary edges are at the edges of the geometry mesh (not shared
	 * by two triangles). Enabling this generates diffraction edges for
	 * all boundary edges, which is more expensive but useful for
	 * incomplete meshes.
	 */
	bool GetEnableDiffractionOnBoundaryEdges() const;
	void SetEnableDiffractionOnBoundaryEdges( bool value );

private:
	AudOcclusion m_occlusionMode;
	int m_maxSoundPropagationDepth;
	float m_movementThreshold;
	int m_numberOfPrimaryRays;
	int m_maxReflectionOrder;
	int m_maxDiffractionOrder;
	int m_maxEmitterRoomAuxSends;
	int m_diffractionOnReflectionsOrder;
	float m_maxPathLength;
	float m_cpuLimitPercentage;
	int m_loadBalancingSpread;
	bool m_enableDiffractionAndTransmission;
	bool m_calcEmitterVirtualPosition;
	bool m_enableDiffraction;
	bool m_enableDiffractionOnBoundaryEdges;
};
