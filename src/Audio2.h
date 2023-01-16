#pragma once

#ifndef _AUDIO2_H_
#define _AUDIO2_H_

//Blue includes
#include "blue/Include/Blue.h"
#include "blue/Include/BlueStatistics.h"
#include "blue/Include/IBluePython.h"
#include "blue/Include/IBlueOS.h"
#include "blue/Include/IBlueCallbackMan.h"

#include "Vector3.h"

//Global setting constants
const int UI_GAME_OBJ_ID = 2;
const int MUSIC_GAME_OBJ_ID = 3;
const int LISTENER_GAME_OBJ_ID = 4;
const int START_GAME_OBJ_COUNT = 5;

// Makes sure objects are initialized far away so you don't hear them when they spawn.
// Game objects are culled by default so this value will never hit Wwise.
const Vector3 WWISE_INIT_POSITION = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);

extern bool g_audioEnabled;
extern bool g_audioInitialized;
extern bool g_debugDisplayAllEmitters;
extern bool g_wwiseCommunicationEnabled;
extern const std::string g_wwiseVersion;

extern IBlueCallbackManPtr g_mainThreadQueue;

#endif
