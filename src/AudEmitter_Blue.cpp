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
							 "Description:\n"	
							 "\tSet the postion and forward direction of the emitter.\n"
							 "\tReturns, always returns AK_Success\n"
							 "Signature:\n"
							 "\tSetPosition( front, pos ) -> AK_Success"
							 "Parameters:\n"
							 "\tfront -- 3-tuple representing the orientation of the object.\n"
							 "\tpos -- 3-tuple representing the world position ."
						   )
	EXPOSURE_CHAINTO( AudGameObjResource )
}