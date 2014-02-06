/* 
	*************************************************************************************

	AudMp3Info.h

	Author:    Andri Mar
	Created:   June 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Wrapper for AkMp3BaseInfo


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDMP3INFO_H_
#define _AUDMP3INFO_H_

#include "Audio2.h"

#include "MP3/soundengineplugin/CCPMP3TagReader.h"

BLUE_CLASS( AudMp3Info ) : public IRoot, public CCPMP3BaseInfo
{
public:
	AudMp3Info( IRoot* lockobj = NULL );

	~AudMp3Info();

	EXPOSE_TO_BLUE();
};
TYPEDEF_BLUECLASS( AudMp3Info );

#endif