#include "stdafx.h"
#include "AudGameObjResource.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

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
		MAP_ATTRIBUTE( "eventName", m_playEvent, "The event to play when the object is loaded. In AudUIPlayer it contains the name of event last played.", Be::READ )
		MAP_ATTRIBUTE( "eventPrefix", m_eventPrefix, "A string that will be prefixed to all events using this emitter", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scalingFactor", m_scalingFactor, "The scaling factor of the attenuation used for the sounds played using this emitter", Be::READ )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"SendEvent", 
			PySendEvent, 
			1,
		    "Sends an event to the audio system and returns an ID for the playing stream.\n"
			":param eventName: The name of the event to send to the sound engine\n"
			":param bypassPrefix: An optional argument that bypasses the audio emitters defined prefix if set to True."
		)
		MAP_METHOD_AND_WRAP( "SetAttenuationScalingFactor", SetAttenuationScalingFactor,
							 "Description:\n"
							 "\tSet the scaling factor of a game object. Modify the attenuation computations\n"
							 "\ton this game object to simulate sounds with a a larger or smaller area of effect.\n"
							 "\tReturns AK_Success if successful, AK_InvalidParameter if the scaling factor\n"
							 "\tspecified was 0 or negative.\n"
							 "Signature:\n"
							 "\tUpdateParameter( value ) -> success/fail code\n"
							 "Parameters:\n"
							 "\tvalue -- Floating point number representing scaling factor. for the attenuation."
						   )
		MAP_METHOD_AND_WRAP( "SetObstructionAndOcclusion", SetObstructionAndOcclusion,
							 "Description:\n"
							 "\tSets the obstruction and occlusion values for a game object and listener pair.\n"
							 "\tReturns, always returns AK_Success.\n"
							 "Signature:\n"
							 "\tSetObstructionAndOcclusion( listenerID, obstruction, occlusion ) -> AK_Success\n"
							 "Parameters:\n"
							 "\tlistenerID -- ID of the listener these values should apply to.\n"
							 "\tobstruction -- Obstruction value\n"
							 "\tocclusion -- Occlusion value\n"
						   )
		MAP_METHOD_AND_WRAP( "SetSwitch", SetSwitch,
							 "Description:\n"
							 "\tSet the WWise switch state for this game object\n"
							 "\tReturns, always returns AK_Success.\n"
							 "Signature:\n"
							 "\tSetSwitch( groupName, switchName ) -> AK_Success\n"
							 "Parameters:\n"
							 "\tgroupName -- Switch group string name.\n"
							 "\tswitchName -- Switch name to enable\n"
						   )
		MAP_METHOD_AND_WRAP( "SetRTPC",
							 SetRTPC,
						     "Set an RTPC value on the game object.\n"
							 ":type rtpcName: str\n"
							 ":param rtpcName: The name of the RTPC you wish to set on the game object.\n"
							 ":type rtpcValue: float\n"
							 ":param rtpcValue: The value you wish to set the RTPC to on the game object.\n"
						   )

	EXPOSURE_END()
}