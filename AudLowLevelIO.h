/* 
	*************************************************************************************

	AudLowLevelIO.h

	Author:    Andri Mar
	Created:   May 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDLOWLEVELIO_H_
#define _AUDLOWLEVELIO_H_

#include "Audio2.h"
#include "AkDefaultIOHookBlocking.h"

BLUE_CLASS( AudLowLevelIO ) : public CAkDefaultIOHookBlocking
{
public:
	AudLowLevelIO( IRoot* lockobj = NULL );
	~AudLowLevelIO();

	EXPOSE_TO_BLUE();

	virtual void Initialize( const std::wstring& path, const std::wstring& lang );

	std::wstring GetSelectedLangugePath( );
};

TYPEDEF_BLUECLASS( AudLowLevelIO );

#endif