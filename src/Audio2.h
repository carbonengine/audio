// Copyright © 2014 CCP ehf.

#pragma once

#ifndef _AUDIO2_H_
#define _AUDIO2_H_

//Blue includes
#include <Blue.h>
#include <IBluePython.h>
#include <IBlueOS.h>
#include <IBlueCallbackMan.h>

#include "Vector3.h"

#include <cmath>

//Global setting constants
const int UI_GAME_OBJ_ID = 2;
const int MUSIC_GAME_OBJ_ID = 3;
const int LISTENER_GAME_OBJ_ID = 4;
const int START_GAME_OBJ_COUNT = 5;

inline bool IsReservedGameObjectID( AkGameObjectID id )
{
	return id < START_GAME_OBJ_COUNT;
}

// Makes sure objects are initialized far away so you don't hear them when they spawn.
// Game objects are culled by default so this value will never hit Wwise.
const Vector3 WWISE_INIT_POSITION = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);

// Whether a position is a real world placement rather than initial position. 
// Also defends against positions that are NaN or infinite.
inline bool IsUsableWorldPosition( const Vector3& position )
{
	return std::isfinite( position.x ) && std::isfinite( position.y ) && std::isfinite( position.z )
		&& !( position.x == WWISE_INIT_POSITION.x
			&& position.y == WWISE_INIT_POSITION.y
			&& position.z == WWISE_INIT_POSITION.z );
}

extern bool g_shuttingDown;
extern bool g_debugDisplayAllEmitters;
extern bool g_wwiseCommunicationEnabled;
extern const std::string g_wwiseVersion;

extern IBlueCallbackManPtr g_mainThreadQueue;

#endif
