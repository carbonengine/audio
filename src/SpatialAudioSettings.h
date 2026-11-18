////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Mar 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

struct AkSpatialAudioInitSettings;

/**
 * @brief Settings for Wwise Spatial Audio, matching AkSpatialAudioInitSettings of the 2025.1 SDK.
 *
 * Every setting is read when Spatial Audio initializes. Settings marked "Live" also reach Wwise
 * when they change afterwards, the others need audio to be terminated and initialized again.
 */
class SpatialAudioSettings
{
public:
	SpatialAudioSettings();

	/// Tells the setters whether Spatial Audio is running, so live settings are sent to Wwise.
	void SetInitialized( bool initialized );

	/// Whether trinity's meshes are sent to Wwise as geometry.
	bool GetSpatialAudioGeometryEnabled() const;
	void SetSpatialAudioGeometryEnabled( bool value );

	/// Whether rooms and portals are sent to Wwise.
	bool GetSpatialAudioRoomsEnabled() const;
	void SetSpatialAudioRoomsEnabled( bool value );

	/// Distance an emitter or listener moves before its paths are validated again.
	float GetMovementThreshold() const;
	void SetMovementThreshold( float value );

	/// Live. Rays cast from the listener to find reflection and diffraction paths.
	int GetNumberOfPrimaryRays() const;
	void SetNumberOfPrimaryRays( int value );

	/// Live. Bounces in a reflection path [0, 4]. 0 turns reflections off.
	int GetMaxReflectionOrder() const;
	void SetMaxReflectionOrder( int value );

	/// Live. Edges a path can bend around [0, 8]. 0 turns geometric diffraction off.
	int GetMaxDiffractionOrder() const;
	void SetMaxDiffractionOrder( int value );

	/// Live. Diffraction paths per emitter [0, 32], not counting the direct path. Wwise recommends 2 to 8.
	int GetMaxDiffractionPaths() const;
	void SetMaxDiffractionPaths( int value );

	/// Live. Reflection paths computed across all emitters, the most prominent first. 0 is no limit.
	int GetMaxGlobalReflectionPaths() const;
	void SetMaxGlobalReflectionPaths( int value );

	/// Live. Room aux sends per emitter. 0 is no limit.
	int GetMaxEmitterRoomAuxSends() const;
	void SetMaxEmitterRoomAuxSends( int value );

	/// Diffraction points at each end of a reflection path. 0 turns diffraction on reflections off.
	int GetDiffractionOnReflectionsOrder() const;
	void SetDiffractionOnReflectionsOrder( int value );

	/// Largest diffraction angle a path can have, in degrees. Paths fade out towards it.
	float GetMaxDiffractionAngleDegrees() const;
	void SetMaxDiffractionAngleDegrees( float value );

	/// Longest reflection or diffraction path, in game units.
	float GetMaxPathLength() const;
	void SetMaxPathLength( float value );

	/**
	 * @brief Share of the audio frame Spatial Audio may use, in percent. 0 is no limit.
	 *
	 * Wwise stays under the limit by spreading path validation over more frames, up to
	 * the load balancing spread. It has no effect while the spread is 1.
	 */
	float GetCPULimitPercentage() const;
	void SetCPULimitPercentage( float value );

	/// Live. Frames path validation is spread over. With a CPU limit this is the largest spread Wwise picks.
	int GetLoadBalancingSpread() const;
	void SetLoadBalancingSpread( int value );

	/// Live. Smoothing of diffraction path parameters, in milliseconds. 0 turns it off. Experimental in Wwise.
	float GetSmoothingConstantMs() const;
	void SetSmoothingConstantMs( float value );

	/// Live. Scales every portal's aperture when reverb sends are computed. 1 is the real size.
	float GetAdjacentRoomBleed() const;
	void SetAdjacentRoomBleed( float value );

	/// Rooms sound can propagate through [0, 8].
	int GetMaxSoundPropagationDepth() const;
	void SetMaxSoundPropagationDepth( int value );

	/// Whether sounds with diffraction and transmission enabled take paths around and through geometry.
	bool GetEnableDiffractionAndTransmission() const;
	void SetEnableDiffractionAndTransmission( bool value );

	/// Whether Wwise moves a diffracted emitter to the position it appears to come from.
	bool GetCalcEmitterVirtualPosition() const;
	void SetCalcEmitterVirtualPosition( bool value );

	/// Live. How the transmission loss of the surfaces on a direct path combines: 0 add, 1 multiply, 2 max.
	int GetTransmissionOperation() const;
	void SetTransmissionOperation( int value );

	/// Emitters needed to form a cluster. Below 2 turns clustering off. Experimental in Wwise.
	int GetClusteringMinPoints() const;
	void SetClusteringMinPoints( int value );

	/// Largest distance between emitters of a cluster, at the dead zone distance.
	float GetClusteringMaxDistance() const;
	void SetClusteringMaxDistance( float value );

	/// Distance around the listener where emitters are not clustered.
	float GetClusteringDeadZoneDistance() const;
	void SetClusteringDeadZoneDistance( float value );

	/// Live. Transmission loss [0, 1] of mesh surfaces. 0 makes Wwise ignore the mesh.
	float GetTransmissionLoss() const;
	void SetTransmissionLoss( float value );

	/// Live. Whether sound diffracts around mesh edges.
	bool GetEnableDiffraction() const;
	void SetEnableDiffraction( bool value );

	/// Live. Whether sound diffracts around edges that belong to one triangle only. Off by default: it adds
	/// many edges for Wwise to search, and closed hulls don't need it.
	bool GetEnableDiffractionOnBoundaryEdges() const;
	void SetEnableDiffractionOnBoundaryEdges( bool value );

	/// Live. Whether a mesh is a closed volume that applies transmission loss once per pass through it.
	/// Has no effect while the transmission operation is max.
	bool GetGeometrySolid() const;
	void SetGeometrySolid( bool value );

	/// Live. How far a mesh moving on its own may get from where Wwise has it before it is sent again,
	/// as a share of its distance from the listener. Each send makes Wwise re-index the mesh, so a far mesh
	/// that turns a little every frame is sent once it has moved far enough to hear. 0 sends every move.
	float GetGeometryUpdateTolerance() const;
	void SetGeometryUpdateTolerance( float value );

	/// Live. Whether portals cut openings in the meshes they overlap. Off by default: Wwise re-cuts every mesh
	/// a portal overlaps each time it rebuilds portal visibility, which takes long with big meshes.
	/// Turn it on when a portal sits in a mesh wall and sound has to pass through it.
	bool GetGeometryPortalSubtraction() const;
	void SetGeometryPortalSubtraction( bool value );

	/// Live. Furthest a mesh can be from the listener and still be in Wwise, in game units. 0 for no limit.
	float GetGeometryMaxDistance() const;
	void SetGeometryMaxDistance( float value );

	/// Live. Most triangles in Wwise at once; the meshes nearest the listener go first. 0 for no limit.
	int GetGeometryMaxTriangles() const;
	void SetGeometryMaxTriangles( int value );

	void PopulateInitSettings( AkSpatialAudioInitSettings& out ) const;

private:
	bool m_initialized;
	bool m_spatialAudioGeometryEnabled;
	bool m_spatialAudioRoomsEnabled;
	float m_movementThreshold;
	int m_numberOfPrimaryRays;
	int m_maxReflectionOrder;
	int m_maxDiffractionOrder;
	int m_maxDiffractionPaths;
	int m_maxGlobalReflectionPaths;
	int m_maxEmitterRoomAuxSends;
	int m_diffractionOnReflectionsOrder;
	float m_maxDiffractionAngleDegrees;
	float m_maxPathLength;
	float m_cpuLimitPercentage;
	int m_loadBalancingSpread;
	float m_smoothingConstantMs;
	float m_adjacentRoomBleed;
	int m_maxSoundPropagationDepth;
	bool m_enableDiffractionAndTransmission;
	bool m_calcEmitterVirtualPosition;
	int m_transmissionOperation;
	int m_clusteringMinPoints;
	float m_clusteringMaxDistance;
	float m_clusteringDeadZoneDistance;
	float m_transmissionLoss;
	bool m_enableDiffraction;
	bool m_enableDiffractionOnBoundaryEdges;
	bool m_geometrySolid;
	bool m_geometryPortalSubtraction;
	float m_geometryUpdateTolerance;
	float m_geometryMaxDistance;
	int m_geometryMaxTriangles;
};
