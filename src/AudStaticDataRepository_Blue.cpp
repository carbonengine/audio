// Copyright © 2022 CCP ehf.

#include "stdafx.h"
#include "AudStaticDataRepository.h"

BLUE_DEFINE( AudStaticDataRepository );

const Be::ClassInfo* AudStaticDataRepository::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudStaticDataRepository, "Encapsulates settings for Audio" )
		MAP_INTERFACE(AudStaticDataRepository)

		MAP_METHOD_AND_WRAP(
			"Initialize",
			Initialize,
			"Initialize the repository with the sound IDs from EVE. This is necessary for audio2 to function when audio culling is enabled." 
		)
	EXPOSURE_END()
}
