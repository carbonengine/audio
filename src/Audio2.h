#pragma once

#ifndef _AUDIO2_H_
#define _AUDIO2_H_

//Blue includes
#include "blue/Include/Blue.h"
#include "blue/Include/IBluePython.h"
#include "blue/Include/IBlueOS.h"
#include "blue/Include/IBlueCallbackMan.h"

#include "Vector3.h"

//Global setting constants
const int UI_GAME_OBJ_ID = 2;
const int MUSIC_GAME_OBJ_ID = 3;
const int LISTENER_GAME_OBJ_ID = 4;
const int START_GAME_OBJ_COUNT = 5;

// Needed for the maximum initial position of an audio object. 
// Increasing it above 1.0e+7F can lead to DC offset calculation issues within WWise.
// Also it cannot be 0 as that would put all objects too close to each other when
// creating the scene -> source starvation / huge noise might occur.
const Vector3 WWISE_INIT_POSITION = Vector3(1.0e+7F, 1.0e+7F, 1.0e+7F);

extern bool g_audioEnabled;
extern bool g_audioInitialized;
extern bool g_debugDisplayAllEmitters;
extern bool g_wwiseCommunicationEnabled;
extern const std::string g_wwiseVersion;

extern IBlueCallbackManPtr g_mainThreadQueue;

#endif
