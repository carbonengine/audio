// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define NOMINMAX //don't want that evil microsoft macro

#ifdef _WIN32
#include <windows.h>
#endif



// TODO: reference additional headers your program requires here
//#include "Wwise_IDs.h"				// IDs for events, gameobjects etc.

#include <list>

#include <BlueExposure.h>
#include <BlueStatistics.h>
#include <IBluePaths.h>
#include <CcpMath.h>