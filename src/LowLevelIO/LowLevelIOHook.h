// Copyright © 2026 CCP ehf.

#pragma once

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkAssert.h>

#if defined(_WIN32)
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkObjectPool.h>
#endif

#include "AudPathResolver.h"

/**
 * @class LowLevelIOHook
 * @brief Services Wwise streaming reads and writes through asynchronous file IO.
 *
 * Registers as the Wwise file location resolver and low level IO hook, resolves
 * paths with AudPathResolver, opens files and runs reads and
 * writes through IO on Windows.
 */
class LowLevelIOHook : public AK::StreamMgr::IAkFileLocationResolver
                     , public AK::StreamMgr::IAkLowLevelIOHook
{
public:
    LowLevelIOHook();
    virtual ~LowLevelIOHook();

    /** @brief Register as the file location resolver and create the streaming device. */
    AKRESULT Init(const AkDeviceSettings& settings);
    /** @brief Destroy the streaming device and release resources. */
    void Term();

    /** @brief Set the base folder prepended to every resolved path. */
    AKRESULT SetBasePath(const AkOSChar* path);
    /** @brief Set the folder holding the essential banks and media, a more compact version of the music and sfx. */
    AKRESULT SetEssentialPath(const AkOSChar* path);
    /** @brief Set the folder that holds streamed audio source files. */
    AKRESULT SetAudioSrcPath(const AkOSChar* path);

    // IAkLowLevelIOHook
    void BatchOpen(AkUInt32 numFiles, AkAsyncFileOpenData** items) override;
    void BatchRead(AkUInt32 numTransfers, BatchIoTransferItem* transferItems) override;
    void BatchWrite(AkUInt32 numTransfers, BatchIoTransferItem* transferItems) override;
    AKRESULT Close(AkFileDesc* fileDesc) override;
    AkUInt32 GetBlockSize(AkFileDesc& fileDesc) override;
    void GetDeviceDesc(AkDeviceDesc& deviceDesc) override;
    AkUInt32 GetDeviceData() override;

private:
    // Resolve and open one file. Called per item by BatchOpen.
    AKRESULT Open(const AkFileOpenData& request, AkFileDesc*& outFileDesc);

    AkFileDesc* CreateDescriptor(const AkFileDesc* copy = nullptr);

    void Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, AkAsyncIOTransferInfo& transfer);
    void Write(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, AkAsyncIOTransferInfo& transfer);

#if defined(_WIN32)
    static VOID CALLBACK CompletionRoutine(DWORD errorCode, DWORD bytesTransferred, LPOVERLAPPED overlapped);

    using OverlappedPool = AK::ObjectPool<OVERLAPPED, AK::ObjectPoolDefaultAllocator<AkMemID_Streaming>>;
    OVERLAPPED* AcquireOverlapped(AkAsyncIOTransferInfo* transfer);
    void ReleaseOverlapped(OVERLAPPED* overlapped);

    OverlappedPool m_pool;
#endif

    AudPathResolver m_resolver;
    AkDeviceID m_deviceID;
};
