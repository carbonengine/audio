////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

/**
 * @brief Threads that run Wwise's audio jobs next to its audio thread.
 *
 * Start after AK::MemoryMgr::Init and before AK::SoundEngine::Init. Stop after AK::SoundEngine::Term
 * and before AK::MemoryMgr::Term: Wwise expects the workers to answer for as long as it runs.
 * Start, Stop and GetThreadCount are for the thread that owns the pool.
 */
class AudJobWorkerPool
{
public:
	static constexpr uint32_t MAX_THREADS = 8;

	AudJobWorkerPool() = default;
	~AudJobWorkerPool();

	AudJobWorkerPool( const AudJobWorkerPool& ) = delete;
	AudJobWorkerPool& operator=( const AudJobWorkerPool& ) = delete;

	/// Threads to run for a request, leaving one logical core for the rest of the game.
	static uint32_t ClampThreadCount( int requested, uint32_t logicalCores );

	/// Returns false and leaves no thread running if any thread fails to start.
	bool Start( uint32_t threadCount );
	void Stop();
	uint32_t GetThreadCount() const;
	/// Worker requests from Wwise served since Start. Callable from any thread.
	uint64_t GetServedRequestCount() const;

	/// Point Wwise's job manager at this pool. Call after Start succeeded.
	void FillSettings( AkJobMgrSettings& settings );

private:
	static void AKSOUNDENGINE_CALL RequestJobWorker( AkJobWorkerFunc workerFunc, AkJobType jobType, AkUInt32 numWorkers, void* clientData );
	static AK_DECLARE_THREAD_ROUTINE( ThreadRoutine );

	void Run();
	bool HasRequestLocked() const;
	AkJobType TakeRequestLocked();

	mutable std::mutex m_mutex;
	std::condition_variable m_wake;
	AkUInt32 m_pendingRequests[AK_NUM_JOB_TYPES] = {};
	uint64_t m_servedRequests = 0;
	AkJobWorkerFunc m_workerFunc = nullptr;
	bool m_stopping = false;

	AkThreadProperties m_threadProperties = {};
	std::vector<AkThread> m_threads;
};
