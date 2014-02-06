#include "StdAfx.h"
#include "AudConfig.h"

// Device settings stuff.

#define AK_DEFAULT_BLOCK_ALLOCATION_TYPE	(AkVirtualAlloc)// Block allocation type. We use the VirtualAlloc hook because it guarantees that memory is
															// aligned on sectors, which is necessary for proper unbuffered transfers (FILE_FLAG_NO_BUFFERING).
															// You can use AkMalloc if you don't open files with FILE_FLAG_NO_BUFFERING flag in the Low-Level IO,
															// and/or if your I/O hooks call your own high-level scheduler.

// Note that the I/O pool is a FixedSizedBlock-style pool and it has no lock: all allocations have 
// the same size, which makes it very efficient. System memory alignment depends on the allocation
// hook. Otherwise, all allocations are aligned to multiples of the granularity. 

#define AK_REQUIRED_IO_POOL_ALIGNMENT		(4)				// No requirement. If granularity is not a multiple of 2 KB, I/O data will be buffered by the system.

#define AK_DEFAULT_MAX_CONCURRENT_IO		(8)				// 8. With AK_SCHEDULER_BLOCKING, it is always 1 anyway. Default is arbitrarily set to 8 for deferred device.

// END Device settings stuff.

AudConfig::AudConfig( IRoot *lockobj )
	: m_deviceSettings()
	, m_initSettings()
	, m_memSettings()
	, m_musicSettings() 
	, m_platformInitSettings()
	, m_streamMgrSettings()
	, m_threadProperties()
	, m_dirty( false )
    , m_asyncFileOpen( false )
{
	// Set all the default values.
	m_memSettings.uMaxNumPools = 20;

	m_streamMgrSettings.uMemorySize = 32*1024;

	m_threadProperties.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
    m_threadProperties.dwAffinityMask = 0;
	m_threadProperties.uStackSize = AK_DEFAULT_STACK_SIZE;
    
    // IMPORTANT NOTICE: This mus define ALL device settings since we 
    // cannot call Ak::StreamMgr::GetDefaultDeviceSettings() here, 
    // as the prebuilt binary contains generated SSE code to fill the FP value.
	m_deviceSettings.pIOMemory		    = NULL;
	m_deviceSettings.uIOMemoryAlignment	= AK_REQUIRED_IO_POOL_ALIGNMENT;
	m_deviceSettings.ePoolAttributes	= AK_DEFAULT_BLOCK_ALLOCATION_TYPE;
	m_deviceSettings.uMaxConcurrentIO	= AK_DEFAULT_MAX_CONCURRENT_IO;
	m_deviceSettings.uIOMemorySize = 1*1024*1024; // 1 Mb of memory for I/O.
    m_deviceSettings.uGranularity = 16*1024;
    m_deviceSettings.fTargetAutoStmBufferLength = 380;    // 380 ms buffering.
    m_deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_BLOCKING; // Scheduler blocking device.
	m_deviceSettings.threadProperties = m_threadProperties;

	AK::SoundEngine::GetDefaultInitSettings( m_initSettings );
	AK::SoundEngine::GetDefaultPlatformInitSettings( m_platformInitSettings );

	AK::MusicEngine::GetDefaultInitSettings( m_musicSettings );
}

AudConfig::~AudConfig()
{
}

bool AudConfig::OnModified( Be::Var* value )
{
	if( g_audioInitialized )
	{
		m_dirty = true;
	}
	return true;
}