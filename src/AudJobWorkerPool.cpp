////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudJobWorkerPool.h"

#include <algorithm>
#include <cstdio>

AudJobWorkerPool::~AudJobWorkerPool()
{
	Stop();
}

uint32_t AudJobWorkerPool::ClampThreadCount( int requested, uint32_t logicalCores )
{
	if( requested <= 0 || logicalCores < 2 )
	{
		return 0;
	}
	return std::min( { static_cast<uint32_t>( requested ), MAX_THREADS, logicalCores - 1 } );
}

bool AudJobWorkerPool::Start( uint32_t threadCount )
{
	if( threadCount == 0 || threadCount > MAX_THREADS || !m_threads.empty() )
	{
		return false;
	}

	{
		// Nothing asked for before these threads exist may reach them.
		std::lock_guard<std::mutex> lock( m_mutex );
		std::fill( std::begin( m_pendingRequests ), std::end( m_pendingRequests ), 0u );
		m_servedRequests = 0;
	}

	AKPLATFORM::AkGetDefaultHighPriorityThreadProperties( m_threadProperties );
	m_threads.reserve( threadCount );
	for( uint32_t i = 0; i < threadCount; ++i )
	{
		char name[32];
		snprintf( name, sizeof( name ), "CarbonAudio::JobWorker-%u", i );

		AkThread thread;
		AKPLATFORM::AkClearThread( &thread );
		const AKRESULT result = AKPLATFORM::AkCreateThread( ThreadRoutine, this, m_threadProperties, &thread, name, nullptr );
		if( result != AK_Success || !AKPLATFORM::AkIsValidThread( &thread ) )
		{
			Stop();
			return false;
		}
		m_threads.push_back( thread );
	}
	return true;
}

void AudJobWorkerPool::Stop()
{
	if( m_threads.empty() )
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock( m_mutex );
		m_stopping = true;
	}
	m_wake.notify_all();

	for( AkThread& thread : m_threads )
	{
		AKPLATFORM::AkWaitForSingleThread( &thread );
		AKPLATFORM::AkCloseThread( &thread, nullptr );
	}
	m_threads.clear();

	std::lock_guard<std::mutex> lock( m_mutex );
	m_stopping = false;
	std::fill( std::begin( m_pendingRequests ), std::end( m_pendingRequests ), 0u );
}

uint32_t AudJobWorkerPool::GetThreadCount() const
{
	return static_cast<uint32_t>( m_threads.size() );
}

uint64_t AudJobWorkerPool::GetServedRequestCount() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_servedRequests;
}

void AudJobWorkerPool::FillSettings( AkJobMgrSettings& settings )
{
	const AkUInt32 threadCount = GetThreadCount();
	settings.fnRequestJobWorker = RequestJobWorker;
	settings.pClientData = this;
	for( AkUInt32 jobType = 0; jobType < AK_NUM_JOB_TYPES; ++jobType )
	{
		settings.uMaxActiveWorkers[jobType] = threadCount;
	}
	// One slab for each worker and one for the audio thread.
	settings.uNumMemorySlabs = threadCount + 1;
}

// Called by Wwise from its audio thread and from inside jobs on the workers.
void AKSOUNDENGINE_CALL AudJobWorkerPool::RequestJobWorker( AkJobWorkerFunc workerFunc, AkJobType jobType, AkUInt32 numWorkers, void* clientData )
{
	AudJobWorkerPool* pool = static_cast<AudJobWorkerPool*>( clientData );
	AKASSERT( pool != nullptr && jobType < AK_NUM_JOB_TYPES );
	if( pool == nullptr || jobType >= AK_NUM_JOB_TYPES )
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock( pool->m_mutex );
		pool->m_workerFunc = workerFunc;
		pool->m_pendingRequests[jobType] += numWorkers;
	}

	if( numWorkers == 1 )
	{
		pool->m_wake.notify_one();
	}
	else
	{
		pool->m_wake.notify_all();
	}
}

AK_DECLARE_THREAD_ROUTINE( AudJobWorkerPool::ThreadRoutine )
{
	AudJobWorkerPool* pool = AK_GET_THREAD_ROUTINE_PARAMETER_PTR( AudJobWorkerPool );
	AK_THREAD_INIT_CODE( pool->m_threadProperties );
	pool->Run();
	AkExitThread( AK_RETURN_THREAD_OK );
}

void AudJobWorkerPool::Run()
{
	AK::MemoryMgr::InitForThread();

	std::unique_lock<std::mutex> lock( m_mutex );
	while( true )
	{
		m_wake.wait( lock, [this] { return m_stopping || HasRequestLocked(); } );
		if( m_stopping )
		{
			break;
		}

		// Wwise expects exactly one call of its worker function for each worker it requested.
		const AkJobType jobType = TakeRequestLocked();
		const AkJobWorkerFunc workerFunc = m_workerFunc;
		++m_servedRequests;
		lock.unlock();

		workerFunc( jobType, 0 );

		lock.lock();
		if( !m_stopping && !HasRequestLocked() )
		{
			lock.unlock();
			AK::MemoryMgr::TrimForThread();
			lock.lock();
		}
	}
	lock.unlock();

	AK::MemoryMgr::TermForThread();
}

bool AudJobWorkerPool::HasRequestLocked() const
{
	for( AkUInt32 pending : m_pendingRequests )
	{
		if( pending > 0 )
		{
			return true;
		}
	}
	return false;
}

AkJobType AudJobWorkerPool::TakeRequestLocked()
{
	for( AkJobType jobType = 0; jobType < AK_NUM_JOB_TYPES; ++jobType )
	{
		if( m_pendingRequests[jobType] > 0 )
		{
			--m_pendingRequests[jobType];
			return jobType;
		}
	}
	AKASSERT( !"TakeRequestLocked called with no pending request" );
	return AkJobType_Generic;
}
