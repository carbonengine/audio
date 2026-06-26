// Copyright © 2020 CCP ehf.

#include "stdafx.h"
#include "AudioInputMgr.h"

BLUE_DEFINE( AudioInputMgr );
BLUE_DEFINE_INTERFACE( IAudioInputMgr );

const Be::ClassInfo* AudioInputMgr::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudioInputMgr, "" )
		MAP_INTERFACE( AudioInputMgr )
		MAP_INTERFACE( IAudioInputMgr )
	EXPOSURE_END()
}
