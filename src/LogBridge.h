////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Contributors: Eric Nielsen
// Creation Date: February 2009
// Copyright (c) 2009-2022, CCP Games
//
// Send Wwise errors directly to our error logging system if running the Debug or Profile versions of the Wwise SDK.
// 

#pragma once
#ifndef _LOGBRIDGE_H_
#define _LOGBRIDGE_H_

using namespace AK::Monitor;

void WwiseLogServerMessageHandler( ErrorCode in_eErrorCode, const AkOSChar* in_pszError, ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID );
void WwiseLogServerBridgeInit( ErrorLevel errorLevel );

#endif