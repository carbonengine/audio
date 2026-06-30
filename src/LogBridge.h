// Copyright © 2009 CCP ehf.

#pragma once
#ifndef _LOGBRIDGE_H_
#define _LOGBRIDGE_H_

using namespace AK::Monitor;

void WwiseLogServerMessageHandler( ErrorCode in_eErrorCode, const AkOSChar* in_pszError, ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID );
void WwiseLogServerBridgeInit( ErrorLevel errorLevel );

#endif