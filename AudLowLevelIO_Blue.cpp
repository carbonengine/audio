#include "stdafx.h"

#include "AudLowLevelIO.h"

BLUE_DEFINE( AudLowLevelIO );

const Be::ClassInfo* AudLowLevelIO::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudLowLevelIO, "Wrapper for Wwises' low level io object." )
		MAP_INTERFACE( IAudLowLevelIO )
		MAP_INTERFACE( AudLowLevelIO )

		MAP_METHOD_AND_WRAP( "__init__", Initialize,
					"Initializes the paths where soundbanks should be retrieved from.\n"
					":type path: str\n"
					":param path: Base path from where all SoundBanks are loaded from.\n"
					":type lang: str\n"
					":param lang: The filepath (relative to the base path) where language specific audio should be retrieved.\n"
				  )
		MAP_METHOD_AND_WRAP( "GetSelectedLangugePath", GetSelectedLangugePath,
							"Description:\n"
							"\tGets the selected language path.\n"
							"Signature:\n"
							"\tGetSelectedLangugePath(  ) -> String\n"
							"Parameters:\n"
							"\tNone"
							)
	EXPOSURE_END()	
}