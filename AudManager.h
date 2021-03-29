/*
	*************************************************************************************

	AudManager.h

	Author:    Andri Mar
	Created:   October 2008
	OS:        Win32
	Project:   Audio2

	Description:

		TBA


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUDMANAGER_H_
#define _AUDMANAGER_H_

#include <queue>
#include "Audio2.h"
#include "AudSettings.h"

//-----------------------------------------------------------------------------
// Wwise includes
//-----------------------------------------------------------------------------
#include <AK/SoundEngine/Common/AkTypes.h>
#if _WIN32
#include "LowLevelIO/Win32/AkFilePackageLowLevelIOBlocking.h"
#elif __APPLE__
#include "LowLevelIO/POSIX/AkFilePackageLowLevelIOBlocking.h"
#endif

// Communication (AKA remote Wwise debugging)
#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include "CcpCore/include/CcpMutex.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
BLUE_DECLARE( AudResource );
BLUE_DECLARE( AudConfig );
BLUE_DECLARE( AudLowLevelIO );
BLUE_DECLARE( AudEmitterMulti );
BLUE_DECLARE( AudEmitter );
BLUE_DECLARE_INTERFACE( IAudActionLog );

//-----------------------------------------------------------------------------
// Typedefs
//-----------------------------------------------------------------------------
typedef std::vector<std::wstring> BankVector;
typedef std::vector<AkGameObjectID> GameObjIDVector;
typedef std::set<AudEmitterMulti*> EmitterMultiSet;

struct WaitingEvent
{
	AkUniqueID eventID;
	AkGameObjectID gameObjectID;
	AkUInt32 numRetries;
};

BLUE_CLASS( AudManager ) :
	public IRoot,
	public IBlueEvents
{
public:
	AudManager( IRoot* lockobj = 0 );
	~AudManager();

	EXPOSE_TO_BLUE();

	//-----------------------------------------------------------------------------
	// IBlueEvents
	//-----------------------------------------------------------------------------
	void OnTick( Be::Time realTime, Be::Time simTime, void* cookie );

	// Interface that handles initializing or termination of audio engine
	void SetEnabled( bool onoff );

	bool Init();
	void Terminate();

	// Decrefs the static AudManager in AudManager_Blue.cpp
	// for correct shutdown of python. Called in PythonClientStop
	// in audio2.cpp
	static void RemovePythonReference();

	void UpdateSettings( AudSettings * settings );

	// Bank loading
	bool LoadBank( const std::wstring& name );
	void UnloadBank( const std::wstring& name );
	void ClearBanks();

	// Game object destruction
	void AddToDestructionVector( AkGameObjectID gameObjID );

	// Exposing loaded sound banks to python
	std::vector<std::wstring> GetLoadedSoundBanks();

	// Add a Failed event to the waiting events vector
	void AddWaitingEvent( AkUniqueID eventID, AkGameObjectID gameObjID );

	// Run throught the list once and try to resend the events to the gameObjects.
	void ProcessWaitingEvents();

	// Gets an AudEmitterMulti for a given event name or creates it if it does not exist.
	Be::Result<std::string> GetEmitterForEventName( const std::wstring& eventName, AudEmitterMulti** out );

	void LogPostEvent( AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name );
	void LogStopPlayingID( AkGameObjectID emitterID, AkPlayingID playID );
	void LogSetSwitch( AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state );
	void LogSetState( const std::wstring& group, const std::wstring& state );
	void LogSetRTPC( AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID = AK_INVALID_PLAYING_ID );

	void EnableDebugDisplayAllEmitters();
	void DisableDebugDisplayAllEmitters();
	bool GetDebugDisplayAllEmitters();

	void RegisterAudEmitter( AudEmitter* emitter );
	void UnregisterAudEmitter( AudEmitter* emitter );
	void StopAll();

private:
	// Please note these inits need to be done in this order!
	bool InitLowLevel();
	bool InitCommunication();
	bool InitSound();
	bool InitMusic();
	void RegisterForTicks();

	void Process(); // Tick handler.

	// Get an emitter for an event that is already playing
	AudEmitterMulti* GetEmitterForEventID( AkUniqueID eventID );

	// Adds an AudEmitterMulti to the m_multiEmitters list.
	void AddMultiEmitterToList( AudEmitterMulti * multiEmitter );

	// Removes an AudEmitterMulti from the m_multiEmitters list.
	void RemoveMultiEmitterFromList( AudEmitterMulti * multiEmitter );

	// Takes care of updating the location for all AudEmitterMulti on each tick.
	void ProcessMultiEmitterList();

	friend class AudEmitterMulti;
	friend class AudEmitter;
	friend class AudEmitterDoppler;

	int m_tickInterval;
	Be::Time m_Time;

	// Initialization settings for Wwise
	AudSettingsPtr m_settings;
	// low level IO hook for Wwise
	CAkFilePackageLowLevelIOBlocking m_lowLevelIO;

#ifndef AK_OPTIMIZED
	AkCommSettings m_commSettings;
#endif

	BankVector m_loadedBanks;

	std::vector<WaitingEvent> m_waitingEvents;
	EmitterMultiSet m_multiEmitters;
	CcpMutex m_waitingEventsMutex;
	CcpMutex m_multiEmitterMutex;
	bool m_useDoppler;

	// Determines whether Wwise tries to asynchronously open files. If false, the client will freeze while an audio asset is 
	// downloaded on demand. Asynchronous opening does not freeze the client when using download on demand in conjunction 
	// with .wem files (aka streaming) but .bnk files will still freeze the client while Wwise waits to open it.
	bool m_asyncOpen; 

	std::vector<AudEmitter*> m_audioEmitters;

	//Debug
	IAudActionLogPtr m_log;
};

TYPEDEF_BLUECLASS( AudManager );

extern AudManager* g_audioManager;
extern BluePythonObject* g_audioManagerWrapper;

#endif
