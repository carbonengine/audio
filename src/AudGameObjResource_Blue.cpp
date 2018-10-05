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
		MAP_INTERFACE( ITr2SoundEmitter )
		MAP_INTERFACE( AudGameObjResource )

		MAP_ATTRIBUTE( "ID", m_ID, "ID number of emitter.", Be::READ )
		MAP_ATTRIBUTE( "name", m_name, "Logical name of emitter. Does NOT have to be unique.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "List of RTPC parameters associated with this emitter.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "eventName", m_playEvent, "Name of event last played or you want played on loading from red file.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "playOnLoad", m_playOnLoad, "True: Play on creation/loading from red file.\nFalse: you get the idea!", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "eventPrefix", m_eventPrefix, "A string that will be prefixed to all events using this emitter", Be::READWRITE )
		MAP_ATTRIBUTE( "scalingFactor", m_scalingFactor, "The scaling factor of the attenuation used for the sounds played using this emitter", Be::READ )

		MAP_METHOD_AND_WRAP( "SendEvent", SendEvent,
							 "Description:\n"
							 "\tSends an event to the audio system and returns an ID for\n"
							 "\tthe playing stream or AK_INVALID_PLAYING_ID if it is unable\n"
							 "\tto play it.\n" 
							 "Signature:\n"
							 "\tSendEvent( eventName ) -> playingID\n"
							 "Parameters:\n"
							 "\teventName -- Name of the event the sound engine should execute. UNICODE!"
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

	EXPOSURE_END()
}