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

#include "Audio2.h"

//-----------------------------------------------------------------------------
// Wwise includes
//-----------------------------------------------------------------------------
#include <AK/SoundEngine/Common/AkTypes.h>

// Communication
#ifndef AK_OPTIMIZED
	#include <AK/Comm/AkCommunication.h>
#endif

#include "CcpCore/include/CcpMutex.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
BLUE_DECLARE( AudResource );
BLUE_DECLARE( AudSettings );
BLUE_DECLARE( AudConfig );
BLUE_DECLARE( AudLowLevelIO );
BLUE_DECLARE( AudEmitterMulti );

//Audio communication settings
#ifndef AK_OPTIMIZED
	#define COMM_POOL_SIZE			(256 * 1024)
	#define COMM_POOL_BLOCK_SIZE	(48)
#endif

//-----------------------------------------------------------------------------
// Typedefs
//-----------------------------------------------------------------------------
typedef std::vector<std::wstring> BankVector;
typedef std::vector<AkGameObjectID> GameObjIDVector;
typedef std::set<AudEmitterMulti*> EmitterMultiSet;

struct WaitingEvent 
{
	AkUniqueID			eventID;
	AkGameObjectID		gameObjectID;
	AkUInt32			numRetries;
};

BLUE_CLASS( AudManager ) : public IRoot, public IBlueEvents
{
public:
	AudManager( IRoot* lockobj = 0 );
	~AudManager();
	
	EXPOSE_TO_BLUE();

	//-----------------------------------------------------------------------------
	// IBlueEvents
	//-----------------------------------------------------------------------------
	void OnTick( Be::Time realTime, Be::Time simTime, void* cookie );

	//-----------------------------------------------------------------------------
	// Audio init/term
	//-----------------------------------------------------------------------------
	void SetEnabled( bool onoff );

	bool Init();
	void Terminate();
	
	// Decrefs the static AudManager in AudManager_Blue.cpp
	// for correct shutdown of python. Called in PythonClientStop
	// in audio2.cpp
	static void RemovePythonReference();

	// Bank loading
	void LoadBank( const std::wstring& name );
	void UnloadBank( const std::wstring& name );

	// Game object destruction
	void AddToDestructionVector( AkGameObjectID gameObjID );

	// Settings store - not used a.t.m. because Wwise isn't
	// filled with runtime changeable settings as is.
	static AudSettings& GetSettings();

	// Exposing wwise constants to python
	static void AddConstantsToModule( PyObject* module );

	// Exposing loaded sound banks to python
	std::vector<std::wstring> GetLoadedSoundBanks();

	// Add a Failed event to the waiting events vector
	void AddWaitingEvent( AkUniqueID eventID, AkGameObjectID gameObjID );

	// Run throught the list once and try to resend the events to the gameObjects.
	void ProcessWaitingEvents( );

	// Gets an AudEmitterMulti for a given event name or creates it if it does not exist.
	Be::Result<std::string> GetEmitterForEventName( const std::wstring& eventName, AudEmitterMulti** out );


private:
	// Please note these inits need to be done in this order!
	bool InitLowLevel();
	bool InitCommunication();
	bool InitSound();
	bool InitMusic();
	bool InitPlugin();
	void RegisterForTicks();

	void Process(); // Tick handler.

	// Get an emitter for an event that is already playing
	AudEmitterMulti* GetEmitterForEventID(AkUniqueID eventID);

	// Adds an AudEmitterMulti to the m_multiEmitters list.
	void AddMultiEmitterToList(AudEmitterMulti* multiEmitter);

	// Removes an AudEmitterMulti from the m_multiEmitters list. 
	void RemoveMultiEmitterFromList(AudEmitterMulti* multiEmitter);

	// Takes care of updating the location for all AudEmitterMulti on each tick.
	void ProcessMultiEmitterList();

	friend class AudEmitterMulti;
	friend class AudEmitter;
	friend class AudEmitterDoppler;

	int m_tickInterval;
	Be::Time m_Time;

	//Exposed attributes
	AudConfigPtr m_initConfig;
	
	#ifndef AK_OPTIMIZED
		AkCommSettings m_commSettings;
	#endif

	BankVector m_loadedBanks;

	std::vector<WaitingEvent> m_waitingEvents;
	EmitterMultiSet m_multiEmitters;
	CcpMutex m_waitingEventsMutex;
	CcpMutex m_multiEmitterMutex;
	bool m_useDoppler;
};

TYPEDEF_BLUECLASS( AudManager );

extern AudManager* g_audioManager;
extern BluePythonObject* g_audioManagerWrapper;

#endif