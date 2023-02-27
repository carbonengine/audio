#include <stdafx.h>
#include "AudUIPlayer.h"

BLUE_DEFINE_ABSTRACT( AudUIPlayer );

const Be::ClassInfo* AudUIPlayer::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudUIPlayer, "Simple wrapper providing a more logical way of playing UI sounds." )
		MAP_INTERFACE( AudUIPlayer )

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
			"PostDialogueEvent",
			PostDialogueEvent,
			"Post an event meant for dialogue. Allows for getting the duration of the playing event.\n"
			":param eventName: Name of the event to be sent to the sound engine.\n"
			":return: An int representing a playing ID of the event sent. 0 or below if it failed to send.\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetEventPlayPosition",
			GetEventPlayPosition,
 			"Get the time elapsed in milliseconds for the given playingID.\n"
			":param playingID: The playing ID of the event whose play position you want to get.\n"
			":return: The time elapsed in milliseconds. -1 if the playingID is invalid or has finished playing.\n"
		)
#if BLUE_WITH_PYTHON
		MAP_ATTRIBUTE( "eventSenderCallback", m_callback, "", Be::READWRITE )
#endif
	EXPOSURE_CHAINTO( AudEmitter )
}

static AudUIPlayerPtr s_audUIPlayer = nullptr;
static PyObject* PyGetUIPlayer( PyObject* self, PyObject* args )
{
	if ( s_audUIPlayer == nullptr )
	{
		s_audUIPlayer = new OAudUIPlayer;
	}
	return PyOS->WrapBlueObject( s_audUIPlayer->GetRawRoot() );
}
MAP_FUNCTION
( 
	"GetUIPlayer", 
	PyGetUIPlayer,
	"Gets an instance of AudUIPlayer, if no instance exists one will be created."
);
