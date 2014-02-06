/* 
	*************************************************************************************

	LogBridge.h

	Author:    Andri Mar
	Created:   February 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Connects Wwise to LogServer


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/
#pragma once
#ifndef _LOGBRIDGE_H_
#define _LOGBRIDGE_H_

#include "Audio2.h"
#include <AK/Tools/Common/AkMonitorError.h>

using namespace AK::Monitor;

void WwiseLogServerMessageHandler( ErrorCode in_eErrorCode, const wchar_t *in_pszError, ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID );
void WwiseLogServerBridgeInit( AK::Monitor::ErrorLevel errorLevel );

#endif