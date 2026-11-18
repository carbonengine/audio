#include "stdafx.h"
#include "AudWorld.h"

BLUE_DEFINE( AudWorld );
BLUE_DEFINE_INTERFACE( ITr2AudWorld );

const Be::ClassInfo* AudWorld::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudWorld, "Tells audio where trinity's scene origin is, so things that hold still in the world hold still for Wwise" )
		MAP_INTERFACE( ITr2AudWorld )
	EXPOSURE_END()
}
