#include "stdafx.h"

#include "AudEmitter.h"
#include "Vector3.h"

BLUE_DEFINE( AudEmitter );
BLUE_DEFINE_INTERFACE( ITr2AudEmitter );

void AudEmitter::Py__init__( const std::string& name )
{
	m_name = name;
	AudGameObjResource::Initialize();
}

const Be::ClassInfo* AudEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitter, "RAII wrapper for Wwise gameobjects. Python constructor takes in a name of the object as a string" )
		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( ITr2AudEmitter )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			Py__init__,
			1,
			"Takes in a name of the emitter created as a string.\n"
			":param name: Name of Audio Emitter. Defaults to empty string."
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"SendEvent", 
			PostEvent, 
			2,
		    "Sends an event to the audio system and returns an ID for the playing stream.\n"
			":param eventName: The name of the event to send to the sound engine\n"
			":param bypassPrefix: An optional argument that bypasses the audio emitters defined prefix if set to True."
			":return: The playing ID associated with this event. 0 If sending the event failed.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"SetPosition",
			SetPosition,
			"Update orientation and position of an audio emitter.\n"
			":param front: Vector3 representing the orientation of the object.\n"
			":param top: Vector3 representing the up vector of the object\n"
			":param position: Vector3 representing the world position .\n"
		)
		MAP_ATTRIBUTE( "debugPosition", m_debugPosition, "The location of the audio emitter in space.", Be::READ )
		MAP_ATTRIBUTE( "debugFront", m_debugFront, "A vector pointing in the same direction that the audio emitter is facing. Can be (0.0, 0.0, 0.0)", Be::READ )
	EXPOSURE_CHAINTO( AudGameObjResource )
}