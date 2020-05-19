#include "stdafx.h"

#include "AudEmitter.h"
#include "Vector3.h"

BLUE_DEFINE( AudEmitter );
BLUE_DEFINE_INTERFACE( ITr2AudEmitter );

void AudEmitter::Py__init__( const std::string& name )
{
	m_name = name;
	Initialize();
}

const Be::ClassInfo* AudEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitter, "RAII wrapper for Wwise gameobjects. Python constructor takes in a name of the object as a string" )
		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( AudGameObjResource )
		MAP_INTERFACE( ITr2AudEmitter )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			Py__init__,
			1,
			"Takes in a name of the emitter created as a string.\n"
			":param name: Name of Audio Emitter. Defaults to empty string."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetPosition",
			SetPosition,
			"Updates the entities orientation and position of an entity.\n"
			"Takes 3 arguments but is not using the \"top\" vector as is.\n"
			":param front: Vector3 representing the orientation of the object.\n"
			":param top: Vector3 representing the up vector of the object\n"
			":param position: Vector3 representing the world position .\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"StopEvent",
			StopEvent,
			"Stops the sounds playing that are part of the given event.\n"
			":param eventName: The name of the event whose sounds you want to stop\n"
			":type eventName: str\n"
			":return: true if the event was found and stopped, false otherwise\n"
			":rtype: bool\n"
		)
	EXPOSURE_CHAINTO( AudGameObjResource )
}