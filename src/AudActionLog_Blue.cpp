#include "stdafx.h"

#include "AudActionLog.h"

BLUE_DEFINE_INTERFACE( IAudActionLog );
BLUE_DEFINE( AudActionLogCB );

const Be::ClassInfo* AudActionLogCB::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudActionLogCB, "Triggers a callback to log whenever an audio action occurs." )
		MAP_INTERFACE( IAudActionLog )
		MAP_INTERFACE( AudActionLogCB )
		
		MAP_METHOD_AND_WRAP
		(
			"RegisterCallback",
			RegisterCallback,
			"Register a callback method to be called whenever a new action is logged."
		)
	EXPOSURE_END()
}
