#include "stdafx.h"
#include "AudPosition.h"

BLUE_DEFINE( AudPosition );

const Be::ClassInfo* AudPosition::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudPosition, "Represents an emitter position." )
		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( AudPosition )


		// This never would have worked... m_value is a struct of a completely unknown type
		//MAP_ATTRIBUTE( "value", m_value, "Position value.", Be::READWRITE | Be::NOTIFY )
	EXPOSURE_END()
}