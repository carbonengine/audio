#include "stdafx.h"

#include "AudEmitterDoppler.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

BLUE_DEFINE( AudEmitterDoppler );

void AudEmitterDoppler::Py__init__( const std::string& name, const std::string& rtpc, const int duration)
{
	m_name = name;
	m_rtpcID = AK::SoundEngine::GetIDFromString( rtpc.c_str() );
	m_dopplerChangeDuration = duration;
	m_rtpcIsDirty = false;
	Initialize();
}

const Be::ClassInfo* AudEmitterDoppler::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitterDoppler, "RAII wrapper for Wwise gameobjects. Python constructor takes in a name of the object as a string" )
		MAP_INTERFACE( AudEmitter )

		MAP_METHOD_AND_WRAP( "__init__", Py__init__, 
							"Description:\n"	
							 "\tCreates a new Doppler audio emitter.\n"
							 "\tReturns, void\n"
							 "Signature:\n"
							 "\t__init__( emitterName, rtpcName ) -> void"
							 "Parameters:\n"
							 "\temitterName -- Name of the emitter being created.\n"
							 "\trtpcName -- Name of the RTPC being used for the doppler change."
							)

	EXPOSURE_CHAINTO( AudEmitter )
}