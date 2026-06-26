// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudEventKey.h"

BLUE_DEFINE( AudEventKey );

const Be::ClassInfo* AudEventKey::ExposeToBlue()
{
    EXPOSURE_BEGIN( AudEventKey, "" )
        MAP_INTERFACE( AudEventKey )

		MAP_ATTRIBUTE
		( 
			"time",  
			m_time,  
			"Time value for this key", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"value",  
			m_value,  
			"String associated with this key. This is passed to the event curve\n"
			"handler.", 
			Be::READWRITE | Be::PERSIST
		)

	EXPOSURE_END()
}
