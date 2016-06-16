#include <stdafx.h>
#include "AudUIPlayer.h"

BLUE_DEFINE_ABSTRACT( AudUIPlayer );

const Be::ClassInfo* AudUIPlayer::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudUIPlayer, "Simple wrapper providing a more logical way of playing UI sounds." )
		MAP_INTERFACE( AudGameObjResource )
		MAP_INTERFACE( AudUIPlayer )

		MAP_METHOD_AND_WRAP( "SendEvent", SendEvent,
							"Description:\n"
							"\tSends an event to Wwise and associates it with this entity.\n"
							"Signature:\n"
							"\tSendEvent( eventname ) -> unsigned int\n"
							"Arguments:\n"
							"\teventname -- Name of the event to be sent. UNICODE!"
							"Returns:\n"
							"\tunsigned int representing the playing id of that request."
							)
	EXPOSURE_END()
}

static BluePythonObject* obj = NULL;
static PyObject* PyGetUIPlayer( PyObject* self, PyObject* args )
{
	if( !obj )
	{
		AudUIPlayer* uip = new OAudUIPlayer;
		obj = PyOS->WrapBlueObject( uip->GetRawRoot() );
		uip->GetRawRoot()->Unlock();
	}
	return obj;
}
MAP_FUNCTION( "GetUIPlayer", PyGetUIPlayer,
			 "Description:\n"
			 "\tGets an instance of AudUIPlayer, if none exists it creates one(Singleton)."
			 "Signature:\n"
			 "\tGetUIPlayer() -> AudUIPlayer instance\n"
			 );