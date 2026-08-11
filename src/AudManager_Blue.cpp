// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudManager.h"

#include "AudGameObjResource.h"

const Be::VarChooser AudioStateChooser[] = {
	{ "Uninitialized", static_cast<int>(AudioState::Uninitialized) },
	{ "Disabled", static_cast<int>(AudioState::Disabled) },
	{ "Enabled", static_cast<int>(AudioState::Enabled) },
	{ nullptr, 0 }
};

BLUE_DEFINE( AudManager );
BLUE_REGISTER_ENUM( "AUDIO_STATE", AudioState, AudioStateChooser );

const Be::ClassInfo* AudManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudManager, "Manage startup, shutdown and soundbank management for Wwise along with logic that needs a global viewpoint of audio2." )
		MAP_INTERFACE( AudManager )

		MAP_ATTRIBUTE( "log", m_log, "A log system for logging events.", Be::READWRITE )
		MAP_ATTRIBUTE("audioCullingEnabled", m_audioCullingEnabled, "Whether audio culling is enabled or disabled.", Be::READ)
		MAP_PROPERTY("maxAwakeGameObjects", GetMaxAwakeGameObjects, SetMaxAwakeGameObjects, "The maximum number of game objects that can be awake at one time.")
		MAP_PROPERTY("weightMultiplier", GetWeightMultiplier, SetWeightMultiplier, "A multiplier applied to each individual weight")
		MAP_ATTRIBUTE( "spatialAudioEnabled", m_spatialAudioEnabled, "Whether spatial audio (sometime's referred to as 3D audio) is enabled", Be::READ )
		MAP_PROPERTY( "oneShotWindow", GetOneShotWindow, SetOneShotWindow, "The window, in milliseconds, that a one shot has to get the chance to play.")
		MAP_PROPERTY( "activeSoundsWeight", GetPlayingEventsWeight, SetPlayingEventsWeight, "The weight applied to a game object if it has sounds currently playing.")
		MAP_PROPERTY( "rangeWeight", GetRangeWeight, SetRangeWeight, "The weight applied to a game object if it is within range of the listener.")
		MAP_PROPERTY( "usedEmitterWeight", GetUsedEmitterWeight, SetUsedEmitterWeight, "The weight applied to a game object if it has been used in any way.")
		MAP_PROPERTY( "waitingOneShotWeight", GetWaitingOneShotWeight, SetWaitingOneShotWeight, "The weight applied to a game object if there is a one shot sound waiting to play.")
		MAP_PROPERTY( "visibleWeight", GetVisibleWeight, SetVisibleWeight, "The weight applied to a game object if it is visible to the listener.")
		MAP_PROPERTY( "playing2DWeight", GetPlaying2DWeight, SetPlaying2DWeight, "The weight applied to a game object if it is currently playing a 2D sound.")

		// Spatial audio geometry settings
		MAP_PROPERTY( "spatialAudioGeometryEnabled", GetSpatialAudioGeometryEnabled, SetSpatialAudioGeometryEnabled, "Enable or disable spatial audio geometry.")
		MAP_PROPERTY( "movementThreshold", GetMovementThreshold, SetMovementThreshold, "Distance an emitter or listener must move to trigger a re-validation of reflections/diffraction.")
		MAP_PROPERTY( "numberOfPrimaryRays", GetNumberOfPrimaryRays, SetNumberOfPrimaryRays, "Number of primary rays used in the ray tracing engine. More rays = better quality but higher CPU.")
		MAP_PROPERTY( "maxReflectionOrder", GetMaxReflectionOrder, SetMaxReflectionOrder, "Maximum reflection order [1-4] - number of bounces in a reflection path.")
		MAP_PROPERTY( "maxDiffractionOrder", GetMaxDiffractionOrder, SetMaxDiffractionOrder, "Maximum diffraction order [1-8] - number of bends in a diffraction path. Set to 0 to disable diffraction.")
		MAP_PROPERTY( "maxEmitterRoomAuxSends", GetMaxEmitterRoomAuxSends, SetMaxEmitterRoomAuxSends, "Maximum number of game-defined auxiliary sends from a single emitter. Set to 0 to disable the limit.")
		MAP_PROPERTY( "diffractionOnReflectionsOrder", GetDiffractionOnReflectionsOrder, SetDiffractionOnReflectionsOrder, "Maximum diffraction points at each end of a reflection path. Set to 0 to disable diffraction on reflections.")
		MAP_PROPERTY( "maxPathLength", GetMaxPathLength, SetMaxPathLength, "Maximum total length of a path composed of segments. Higher values compute longer paths but increase CPU cost.")
		MAP_PROPERTY( "cpuLimitPercentage", GetCPULimitPercentage, SetCPULimitPercentage, "Targeted computation time for ray tracing as a percentage [0-100] of the audio frame. 0 = no limit.")
		MAP_PROPERTY( "loadBalancingSpread", GetLoadBalancingSpread, SetLoadBalancingSpread, "Spread path computation over N frames [1..]. 1 = no load balancing.")
		MAP_PROPERTY( "enableDiffractionAndTransmission", GetEnableDiffractionAndTransmission, SetEnableDiffractionAndTransmission, "Enable geometric diffraction and transmission path computation.")
		MAP_PROPERTY( "calcEmitterVirtualPosition", GetCalcEmitterVirtualPosition, SetCalcEmitterVirtualPosition, "Calculate virtual position for emitters diffracted through portals or around geometry.")
		MAP_PROPERTY( "transmissionLoss", GetTransmissionLoss, SetTransmissionLoss, "Per-mesh setting: transmission loss [0.0-1.0] applied to geometry surfaces when meshes are registered.")
		MAP_PROPERTY( "enableDiffraction", GetEnableDiffraction, SetEnableDiffraction, "Per-mesh setting: enable or disable geometric diffraction on mesh geometry.")
		MAP_PROPERTY( "enableDiffractionOnBoundaryEdges", GetEnableDiffractionOnBoundaryEdges, SetEnableDiffractionOnBoundaryEdges, "Per-mesh setting: switch to enable or disable geometric diffraction on boundary edges for this mesh.")

		// Obstruction / occlusion
		MAP_PROPERTY( "obstructionOcclusionEnabled", GetObstructionOcclusionEnabled, SetObstructionOcclusionEnabled, "Enable or disable game-driven obstruction/occlusion processing. Disabling fades all values back to clear.")
		MAP_PROPERTY( "obstructionOcclusionFadeRate", GetObstructionOcclusionFadeRate, SetObstructionOcclusionFadeRate, "How fast obstruction/occlusion values fade towards their targets, in units per second. 0 = instantaneous.")
		MAP_PROPERTY( "occluderBlockedOcclusion", GetOccluderBlockedOcclusion, SetOccluderBlockedOcclusion, "Occlusion [0.0-1.0] applied to an emitter whose line of sight an occluder sphere blocks. Kept below 1.0 so blocked sounds stay muffled instead of silenced.")
		MAP_PROPERTY( "occluderRadiusScale", GetOccluderRadiusScale, SetOccluderRadiusScale, "What fraction [0.0-1.0] of an occluder's radius counts as solid. The game registers bounding spheres, which are much larger than the objects inside them, so a sightline can clip a sphere while passing visibly clear of the rock. Lower this to require real overlap instead of a graze. 1.0 uses the radii as given.")
		MAP_PROPERTY( "occluderLosRecomputeInterval", GetOccluderLosRecomputeInterval, SetOccluderLosRecomputeInterval, "How often sphere line of sight is recomputed, in seconds. Fades still advance every update. 0 = every update.")

		MAP_METHOD_AND_WRAP
		( 
			"UpdateSettings",
			UpdateSettings,
			"Update settings to be used when starting Wwise. Needs to be called before SetEnabled in order to apply.\n"
			":settings: An AudSettings instance defining Wwise specific settings."
		)
		MAP_METHOD_AND_WRAP
		( 
			"Disable",
			Disable,
			"Disable CarbonAudio which unloads all SoundBanks without terminating Wwise."
		)
		MAP_METHOD_AND_WRAP
		( 
			"DisableSpatialAudio",
			DisableSpatialAudio,
			"Signal that you want spatial audio to be disabled. This method is asynchronous and will return True if the request "
			"was properly queued."
		)
		MAP_METHOD_AND_WRAP
		( 
			"Enable",
			Enable,
			"Enable CarbonAudio and load the given soundbanks.\n"
			":param soundBanksToLoad: A list of SoundBanks to load when enabling CarbonAudio. Note: the Init SoundBank\n"
			"                         is implicitly loaded and does not need to be passed in.\n"
			":type soundBanksToLoad: list"
		)
		MAP_METHOD_AND_WRAP
		( 
			"EnableSpatialAudio",
			EnableSpatialAudio,
			"Enable spatial audio. This method is asynchronous and a successful return means that the request "
			"to change the current Wwise audio device shareset was properly queued."
		)
		MAP_METHOD_AND_WRAP
		( 
			"LoadBank",
			LoadBank,
			"Loads the given soundbank from disk if it exists.\n"
			":param name: Name of the soundbank to load\n"
			":return: True or False depending on if the call to LoadBank failed or not."
		)
		MAP_METHOD_AND_WRAP
		( 
			"UnloadBank",
			UnloadBank,
			"Unload the given soundbank from memory.\n"
			":param name: Name of the soundbank to unload.\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"ClearBanks",
			ClearBanks,
			"Unload all currently loaded soundbanks from memory."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetLoadedSoundBanks",
			GetLoadedSoundBanks,
			"Return a list of loaded soundbanks."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetState",
			GetStateValue,
			"Return Carbon Audio's current engine lifecycle state."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetAudioEmitter",
			GetAudioEmitter,
			"Return the audio emitter object for the given emitter ID if it exists.\n"
			":param emitterID: The ID of the emitter to get.\n"
			":return: An audio emitter object if it exists, None otherwise."
		)
		MAP_METHOD_AND_WRAP
		( 
			"RegisterAudioDeviceChangeCallback",
			RegisterAudioDeviceChangeCallback,
			"Registers a callback that will be called every time the audio device changes such as by initializing carbon audio, "
			"enabling/disabling spatial audio manually or if the system's audio output changes. This callback allows the consumer of Carbon "
			"Audio to know if the user's system supports spatial audio or not. It is possible to enable Carbon Audio's spatial audio features "
			"without the user's machine having the ability to utilize it, hence the existence of this callback. The registered callback must "
			"accept a boolean argument that signifies whether or not the user's current audio output supports spatial audio.\n"
			"An example of a valid callback for this would be:\n\n"
			"def audioDeviceChangeCallback(outputSupportsSpatialAudio):\n"
			"    if outputSupportsSpatialAudio:\n"
			"        print('The user's current output device supports spatial audio.')\n"
			"    else:\n"
			"        print('The user's current output device does not support spatial audio.)\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"SetGlobalRTPC",
			SetGlobalRTPC,
			"Set an RTPC value not associated with any audio emitters.\n"
			":param rtpcName: The name of the RTPC to set.\n"
			":param value: The value to set the RTPC to."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetState",
			SetState,
			"Set a global state in Wwise.\n"
			":param stateGroup: The name of the state group the state belongs to in Wwise.\n"
			":param stateName: The state you want to set in Wwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetEmitterLineOfSightBlockage",
			SetEmitterLineOfSightBlockage,
			"Set a line-of-sight blockage ratio [0.0-1.0] for an emitter (0 = clear). Returns True if the emitter exists."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetEmitterOcclusion",
			GetEmitterOcclusion,
			"Get the occlusion value currently applied to an emitter. This is the live, mid-fade value "
			"rather than the target that was set, so it can be used to observe a fade in progress. "
			"Returns 0.0 if the emitter is clear or is not being tracked."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearObstructionOcclusion",
			ClearObstructionOcclusion,
			"Fade the obstruction/occlusion values of all tracked emitters back to clear."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetOccluderSphere",
			SetOccluderSphere,
			"Add or move a sphere that blocks the line of sight to emitters behind it. While any occluder\n"
			"spheres are registered, occlusion is computed per emitter against them every update and\n"
			"per-emitter blockage calls (SetEmitterLineOfSightBlockage) are rejected.\n"
			":param occluderID: Stable identifier of the occluder (e.g. a destiny ball ID).\n"
			":param x, y, z:   Centre of the sphere in game world space (e.g. raw destiny ball\n"
			"                  coordinates). Kept in double precision and translated into audio\n"
			"                  space via SetOccluderOrigin, so occluders never need re-sending\n"
			"                  as the player moves.\n"
			":param radius:    Radius of the sphere. Values <= 0 remove the occluder."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetOccluderOrigin",
			SetOccluderOrigin,
			"Report the game world point that audio space is centred on. Emitter and listener positions\n"
			"reach audio as floats relative to this point, while occluder centres are given in game world\n"
			"space, so this is what relates the two. Send it every tick; it costs one call no matter how\n"
			"many occluders are registered.\n"
			":param x, y, z: The game world position corresponding to the audio space origin\n"
			"                (in EVE, the ego ball's position)."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetOccluderSphereCount",
			GetOccluderSphereCount,
			"How many occluder spheres are currently registered."
		)
		MAP_METHOD_AND_WRAP
		(
			"DescribeEmitterOcclusion",
			DescribeEmitterOcclusion,
			"Explain an emitter's sightline, for diagnosing occlusion that looks wrong. Reports the\n"
			"audio-space listener and emitter positions this system is comparing, the occluder origin,\n"
			"the emitter's current and target occlusion, and which occluder blocks it if any.\n"
			":param emitterID: The emitter to explain.\n"
			":return: A human-readable summary."
		)
		MAP_METHOD_AND_WRAP
		(
			"RemoveOccluderSphere",
			RemoveOccluderSphere,
			"Remove a single occluder sphere. Emitters it was blocking fade back to clear.\n"
			":param occluderID: The identifier the occluder was registered with."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearOccluderSpheres",
			ClearOccluderSpheres,
			"Remove every occluder sphere and fade all emitters back to clear."
		)
		MAP_METHOD_AND_WRAP
		( 
			"SpatialAudioIsSupported",
			SpatialAudioIsSupported,
			"Signals whether Carbon Audio suports spatial audio on the current operating system."
		)
		MAP_METHOD_AND_WRAP
		( 
			"StopAll",
			StopAll,
			"Stop all sounds currently playing on all audio emitters."
		)
		MAP_METHOD_AND_WRAP
		(
			"EnableDebugDisplayAllEmitters",
			EnableDebugDisplayAllEmitters,
			"Forces all AudEmitters to render their debug info on screen."
		)
		MAP_METHOD_AND_WRAP
		(
			"DisableDebugDisplayAllEmitters",
			DisableDebugDisplayAllEmitters,
			"Stops the displaying of all AudEmitter debug info on screen."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetDebugDisplayAllEmitters",
			GetDebugDisplayAllEmitters,
			"Return the value of debugDisplayAllEmitters."
		)
		MAP_METHOD_AND_WRAP
		(
			"DisableAudioCulling",
			DisableAudioCulling,
			"Disable audio culling and wake up all game objects so they can be managed only by Wwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"EnableAudioCulling",
			EnableAudioCulling,
			"Enable audio culling and let audio2 manage which game objects to expose to Wwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"ResetCullingSettings",
			ResetCullingSettings,
			"Resets the audio culling settings back to default values."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetPrioritizedEmitters",
			GetPrioritizedAudioEmitters	,
			"Exposes the prioritized list of audio emitters as determined by the audio prioritization system. ONLY USE FOR DEBUG PURPOSES!"
		)
		MAP_METHOD_AND_WRAP
		(
			"StartProfilerCapture",
			StartProfilerCapture,
			"Starts recording the sound engine profiling information into a file."
			":return: An AKRESULT value indicating success or failure."
		)
		MAP_METHOD_AND_WRAP
		(
			"StopProfilerCapture",
			StopProfilerCapture,
			"Stops recording the sound engine profiling information."
			":return: An AKRESULT value indicating success or failure." 
		)
		MAP_METHOD_AND_WRAP
		(
			"IsProfilerCapturing",
			IsProfilerCapturing,
			"Checks if the profiler is currently capturing."
			":return: True if the profiler is capturing, False otherwise."
		)
	EXPOSURE_END()
}
