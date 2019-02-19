# include "stdafx.h"
# include "AudioEmitter.h"

BLUE_DEFINE( AudioEmitter );
BLUE_DEFINE_INTERFACE( ITr2AudioEmitter );

const Be::ClassInfo* AudioEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudioEmitter, "" )

		MAP_INTERFACE( ITr2AudioEmitter )
		MAP_INTERFACE( AudioEmitter )

		MAP_ATTRIBUTE
		( 
			"eventName", 
			m_eventName, 
			"Name of the event to be sent to Wwise", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP
		(
			"__init__", 
			Py__init__,
			"Creates a game object for the sound engine"
		)

		MAP_METHOD_AND_WRAP
		( 
			"SendEvent", 
			SendEvent, 
			"Sends an event to the sound engine to be played at the emitter's position"
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetTransform", 
			SetTransform, 
			"Sets the emitter's transform in the sound engine\n" 
			":param worldTransform: A matrix transform detailing where the emitter should emit sounds from"
		)

		EXPOSURE_END()
}