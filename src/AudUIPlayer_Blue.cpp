#include <stdafx.h>
#include "AudUIPlayer.h"

BLUE_DEFINE_ABSTRACT( AudUIPlayer );

const Be::ClassInfo* AudUIPlayer::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudUIPlayer, "Simple wrapper providing a more logical way of playing UI sounds." )
		MAP_INTERFACE( AudGameObjResource )
		MAP_INTERFACE( AudUIPlayer )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"SendEvent", 
			PySendEvent,
			1,
			"Send an event to Wwise and associates it with this entity."
			":param eventName: Name of the event to be sent"
			":type eventName: str"
			":param bypassPrefix: An optional argument that bypasses the audio emitters defined prefix if set to True."
			":type bypassPrefix: boolean"
			":return: an int representing a playing ID of the event sent. 0 if it failed to send."
			":rtype: int"
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetRTPC", 
			SetRTPC,
			"Set an RTPC value on the game object."
			":type rtpcName: str"
			":param rtpcName: The name of the RTPC you wish to set on the game object."
			":type rtpcValue: float"
			":param rtpcValue: The value you wish to set the RTPC to on the game object."
		)
		MAP_METHOD_AND_WRAP
		( 
			"SendEventWithCallback", 
			SendEventWithCallback,
			"Send an event to Wwise and associates it with this entity. When event has finished a callback will be made to the callback function."
			":param eventName: Name of the event to be sent. UNICODE!"
			":type eventName: str"
			":return: an int representing a playing ID of the event sent. 0 if it failed to send."
			":rtype: int"
		)
		MAP_METHOD_AND_WRAP
		(
			"StopSound",
			StopSound,
			"Stop a playing sound by playing ID."
			":param playingID: The playingID of the sound you want to stop. playingIDs are returned when sending events."
			":type playingID: int"
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
