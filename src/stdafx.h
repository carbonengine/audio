// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define NOMINMAX //don't want that evil microsoft macro
#define _CRT_SECURE_NO_WARNINGS // This is needed to stop a warning in the included Wwise headers

#ifdef _WIN32
#include <windows.h>
#endif


// TODO: reference additional headers your program requires here
//#include "Wwise_IDs.h"				// IDs for events, gameobjects etc.

#include <list>

// We have to include Wwise headers here so we can apply the _CRT_SECURE_NO_WARNINGS flag to them.
// Wwise still uses some deprecated windows API's that trigger CRT warnings.
#include <AK/AkWwiseSDKVersion.h>
#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkFNVHash.h>
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkProfilingID.h>

#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include <BlueExposure.h>
#include <BlueStatistics.h>
#include <IBluePaths.h>
#include <CcpMath.h>

#undef _CRT_SECURE_NO_WARNINGS
