// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudParameter.h"

BLUE_DEFINE( AudParameter );

const Be::ClassInfo* AudParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudParameter, "Wrapper for Wwises' RTPC parameters." )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( AudParameter )

		MAP_ATTRIBUTE( "name", m_name, "Name of the RTPC parameter.", Be::READWRITE )
		MAP_ATTRIBUTE( "value", m_value, "Value of the parameter.", Be::READWRITE | Be::NOTIFY )
	EXPOSURE_END()
}