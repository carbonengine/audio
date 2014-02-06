#include "stdafx.h"

#include "AudEmitterPersisted.h"
#include "Vector3.h"

BLUE_DEFINE( AudEmitterPersisted );

void AudEmitterPersisted::Py__init__( const std::string& name, const std::wstring& playEvent, bool playOnLoad )
{
	m_name = name;
	m_playEvent = playEvent;
	m_playOnLoad = playOnLoad;
	Initialize();
}


const Be::ClassInfo* AudEmitterPersisted::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitterPersisted, "RAII wrapper for Wwise gameobjects that can be persisted by saving to red files." )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( AudEmitterPersisted )

		MAP_ATTRIBUTE( "eventName", m_playEvent, "Name of event to play.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "playOnLoad", m_playOnLoad, "True: Play on creation/loading from red file.\nFalse: you get the idea!", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "__init__", Py__init__, 
							 "Description:\n"
							 "\tArgument constructor\n"
							 "Signature:\n"
							 "\t__init__( name, playEvent, playOnLoad = True )\n"
							 "Parameters:\n"
							 "\tname -- Name of the game object.\n"
							 "\tplayEvent -- Name of the event the sound engine should execute. UNICODE!"
							 "\tplayOnLoad -- If True, plays the event specified in playEvent on object load.\n"
						   )
	EXPOSURE_CHAINTO( AudEmitter )
}