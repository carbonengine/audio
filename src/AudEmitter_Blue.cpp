#include "stdafx.h"

#include "AudEmitter.h"
#include "Vector3.h"

BLUE_DEFINE( AudEmitter );

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
		MAP_INTERFACE( AudEmitter )

		MAP_METHOD_AND_WRAP( "__init__", Py__init__, "Takes in a name of the emitter created as a string")
		MAP_METHOD_AND_WRAP( "SetPosition", SetPosition,
							 "\tUpdates the entities orientation and position of an entity.\n"
							 "\tTakes 3 arguments but is not using the \"top\" vector as is.\n"
							 "Arguments:\n"
							 "\tfront -- Vector3 representing the orientation of the object.\n"
							 "\ttop -- Vector3 representing the up vector of the object\n"
							 "\tposition -- Vector3 representing the world position .\n"
						   )
	EXPOSURE_CHAINTO( AudGameObjResource )
}