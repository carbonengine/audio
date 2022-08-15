#include "stdafx.h"
#include "AudGameObjResource.h"

BLUE_DEFINE_INTERFACE( ITr2SoundEmitter );

BLUE_DEFINE_ABSTRACT( AudGameObjResource );

const Be::ClassInfo* AudGameObjResource::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudGameObjResource, "" )

		MAP_INTERFACE( IBlueEventListener )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( AudGameObjResource )

		MAP_ATTRIBUTE( "ID", m_ID, "ID number of emitter.", Be::READ )
		MAP_ATTRIBUTE( "name", m_name, "Logical name of emitter. Does NOT have to be unique.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "List of RTPC parameters associated with this emitter.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "eventPrefix", m_eventPrefix, "A string that will be prefixed to all events using this emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scalingFactor", m_scalingFactor, "The scaling factor of the attenuation used for the sounds played using this emitter", Be::READ )

		MAP_METHOD_AND_WRAP
		( 
			"SetAttenuationScalingFactor", 
			SetAttenuationScalingFactor,
			"Set the scaling factor of a game object. Modify the attenuation computations\n"
			"on this game object to simulate sounds with a a larger or smaller area of effect.\n"
			"Returns AK_Success if successful, AK_InvalidParameter if the scaling factor specified was 0 or negative.\n"
			":param value: Floating point number representing scaling factor for the attenuation."
			":return: True if the attenuation was able to be set in Wwise. False if not or audio is not initialized."
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetSwitch", 
			SetSwitch,
			"Set the WWise switch state for this game object\n"
			":param groupName: Switch group string name.\n"
			":param switchName: Switch name to enable\n"
			":return: True if successfully set. Be aware that Wwise always return true so "
					  "something catastrophic would have to happen to return false or audio is not initalized."
		)
		MAP_METHOD_AND_WRAP
		( 
			"SetRTPC",
			SetRTPC,
			"Set an RTPC value on the game object.\n"
			":param rtpcName: The name of the RTPC you wish to set on the game object.\n"
			":param rtpcValue: The value you wish to set the RTPC to on the game object.\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"SeekOnEventPercent",
			SeekOnEventPercent,
			"Seek on an event by using a percentage of its duration.\n"
			":param playingID: The playingID of the event you want to seek on. Must have been played on this game object.\n"
			":param percent: The desired position, in percentage, where you want playback of this event to restart.\n"
			"				 Expressed in a percentage of the audio file's total duration between 0 and 1.\n"
	 	)
		MAP_METHOD_AND_WRAP
		( 
			"SeekOnEventMs",
			SeekOnEventMs,
			"Seek on an event using milliseconds.\n"
			":param playingID: The playingID of the event you want to seek on. Must have been played on this game object.\n"
			":param msToSeek: Desired position where playback should restart, in milliseconds.\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"StopAll",
			StopAll,
			"Stop all sounds playing on this game object.\n"
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"StopEvent",
			StopEvent,
			1,
			"Stop all sounds associated with a particular Wwise event.\n"
			":param eventName: The name of the event whose sounds you want to stop\n"
			":param fadeOutDuration: How long you want the fade out of the sound to be in milliseconds. Defaults to 1000.\n"
			":return: true if the event was found and stopped, false otherwise\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"StopSound",
			StopSound,
			"Stop the given playing ID. \n"
			":param playingID: The playingID of the event you want to stop.\n"
			":param fadeOut: The amount of time in milliseconds you want the sound to take to fade out. Defaults to 1000\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetPlayingEvents",
			GetPlayingEvents,
			"Get the currently playing events on this audio emitter.\n"
			":return: A dict containing the playing IDs and events currently playing.\n"
		)
		MAP_METHOD_AND_WRAP
		( 
			"Cull",
			Cull,
			"Debug entry point to allow python to cull a game object. Never use this for anything other than debugging!"
		)
		MAP_METHOD_AND_WRAP
		( 
			"Wake",
			Wake,
			"Debug entry point to allow python to wake a game object from being culled. Never use this for anything other than debugging!"
		)
		MAP_METHOD_AND_WRAP
		( 
			"IsCulled",
			IsCulled,
			"Whether or not this game object is currently culled."
		)
			
	EXPOSURE_END()
}