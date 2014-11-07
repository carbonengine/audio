#include "stdafx.h"

#include "AudEmitterMulti.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>



BLUE_DEFINE_ABSTRACT( AudEmitterMulti );

const Be::ClassInfo* AudEmitterMulti::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitterMulti,
					"Takes the event name (std::wstring) as an argument, and uses it for the name of the emitter.\n"
					"It plays the event through the emitter, but will require an update to location on every frame.\n"
				  )

		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( AudGameObjResource )
		MAP_INTERFACE( AudEmitterMulti )

		MAP_METHOD_AND_WRAP(	"SetMaximumLocations",
								SetMaximumLocations,
								"Description:\n"
								"\tSets the maximum locations a AudEmitterMulti can have.\n"
								"\tLegal values are between 1 and 82, both included.\n"
								"\tDefault value is 30.\n"
								"\tValues outside that range will be forced to min or max.\n"
								"Signature:\n"
								"\tSetMaximumLocations( numberOfLocations ) -> void\n"
								"Parameters:\n"
								"\tnumberOfLocations -- Number (unsigned int) of locations a AudEmitterMulti can have."
							)

		MAP_ATTRIBUTE(  "maximumLocations", 
						m_maximumLocations, 
						"The number of maximum locations an AudEmitterMulti can have.(82 max)\n", 
						Be::READ 
					 )


	EXPOSURE_CHAINTO( AudGameObjResource )
}