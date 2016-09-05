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

							MAP_METHOD_AND_WRAP( "SendEventWithCallback", SendEventWithCallback,
							"Description:\n"
							"\tSends an event to Wwise and associates it with this entity.\n"
							"\tWhen event has finished a callback will be made to the m_callback function.\n"
							"Signature:\n"
							"\tSendEvent( eventname ) -> unsigned int\n"
							"Arguments:\n"
							"\teventname -- Name of the event to be sent. UNICODE!"
							"Returns:\n"
							"\tunsigned int representing the playing id of that request."
							)
#if BLUE_WITH_PYTHON
							MAP_ATTRIBUTE( "eventSenderCallback", m_callback, "", Be::READWRITE )
#endif
	EXPOSURE_END()
}

static BluePythonObject* s_audUIPlayer = NULL;
static PyObject* PyGetUIPlayer( PyObject* self, PyObject* args )
{
	if( !s_audUIPlayer )
	{
		AudUIPlayer* uip = new OAudUIPlayer;
		s_audUIPlayer = PyOS->WrapBlueObject( uip->GetRawRoot() );
		uip->GetRawRoot()->Unlock();
	}
	return s_audUIPlayer;
}
MAP_FUNCTION( "GetUIPlayer", PyGetUIPlayer,
			 "Description:\n"
			 "\tGets an instance of AudUIPlayer, if none exists it creates one(Singleton)."
			 "Signature:\n"
			 "\tGetUIPlayer() -> AudUIPlayer instance\n"
			 );
