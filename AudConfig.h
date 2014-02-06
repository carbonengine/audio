/* 
	*************************************************************************************

	AudConfig.h

	Author:    Andri Mar
	Created:   March 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Setting store for audio2, used for configuration used in initialization of
		audio2.


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDCONFIG_H_
#define _AUDCONFIG_H_

#include "Audio2.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>				// AkThreadProperties, AkInitSettings, AkPlatformInitSettings
#include <AK/MusicEngine/Common/AkMusicEngine.h>				// AkMusicSettings
#include <AK/SoundEngine/Common/AkMemoryMgr.h>					// Memory Manager
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>			// AkDeviceSettings, AkStreamMgrSettings
#include <AK/SoundEngine/Common/AkModule.h>						// AkMemSettings
#include <AK/SoundEngine/Common/AkQueryParameters.h>

#include "IAudLowLevelIO.h"

class AudManager;

BLUE_CLASS( AudConfig ) : public INotify
{
public:
	AudConfig( IRoot* lockobj = NULL );
	~AudConfig();

	EXPOSE_TO_BLUE();

	// INotify
	virtual bool OnModified( Be::Var* value );

	friend AudManager;

private:
	AkMemSettings m_memSettings;
	AkThreadProperties m_threadProperties;
	AkDeviceSettings m_deviceSettings;
	AkStreamMgrSettings m_streamMgrSettings;
	AkInitSettings m_initSettings;
	AkPlatformInitSettings m_platformInitSettings;
	AkMusicSettings m_musicSettings;
	IAudLowLevelIOPtr m_lowLevelIO;

	bool m_dirty;
    bool m_asyncFileOpen;
};
TYPEDEF_BLUECLASS( AudConfig );

	// AkMemSettings* m_memSettings;
		// uMaxNumPools
	// AkThreadProperties* m_threadProperties;
		// dwAffinityMask
		// nPriority
		// uStackSize
	// AkDeviceSettings* m_deviceSettings;
		// dwIdleWaitTime
		// fTargetAutoStmBufferLength
		// pThreadProperties
		// uGranularity
		// uIOMemorySize
		// uSchedulerTypeFlags
	// AkStreamMgrSettings* m_streamMgrSettings;
		// pLowLevelIO
		// uMemorySize
	// AkInitSettings* m_initSettings;
		// bEnableGameSyncPreparation 
		// pfnAssertHook
		// uCommandQueueSize
		// uDefaultPoolSize
		// uMaxNumPaths
		// uMaxNumTransitions
		// uMonitorPoolSize
		// uMonitorQueuePoolSize
		// uPrepareEventMemoryPoolID 
	// AkPlatformInitSettings* m_platformInitSettings;
		// bGlobalFocus
		// eAudioQuality
		// hWnd
		// threadBankManager
		// threadLEngine
		// threadMonitor
		// uLEngineDefaultPoolSize
		// uNumRefillsInVoice 
	// AkMusicSettings* m_musicSettings;
		// fStreamingLookAheadRatio
#endif