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

enum class SoundBankStatus
{
	LOADING,
	LOADED,
	UNLOADING,
	UNLOADED,
	NOT_LOADED
};

struct SoundBankInfo 
{
	SoundBankStatus soundBankStatus;
	AkBankID soundBankID;
	std::wstring soundBankName;
	std::vector<std::pair<AudGameObjResourcePtr, std::wstring>> waitingEventsAfterLoad;
};

struct MonitoredParameterInfo
{
	float parameterValue = 0.0f;
	bool parameterExists = false;
	int watchers = 0;
};

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
		long long oneShotWindow = 50; // The window, in milliseconds, a one shot has to potentially be played.
		float weightMultiplier = 10000000.0f; // A multiplier applied to each individual weight.
		float playingVitalSoundWeight = std::numeric_limits<float>::max(); // The weight applied to a game object if it is playing a vital sound.
		float playing2DWeight = 999.0f; // The weight applied to a game object if it is playing or slated to play any 2D sounds.
		float rangeWeight = 400.0f; // The weight applied to a game object if it is within range of the listener.
		float activeSoundsWeight = 200.0f; // The weight applied to a game object if it has sounds currently playing.
		float waitingOneShotWeight = 100.0f; // The weight applied to a game object if there is a one shot sound waiting to play.
		float visibleWeight = 100.0f; // the weight applied to a game object if it is visible to the listener.
		float usedEmitterWeight = 50.0f; // The weight applied to a game object if it has been used in any way.
	};


	EXPOSE_TO_BLUE();

	// IBlueEvents
	void OnTick( Be::Time realTime, Be::Time simTime, void* cookie ) override;

	// Unloads all soundbanks currently loaded.
	void ClearBanks();
	// Disable CarbonAudio and terminate all relevant subprocesses. 
	void Disable();
	// Disable spatial audio. Works whether or not the user has a spatial audio endpoint active on their system.
	bool DisableSpatialAudio();
	// Enable CarbonAudio and load the given soundbanks.
	void Enable(BankVector soundBanksToLoad);
	// Enable spatial audio. If the user does not have a spatial audio endpoint active then it will do nothing.
	bool EnableSpatialAudio();
	// Get any game object if it currently exists.
	AudGameObjResource* GetAudioEmitter( AkGameObjectID emitterID );
	// Retreive a vector of all currently loaded soundbanks.
	const std::vector<std::wstring> GetLoadedSoundBanks();
	// Get the name of a SoundBank given its bank ID 
	std::wstring GetSoundBankName( const AkBankID bankID );
	// Get info about currently loaded, loading or unloading SoundBanks.
	SoundBankStatus GetSoundBankStatus( const AkBankID bankID );
	// Get info about currently loaded, loading or unloading SoundBanks. Note: lookup by SoundBank name is slower than lookup by bank ID.
	SoundBankStatus GetSoundBankStatus( const std::wstring& soundBankName );
	// Asynchronosly request to load the given soundbank into memory if it can be found on disk. 
	void LoadBank( const std::wstring& name );
	// Get info about a monitored audio parameter
	const MonitoredParameterInfo* GetParameterInfo( const std::wstring& audioParameterName );
	// Register an audio parameter to be monitored. 
	void RegisterParameter( const std::wstring& audioParameterName );
	// Register a callback to keep track of if the user's current output device supports spatial audio.
	void RegisterAudioDeviceChangeCallback( const BlueScriptCallback callback );
	// Register a game object for the audio manager to keep track of.
	void RegisterGameObject( AkGameObjectID emitterID, AudGameObjResource* emitter );
	// Register an event to be sent to Wwise after it is done loading. Only works with soundbanks in the SoundBankStatus::Loading state.
	void RegisterEventAfterSoundBankLoad( std::wstring& soundBankName, std::wstring& eventName, AudGameObjResource* emitter );
	// Set an RTPC not associated with a specific game object.
	bool SetGlobalRTPC( const std::wstring& rtpcName, float value );
	// Set a global state in Wwise.
	bool SetState( const std::wstring& stateGroup, const std::wstring& stateName );
	// Can be called to see if the current platform supports spatial audio.
	const bool SpatialAudioIsSupported();
	// Stop all currently playing sounds on all game objects.
	void StopAll();
	// Update audio engine settings, must be called before the audio engine is initialized in SetEnabled.
	void UpdateSettings( AudSettings * settings );
	// Asynchronously request to unload the given SoundBank from memory if it is currently loaded.
	void UnloadBank( const std::wstring& name );
	// Update the status of a particular SoundBank. If the status is SoundBankStatus::Loading then all events waiting for that SoundBank will be posted.
	void UpdateSoundBankStatus( const AkBankID bankID, const SoundBankStatus soundBankStatus );
	// Remove a particular SoundBank from the map that keeps track of its status.
	void StopTrackingSoundBank( const AkBankID bankID );
	// Unregister an audio parameter from being monitored. 
	void UnregisterParameter( const std::wstring& audioParameterName );
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
	bool GetAudioCullingEnabled() const;
	long long GetOneShotWindow() const;
	float GetPlaying2DWeight() const;
	float GetPlayingEventsWeight() const;
	float GetPlayingVitalSoundWeight() const;
	float GetRangeWeight() const;
	float GetUsedEmitterWeight() const;
	float GetVisibleWeight() const;
	float GetWaitingOneShotWeight() const;
	void SetOneShotWindow( long long numMilliseconds );
	void SetPlaying2DWeight( float weight );
	void SetPlayingEventsWeight( float weight );
	void SetPlayingVitalSoundWeight( float weight );
	void SetRangeWeight( float weight );
	void SetUsedEmitterWeight( float weight );
	void SetVisibleWeight( float weight );
	void SetWaitingOneShotWeight( float weight );

	// Wwise Callbacks
	static void AkPlatformProfilerPopTimer();
	static void AkPlatformProfilerPostmarker( AkPluginID in_uPluginID, const char* in_pszMarkerName );
	static void AkPlatformProfilerPushTimer( AkPluginID in_uPluginID, const char* in_pszZoneName );
	// Callback that is called right after audio is rendered every Wwise tick.
	static void GlobalCallbackEndRender( AK::IAkGlobalPluginContext* in_pContext, AkGlobalCallbackLocation in_eLocation, void* in_pCookie );

	// Debug
	std::vector<std::pair<AkGameObjectID, AudGameObjResource*>> GetPrioritizedAudioEmitters();
#ifndef AK_OPTIMIZED
	// Get the event name for the given playingID and emitter. 
	const std::wstring GetEventName( AkGameObjectID emitterID, AkPlayingID playingID );
#endif
	
private:
	// Compute a Wwise hash given a soundbank name. 
	AkBankID ComputeWwiseHashForSoundBank( const std::wstring& soundBankName );
	// Run the culling algorithm on all game objects.
	void CullAudio();
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
	// Update all watched audio parameters with their current values in Wwise (if they exist).
	void UpdateMonitoredParameters();

	//*** Callbacks ***//
	// A callback that is called whenever the audio device is changed.
	static void AudioDeviceStatusChangeCallback( AK::IAkGlobalPluginContext * in_pContext, AkUniqueID in_idAudioDeviceShareset, AkUInt32 in_idDeviceID, AK::AkAudioDeviceEvent in_idEvent, AKRESULT in_AkResult );
	// The callback called by Wwise once a SoundBank has been attempted to load.
	static void LoadBankCallback( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, void* in_pCookie );
	// The callback used by Wwise once it is done trying to unload a SoundBank 
	static void UnloadBankCallback( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, void* in_pCookie );

	friend class AudGameObjResource;

	// Determines whether Wwise tries to asynchronously open files. If false, the client will freeze while an audio asset is
	// downloaded on demand. Asynchronous opening does not freeze the client when using download on demand in conjunction
	// with .wem files (aka streaming) but .bnk files will still freeze the client while Wwise waits to open it.
	bool m_asyncOpen;
	// Signals whether Carbon Audio's spatial audio features are enabled. If the user currently doesn't have an active spatial audio endpoint then output will still be in stereo.
	bool m_spatialAudioEnabled;
	// A map of all existing game objects.
	std::vector< std::pair<AkGameObjectID, AudGameObjResource*> > m_gameObjects;
	std::map<AkBankID, SoundBankInfo> m_soundBankInfoMap;
	CcpMutex m_soundBankMutex;
	// low level IO hook for Wwise
	CAkFilePackageLowLevelIOBlocking m_lowLevelIO;
	// Initialization settings for Wwise
	AudSettingsPtr m_settings;
	const CullingInitSettings m_cullingInitSettings;
	int m_tickInterval;

	// A map of all currently monitored audio parameters in Wwise (e.g. RTPCs)
	std::map<std::wstring, MonitoredParameterInfo> m_monitoredParametersMap;
	CcpMutex m_moniteredParametersMapMutex;

	// Audio culling settings
	bool m_audioCullingEnabled;
	int m_maxAwakeGameObjects;
	long long m_oneShotWindow;
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
