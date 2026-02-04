#include "stdafx.h"
#include "AudGeometry.h"

BLUE_DEFINE_ABSTRACT( AudGeometry );

const Be::ClassInfo* AudGeometry::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudGeometry, "Audio geometry for Wwise Spatial Audio occlusion/diffraction" )
	EXPOSURE_END()
}
