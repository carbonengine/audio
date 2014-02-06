#pragma once

#ifndef _AUDIO2_H_
#define _AUDIO2_H_

//Blue includes
#include <Blue/Include/Blue.h>
#include <Blue/Include/IBluePython.h>
#include <Blue/Include/IBlueOS.h>
#include <Blue/Include/IBlueCallbackMan.h>

//Global setting constants
const int UI_GAME_OBJ_ID = 2;
const int JUKEBOX_GAME_OBJ_ID = 3;
const int START_GAME_OBJ_COUNT = 4;

// Needed for the maximum initial position of an audio object. 
// Increasing it above 1.0e+7F can lead to DC offset calculation issues within WWise.
// Also it cannot be 0 as that would put all objects too close to each other when
// creating the scene -> source starvation / huge noise might occur.
const float WISE_INIT_POSITION = 1.0e+7F;

extern bool g_audioEnabled;
extern bool g_audioInitialized;

extern IBlueCallbackManPtr g_mainThreadQueue;

extern void DestroyListenerVector();

//Backwards compatibility
typedef LPCWSTR			AkLpCtstr;
typedef LPCSTR			AkLpCstr;

#endif