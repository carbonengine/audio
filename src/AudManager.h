////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Contributors: Eric Nielsen
// Creation Date: October 2008
// Copyright (c) 2008-2022, CCP Games
//

#pragma once
#ifndef _AUDMANAGER_H_
#define _AUDMANAGER_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include "Audio2.h"
#include "AudSettings.h"
#include "AudListener.h"

#if _WIN32
#include "LowLevelIO/Win32/AkFilePackageLowLevelIOBlocking.h"
#elif __APPLE__
#include "LowLevelIO/POSIX/AkFilePackageLowLevelIOBlocking.h"
#endif

BLUE_DECLARE( AudConfig );
BLUE_DECLARE( AudLowLevelIO );
BLUE_DECLARE( AudGameObjResource );
BLUE_DECLARE_INTERFACE( IAudActionLog );

typedef std::vector<std::wstring> BankVector;
typedef std::vector<AkGameObjectID> GameObjIDVector;

// ------------------------------------------------------------------------
// Description:
//   Handles initialization/termination of the audio engine as well as any
//   methods that have to do with managing the state of the audio system as 
//   a whole.
// ------------------------------------------------------------------------
BLUE_CLASS( AudManager ) :
	public IRoot,
	public IBlueEvents
{
public:
	AudManager( IRoot* lockobj = 0 );
	~AudManager();

	const struct CullingInitSettings 
	{
		int maxAwakeGameObjects = 75; // Number of audio emitters that can be awake at the same time.
		float weightMultiplier = 1000.0f; // A multiplier applied to each individual weight.
		float activeSoundsWeight = 1000.0f; // The weight applied to a game object if it has sounds currently playing.
		float rangeWeight = 500.0f; // The weight applied to a game object if it is within range of the listener.
		float usedEmitterWeight = 10000000.0f; // The weight applied to a game object if it has been used in any way.
		float waitingOneShotWeight = 1000.0f; // The weight applied to a game object if there is a one shot sound waiting to play.
		float visibleWeight = 1500.0f; // The weight applied to a game object if it is visible to the listener.
		float playing2DWeight = 99999999.0f; // The weight applied to a game object if it is playing or slated to play any 2D sounds.
		float playingVitalSoundWeight = std::numeric_limits<float>::max(); // The weight applied to a game object if it is playing a vital sound.
	};

	EXPOSE_TO_BLUE();

	// IBlueEvents
	void OnTick( Be::Time realTime, Be::Time simTime, void* cookie ) override;

	// Unloads all soundbanks currently loaded.
	void ClearBanks();
	// Get any game object if it currently exists.
	AudGameObjResource* GetAudioEmitter( AkGameObjectID emitterID );
	// Retreive a vector of all currently loaded soundbanks.
	std::vector<std::wstring> GetLoadedSoundBanks();
	// Load the given soundbank into memory if it can be found on disk. 
	bool LoadBank( const std::wstring& name );
	// Register a game object for the audio manager to keep track of.
	void RegisterGameObject( AkGameObjectID emitterID, AudGameObjResource* emitter );
	// Handle both initialization and termination of the audio engine.
	void SetEnabled( bool onoff );
	// Set an RTPC not associated with a specific game object.
	bool SetGlobalRTPC( const std::wstring& rtpcName, float value );
	// Set a global state in Wwise.
	bool SetState( const std::wstring& stateGroup, const std::wstring& stateName );
	// Stop all currently playing sounds on all game objects.
	void StopAll();
	// Update audio engine settings, must be called before the audio engine is initialized in SetEnabled.
	void UpdateSettings( AudSettings * settings );
	// Unload the given soundbank from memory if it is currently loaded.
	void UnloadBank( const std::wstring& name );
	// Unregister a game object from the audio manager.
	void UnregisterGameObject( AkGameObjectID emitterID );
	// Disable audio culling and wake up all game objects so they can be managed only by Wwise.
	void DisableAudioCulling();
	// Enable audio culling and let audio2 manage which game objects to expose to Wwise.
	void EnableAudioCulling();
	// Reset audio culling settings to their default values.
	void ResetCullingSettings();

	// Debugging
	void DisableDebugDisplayAllEmitters();
	void EnableDebugDisplayAllEmitters();
	bool GetDebugDisplayAllEmitters();
	void LogPostEvent( AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name );
	void LogExecuteActionOnPlayingID( AkGameObjectID emitterID, AkPlayingID playID, const std::wstring& action );
	void LogSetSwitch( AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state );
	void LogSetState( const std::wstring& group, const std::wstring& state );
	void LogSetRTPC( AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID = AK_INVALID_PLAYING_ID );

	// Audio culling getters/setters
	float GetPlaying2DWeight() const;
	float GetPlayingEventsWeight() const;
	float GetPlayingVitalSoundWeight() const;
	float GetRangeWeight() const;
	float GetUsedEmitterWeight() const;
	float GetVisibleWeight() const;
	float GetWaitingOneShotWeight() const;
	void SetPlaying2DWeight( float weight );
	void SetPlayingEventsWeight( float weight );
	void SetPlayingVitalSoundWeight( float weight );
	void SetRangeWeight( float weight );
	void SetUsedEmitterWeight( float weight );
	void SetVisibleWeight( float weight );
	void SetWaitingOneShotWeight( float weight );

	// Debug
	std::vector<std::pair<AkGameObjectID, AudGameObjResource*>> GetPrioritizedAudioEmitters();
	
private:
	// Initializes all parts of Wwise in the correct order.
	bool Init();
	// Initializes communcation with Wwise. This is only done if using the Profile flavor of the Wwise SDK.
	bool InitCommunication();
	// Initializes the low level IO for Wwise.
	bool InitLowLevel();
	// Initializes Wwise's music engine.
	bool InitMusic();
	// Initializes Wwise's sound engine.
	bool InitSound();
	// Tick handler
	void Process(); 
	// Registers audio2 for the tick handler.
	void RegisterForTicks();
	// Terminates all Wwise modules.
	void Terminate();
	// Get the listener game object if it exists.
	AudListenerPtr GetListener();
	// Run the culling algorithm on all game objects.
	void CullAudio();

	friend class AudGameObjResource;

	// Determines whether Wwise tries to asynchronously open files. If false, the client will freeze while an audio asset is
	// downloaded on demand. Asynchronous opening does not freeze the client when using download on demand in conjunction
	// with .wem files (aka streaming) but .bnk files will still freeze the client while Wwise waits to open it.
	bool m_asyncOpen;
	// A map of all existing game objects.
	std::vector< std::pair<AkGameObjectID, AudGameObjResource*> > m_gameObjects;
	// All currently loaded soundbanks.
	BankVector m_loadedBanks;
	// low level IO hook for Wwise
	CAkFilePackageLowLevelIOBlocking m_lowLevelIO;
	// Initialization settings for Wwise
	AudSettingsPtr m_settings;
	const CullingInitSettings m_cullingInitSettings;
	int m_tickInterval;

	// Audio culling settings
	bool m_audioCullingEnabled;
	int m_maxAwakeGameObjects;
	float m_waitingOneShotWeight;
	float m_usedEmitterWeight; 
	float m_rangeWeight;
	float m_playingEventsWeight;
	float m_visibleWeight;
	float m_playing2DWeight;
	float m_playingVitalSoundWeight;
	float m_weightMultiplier;
#ifndef AK_OPTIMIZED
	// Wwise communication interface settings. 
	AkCommSettings m_commSettings;
#endif
	//Debug
	IAudActionLogPtr m_log;
};

TYPEDEF_BLUECLASS( AudManager );

extern AudManager* g_audioManager;
extern BluePythonObject* g_audioManagerWrapper;

#endif
