////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Mar 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

struct AkSpatialAudioInitSettings;

/**
 * @brief A wrapper with configuration settings for Wwise Spatial Audio initialization.
 *
 */
class SpatialAudioSettings
{
public:
	SpatialAudioSettings();

	/**
	 * @brief Controls whether geometry based spatial audio processing is enabled.
	 */
	bool GetSpatialAudioGeometryEnabled() const;
	void SetSpatialAudioGeometryEnabled( bool value );

	/**
	 * @brief Amount that an emitter or listener has to move to trigger a validation of reflections/diffraction.
	 *
	 * Larger values can reduce the CPU load at the cost of reduced accuracy.
	 * Note that the ray tracing itself is not affected by this value.
	 * Rays are cast each time a Spatial Audio update is executed.
	 */
	float GetMovementThreshold() const;
	void SetMovementThreshold( float value );

	/**
	 * @brief The number of primary rays used in the ray tracing engine.
	 *
	 * A larger number of rays will increase the chances of finding reflection and diffraction paths,
	 * but will result in higher CPU usage. When CPU limit is active
	 * (see @c fCPULimitPercentage), this setting represents the maximum
	 * allowed number of primary rays.
	 */
	int GetNumberOfPrimaryRays() const;
	void SetNumberOfPrimaryRays( int value );

	/**
	 * @brief Maximum reflection order [1, 4] - the number of 'bounces' in a reflection path.
	 *
	 * A high reflection order renders more details at the expense of higher CPU usage.
	 */
	int GetMaxReflectionOrder() const;
	void SetMaxReflectionOrder( int value );

	/**
	 * @brief Maximum diffraction order [1, 8] - the number of 'bends' in a diffraction path.
	 *
	 * A high diffraction order accommodates more complex geometry at the expense of higher CPU usage.
	 * Diffraction must be enabled on the geometry to find diffraction paths
	 * (refer to @c AkGeometryParams). Set to 0 to disable diffraction on all geometry.
	 * This parameter limits the recursion depth of diffraction rays cast from the listener
	 * to scan the environment, and also the depth of the diffraction search to find paths
	 * between emitter and listener.
	 * To optimize CPU usage, set it to the maximum number of edges you expect the obstructing
	 * geometry to traverse. For example, if box-shaped geometry is used exclusively, and only
	 * a single box is expected between an emitter and the listener, limiting @c uMaxDiffractionOrder
	 * to 2 may be sufficient.
	 * A diffraction path search starts from the listener, so when the maximum diffraction order
	 * is exceeded, the remaining geometry between the end of the path and the emitter is ignored.
	 * In such case, where the search is terminated before reaching the emitter, the diffraction
	 * coefficient will be underestimated. It is calculated from a partial path, ignoring any
	 * remaining geometry.
	 */
	int GetMaxDiffractionOrder() const;
	void SetMaxDiffractionOrder( int value );

	/**
	 * @brief The maximum number of game-defined auxiliary sends that can originate from a single emitter.
	 *
	 * An emitter can send to its own room, and to all adjacent rooms if the emitter and listener
	 * are in the same room. If a limit is set, the most prominent sends are kept, based on spread
	 * to the adjacent portal from the emitter's perspective.
	 * Set to 1 to only allow emitters to send directly to their current room, and to the room
	 * a listener is transitioning to if inside a portal. Set to 0 to disable the limit.
	 */
	int GetMaxEmitterRoomAuxSends() const;
	void SetMaxEmitterRoomAuxSends( int value );

	/**
	 * @brief The maximum possible number of diffraction points at each end of a reflection path.
	 *
	 * Diffraction on reflection allows reflections to fade in and out smoothly as the listener
	 * or emitter moves in and out of the reflection's shadow zone.
	 * When greater than zero, diffraction rays are sent from the listener to search for reflections
	 * around one or more corners from the listener.
	 * Diffraction must be enabled on the geometry to find diffracted reflections
	 * (refer to @c AkGeometryParams). Set to 0 to disable diffraction on reflections.
	 * To allow reflections to propagate through portals without being cut off,
	 * set @c uDiffractionOnReflectionsOrder to 2 or greater.
	 */
	int GetDiffractionOnReflectionsOrder() const;
	void SetDiffractionOnReflectionsOrder( int value );

	/**
	 * @brief The total length of a path composed of a sequence of segments (or rays) cannot exceed
	 *        the defined maximum path length.
	 *
	 * High values compute longer paths but increase the CPU cost.
	 * Each individual sound is also affected by its maximum attenuation distance, specified
	 * in the Authoring tool. Reflection or diffraction paths, calculated inside Spatial Audio,
	 * will never exceed a sound's maximum attenuation distance.
	 * Note, however, that attenuation is considered infinite if the furthest point is above
	 * the audibility threshold.
	 */
	float GetMaxPathLength() const;
	void SetMaxPathLength( float value );

	/**
	 * @brief Defines the targeted computation time allocated for the ray tracing engine.
	 *
	 * Defined as a percentage [0, 100] of the current audio frame.
	 * The ray tracing engine dynamically adapts the number of primary rays to target
	 * the specified computation time value. In all circumstances, the computed number
	 * of primary rays cannot exceed the number of primary rays specified by
	 * @c uNumberOfPrimaryRays.
	 * A value of 0 indicates no target has been set. In this case, the number of primary
	 * rays is fixed and is set by @c uNumberOfPrimaryRays.
	 */
	float GetCPULimitPercentage() const;
	void SetCPULimitPercentage( float value );

	/**
	 * @brief Spread the computation of paths on uLoadBalancingSpread frames [1, ..].
	 *
	 * When uLoadBalancingSpread is set to 1, no load balancing is done.
	 * Values greater than 1 indicate the computation of paths will be spread
	 * on this number of frames.
	 */
	int GetLoadBalancingSpread() const;
	void SetLoadBalancingSpread( int value );

	/**
	 * @brief Enable computation of geometric diffraction and transmission paths for all sources
	 *        that have the "Enable Diffraction and Transmission" box checked in the Positioning
	 *        tab of the Wwise Property Editor.
	 *
	 * This flag enables sound paths around (diffraction) and through (transmission) geometry
	 * (see @c AK::SpatialAudio::SetGeometry).
	 * Setting @c bEnableGeometricDiffractionAndTransmission to false implies that geometry
	 * is only to be used for reflection calculation.
	 * Diffraction edges must be enabled on geometry for diffraction calculation
	 * (see @c AkGeometryParams).
	 * If @c bEnableGeometricDiffractionAndTransmission is false but a sound has
	 * "Enable Diffraction and Transmission" selected in the Positioning tab of the authoring tool,
	 * the sound will diffract through portals but will pass through geometry as if it is not there.
	 * One would typically disable this setting in the case that the game intends to perform its own
	 * obstruction calculation, but geometry is still passed to spatial audio for reflection calculation.
	 */
	bool GetEnableDiffractionAndTransmission() const;
	void SetEnableDiffractionAndTransmission( bool value );

	/**
	 * @brief An emitter that is diffracted through a portal or around geometry will have its
	 *        apparent or virtual position calculated by Wwise Spatial Audio and passed on to
	 *        the sound engine.
	 */
	bool GetCalcEmitterVirtualPosition() const;
	void SetCalcEmitterVirtualPosition( bool value );

	/**
	 * @brief Transmission loss [0.0-1.0] applied to geometry surfaces when meshes are registered.
	 */
	float GetTransmissionLoss() const;
	void SetTransmissionLoss( float value );

	/**
	 * @brief Switch to enable or disable geometric diffraction for this Geometry.
	 */
	bool GetEnableDiffraction() const;
	void SetEnableDiffraction( bool value );

	/**
	 * @brief Switch to enable or disable geometric diffraction on boundary edges for this mesh.
	 *        Boundary edges are edges that are connected to only one triangle.
	 */
	bool GetEnableDiffractionOnBoundaryEdges() const;
	void SetEnableDiffractionOnBoundaryEdges( bool value );

	/**
	 * @brief Populates an AkSpatialAudioInitSettings struct from the current settings.
	 */
	void PopulateInitSettings( AkSpatialAudioInitSettings& out ) const;

private:
	bool m_spatialAudioGeometryEnabled;
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
	float m_transmissionLoss;
	bool m_enableDiffraction;
	bool m_enableDiffractionOnBoundaryEdges;
};
