// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudPosition.h"

BLUE_DEFINE( AudPosition );

const Be::ClassInfo* AudPosition::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudPosition, "Represents an emitter position." )
		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( AudPosition )
	EXPOSURE_END()
}