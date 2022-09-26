#include "stdafx.h"
#include "AudManager.h"

#include "AudGameObjResource.h" 

BLUE_DEFINE( AudManager );

const Be::ClassInfo* AudManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudManager, "Manage startup, shutdown and soundbank management for Wwise along with logic that needs a global viewpoint of audio2." )
		MAP_INTERFACE( AudManager )

		MAP_ATTRIBUTE( "log", m_log, "A log system for logging events.", Be::READWRITE )
		MAP_ATTRIBUTE( "audioCullingEnabled", m_audioCullingEnabled, "Whether audio culling is enabled or disabled.", Be::READ )
		MAP_ATTRIBUTE( "maxAwakeGameObjects", m_maxAwakeGameObjects, "The maximum number of game objects that can be awake at one time.", Be::READWRITE )
		MAP_ATTRIBUTE( "weightMultiplier", m_weightMultiplier, "A multiplier applied to each individual weight", Be::READWRITE )
		MAP_PROPERTY( "oneShotWindow", GetOneShotWindow, SetOneShotWindow, "The window, in milliseconds, that a one shot has to get the chance to play.")
		MAP_PROPERTY( "activeSoundsWeight", GetPlayingEventsWeight, SetPlayingEventsWeight, "The weight applied to a game object if it has sounds currently playing.")
		MAP_PROPERTY( "rangeWeight", GetRangeWeight, SetRangeWeight, "The weight applied to a game object if it is within range of the listener.")
		MAP_PROPERTY( "usedEmitterWeight", GetUsedEmitterWeight, SetUsedEmitterWeight, "The weight applied to a game object if it has been used in any way.")
		MAP_PROPERTY( "waitingOneShotWeight", GetWaitingOneShotWeight, SetWaitingOneShotWeight, "The weight applied to a game object if there is a one shot sound waiting to play.")
		MAP_PROPERTY( "visibleWeight", GetVisibleWeight, SetVisibleWeight, "The weight applied to a game object if it is visible to the listener.")
		MAP_PROPERTY( "playing2DWeight", GetPlaying2DWeight, SetPlaying2DWeight, "The weight applied to a game object if it is currently playing a 2D sound.")
		
		MAP_METHOD_AND_WRAP
		( 
			"UpdateSettings",
			UpdateSettings,
			"Update settings to be used when starting Wwise. Needs to be called before SetEnabled in order to apply.\n"
			":settings: An AudSettings instance defining Wwise specific settings."
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetEnabled",
			SetEnabled,
			"Toggle the audio engine on or off.\n"
			":param onoff: whether the audio engine should initialized or terminated."
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
			"GetAudioEmitter",
			GetAudioEmitter,
			"Return the audio emitter object for the given emitter ID if it exists.\n"
			":param emitterID: The ID of the emitter to get.\n"
			":return: An audio emitter object if it exists, None otherwise."
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
	EXPOSURE_END()
}