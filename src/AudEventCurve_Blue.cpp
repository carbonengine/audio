#include "stdafx.h"
#include "AudConstants.h"
#include "AudEventCurve.h"

BLUE_DEFINE( AudEventCurve );
BLUE_DEFINE_INTERFACE( ITriFunction );
BLUE_DEFINE_INTERFACE( ITriCurveLength );
BLUE_DEFINE_INTERFACE( IEveSpaceObject2 );
BLUE_DEFINE_INTERFACE( ITriConstants );

const Be::ClassInfo* AudEventCurve::ExposeToBlue()
{
    EXPOSURE_BEGIN
	( 
		AudEventCurve, 
		"The AudEventCurve is a audio events sequencer with scalable playback speed.\n"
		"It holds an instance of a TriObserverLocal that should be located on the \n"
		"source object. When it is loaded up with a TriObserverLocal it will automatically\n"
		"create an AudEmitter on the TriObserverLocal and all events on the curve will be\n"
		"played through that emitter." 
	)
        MAP_INTERFACE( AudEventCurve )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITriCurveLength )

		MAP_ATTRIBUTE( "name", m_name, "The name of the curve", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "time", m_time, "Current time of the curve", Be::READ )
		MAP_ATTRIBUTE( "length", m_length, "Length of the curve", Be::READ )
		MAP_ATTRIBUTE( "localTime", m_localTime, "Local time - reset to 0 when curve cycles.", Be::READ )
		MAP_ATTRIBUTE( "value", m_value, "The string value of the last key triggered.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "keys", m_keys, "These are the keys of the curve", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "audioEmitter", m_audioEmitter, "This is the AudEmitter that is created when the curve is loaded.", Be::READ )
		MAP_ATTRIBUTE
		(
			"sourceTriObserver",
			m_sourceTriObserver,
			"The TriObserverLocal of the source object that holds the actual\n"
			"location of the sound.\n"
			"When this is set an AudEmitter will be created to connect the\n"
			"TriObserverLocal and the AudEventCurve when the object is loaded.\n"
			"This has the side effect that if the curveSet is set to play on load\n"
			"the audio event will be sent as soon as the AudEventCurve is created\n\n"
			"If this is done directly without SetSourceTriObserver the emitter will\n"
			"not be created.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE_WITH_CHOOSER(   
			"extrapolation", 
			m_extrapolation, 
			"This controls how time values outside the range of the curve are handled", 
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			TriExtrapolation
		)
		MAP_METHOD_AND_WRAP
		( 
			"AddKey", 
			AddKey, 
			"Adds a key with a string value. This string value is passed to\n"
			"then sent to the sound engine through the AudEmitter when the\n"
			"key is triggered.\n"
			"takes a float that represents time and a string for the event name"
		)
		MAP_METHOD_AND_WRAP
		(
			"SetSourceTriObserver",
			SetSourceTriObserver,
			"Set an instance of TriObserverLocal. Takes in an instance of \n"
			"TriObserverLocal of the sourceObject that the sound of the curve\n"
			"should play out of."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetSourceTriObserver",
			GetSourceTriObserver,
			"Gets and returns the TriObserverLocal if it exists"
		)
		MAP_METHOD_AND_WRAP
		( 
			"RemoveKey",
			RemoveKey,
			"Remove a key from the curve, takes in the index number of the key to be removed."
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetKeyCount", 
			GetKeyCount, 
			"Get number of keys in the curve" 
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetKeyTime", 
			GetKeyTime, 
			"Get the time of a specific key" 
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetKeyValue", 
			GetKeyValue, 
			"Get the value of a specific key" 
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetKeyTime", 
			SetKeyTime, 
			"Set the time of a specific key. Takes in an index of key and a float value for time." 
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetKeyValue", 
			SetKeyValue, 
			"Set the value of a specific key. Takes in an index of key and a std::wstring for value." 
		)

    EXPOSURE_END()
}