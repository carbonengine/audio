#include "stdafx.h"
#include "AudGeometry.h"

BLUE_DEFINE( AudGeometry );
BLUE_DEFINE_INTERFACE( ITr2AudGeometry );

const Be::ClassInfo* AudGeometry::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudGeometry, "Audio geometry for Wwise Spatial Audio occlusion/diffraction" )
		MAP_INTERFACE( ITr2AudGeometry )
	EXPOSURE_END()
}
