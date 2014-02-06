/* 
	*************************************************************************************

	IAudLowLevelIO.h

	Author:    Andri Mar
	Created:   January 2009
	Updated:   May 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Blue wrapper interface for Wwise's low level IO interfaces.


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _IAUDLOWLEVELIO_H_
#define _IAUDLOWLEVELIO_H_

#include "Audio2.h"
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

BLUE_INTERFACE( IAudLowLevelIO ) : public IRoot,
								   public AK::StreamMgr::IAkFileLocationResolver,
								   public AK::StreamMgr::IAkIOHookBlocking
{

};

#endif