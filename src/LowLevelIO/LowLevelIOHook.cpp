// Copyright (c) 2026 CCP Games

#include "LowLevelIOHook.h"
#include "FileHelpers.h"

#include <AK/Tools/Common/AkObject.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>

#include <CCPLog.h>

#if defined(__APPLE__)
#include <unistd.h>
#include <stdio.h>
#endif

namespace
{
#if defined(_WIN32)
    const wchar_t* const kDeviceName = L"Carbon Audio IO";
#else
    const char* const kDeviceName = "Carbon Audio IO";
#endif
}

LowLevelIOHook::LowLevelIOHook()
    : m_deviceID(AK_INVALID_DEVICE_ID)
{
}

LowLevelIOHook::~LowLevelIOHook()
{
}

AKRESULT LowLevelIOHook::Init(const AkDeviceSettings& settings)
{
#if defined(_WIN32)
    AKRESULT poolResult = m_pool.Init(settings.uMaxConcurrentIO);
    if (poolResult != AK_Success)
        return poolResult;
#endif

    if (!AK::StreamMgr::GetFileLocationResolver())
        AK::StreamMgr::SetFileLocationResolver(this);

    return AK::StreamMgr::CreateDevice(settings, this, m_deviceID);
}

void LowLevelIOHook::Term()
{
    AK::StreamMgr::DestroyDevice(m_deviceID);

    if (AK::StreamMgr::GetFileLocationResolver() == this)
        AK::StreamMgr::SetFileLocationResolver(nullptr);

#if defined(_WIN32)
    m_pool.Term();
#endif
}

AKRESULT LowLevelIOHook::SetBasePath(const AkOSChar* path)
{
    return m_resolver.SetBasePath(path);
}

AKRESULT LowLevelIOHook::SetEssentialPath(const AkOSChar* path)
{
    return m_resolver.SetEssentialPath(path);
}

AKRESULT LowLevelIOHook::SetAudioSrcPath(const AkOSChar* path)
{
    return m_resolver.SetAudioSrcPath(path);
}

AkFileDesc* LowLevelIOHook::CreateDescriptor(const AkFileDesc* copy)
{
    if (!copy)
        return AkNew(AkMemID_Streaming, AkFileDesc());

    return AkNew(AkMemID_Streaming, AkFileDesc(*copy));
}

AKRESULT LowLevelIOHook::Open(const AkFileOpenData& request, AkFileDesc*& outFileDesc)
{
    outFileDesc = CreateDescriptor();
    if (!outFileDesc)
        return AK_InsufficientMemory;

    AkOSChar path[AK_MAX_PATH];
    AKRESULT result = m_resolver.Resolve(request, path);
    if (result == AK_Success)
        result = FileHelpers::Open(path, request.eOpenMode, true, *outFileDesc);

    if (result == AK_Success)
    {
        outFileDesc->deviceID = m_deviceID;
        return AK_Success;
    }

#if defined(_WIN32)
    CCP_LOGERR("Failed to open audio file (id %u, path '%S'): result %d",
        (unsigned int)request.fileID, path, result);
#else
    CCP_LOGERR("Failed to open audio file (id %u, path '%s'): result %d",
        (unsigned int)request.fileID, path, result);
#endif

    AkDelete(AkMemID_Streaming, outFileDesc);
    outFileDesc = nullptr;
    return result;
}

void LowLevelIOHook::BatchOpen(AkUInt32 numFiles, AkAsyncFileOpenData** items)
{
    for (AkUInt32 i = 0; i < numFiles; ++i)
    {
        AkAsyncFileOpenData* item = items[i];
        AKRESULT result = Open(*item, item->pFileDesc);
        item->pCallback(item, result);
    }
}

void LowLevelIOHook::BatchRead(AkUInt32 numTransfers, BatchIoTransferItem* transferItems)
{
    for (AkUInt32 i = 0; i < numTransfers; ++i)
    {
        BatchIoTransferItem& item = transferItems[i];
        Read(*item.pFileDesc, item.ioHeuristics, *item.pTransferInfo);
    }
}

void LowLevelIOHook::BatchWrite(AkUInt32 numTransfers, BatchIoTransferItem* transferItems)
{
    for (AkUInt32 i = 0; i < numTransfers; ++i)
    {
        BatchIoTransferItem& item = transferItems[i];
        Write(*item.pFileDesc, item.ioHeuristics, *item.pTransferInfo);
    }
}

AKRESULT LowLevelIOHook::Close(AkFileDesc* fileDesc)
{
    if (!fileDesc)
        return AK_Success;

    AKRESULT result = FileHelpers::CloseFile(*fileDesc);
    AkDelete(AkMemID_Streaming, fileDesc);
    return result;
}

AkUInt32 LowLevelIOHook::GetBlockSize(AkFileDesc&)
{
    // Files are opened buffered (FILE_FLAG_SEQUENTIAL_SCAN, see FileHelpers::Open), so
    // the OS cache handles any offset and size and there is no alignment constraint. The
    // SDK says to return 1 when no byte alignment is required, letting reads and writes
    // start at any offset (see AK::StreamMgr::IAkLowLevelIOHook::GetBlockSize in
    // AkStreamMgrModule.h).
    return 1;
}

void LowLevelIOHook::GetDeviceDesc(AkDeviceDesc& deviceDesc)
{
#ifndef AK_OPTIMIZED
    AKASSERT(m_deviceID != AK_INVALID_DEVICE_ID);

    deviceDesc.deviceID = m_deviceID;
    deviceDesc.bCanRead = true;
    deviceDesc.bCanWrite = true;

  #if defined(_WIN32)
    AKPLATFORM::SafeStrCpy(deviceDesc.szDeviceName, kDeviceName, AK_MONITOR_DEVICENAME_MAXLENGTH);
    deviceDesc.uStringSize = (AkUInt32)wcslen(deviceDesc.szDeviceName) + 1;
  #else
    AK_CHAR_TO_UTF16(deviceDesc.szDeviceName, kDeviceName, AK_MONITOR_DEVICENAME_MAXLENGTH);
    deviceDesc.szDeviceName[AK_MONITOR_DEVICENAME_MAXLENGTH - 1] = '\0';
    deviceDesc.uStringSize = (AkUInt32)AKPLATFORM::AkUtf16StrLen(deviceDesc.szDeviceName) + 1;
  #endif
#else
    AK_UNUSEDVAR(deviceDesc);
#endif
}

AkUInt32 LowLevelIOHook::GetDeviceData()
{
    // Returning 1 tells Wwise that file opens are asynchronous.
    return 1;
}

#if defined(_WIN32)

VOID CALLBACK LowLevelIOHook::CompletionRoutine(DWORD errorCode, DWORD bytesTransferred, LPOVERLAPPED overlapped)
{
    if (!overlapped || !overlapped->hEvent)
        return;

    AkAsyncIOTransferInfo* transfer = (AkAsyncIOTransferInfo*)overlapped->hEvent;
    LowLevelIOHook* hook = (LowLevelIOHook*)transfer->pUserData;
    hook->ReleaseOverlapped(overlapped);

    AKRESULT result = AK_Fail;
    if (errorCode == ERROR_SUCCESS)
    {
        result = AK_Success;
        AKASSERTD(bytesTransferred >= transfer->uRequestedSize && bytesTransferred <= transfer->uBufferSize);
    }

    AK_UNUSEDVAR(bytesTransferred);
    transfer->pCallback(transfer, result);
}

OVERLAPPED* LowLevelIOHook::AcquireOverlapped(AkAsyncIOTransferInfo* transfer)
{
    if (!transfer)
        return nullptr;

    OVERLAPPED* overlapped = m_pool.Allocate();
    AKASSERT(overlapped || !"Too many concurrent transfers in the low level IO");
    if (!overlapped)
        return nullptr;

    // ReadFileEx ignores hEvent so we reuse it to carry the transfer.
    overlapped->hEvent = transfer;
    return overlapped;
}

void LowLevelIOHook::ReleaseOverlapped(OVERLAPPED* overlapped)
{
    if (!overlapped)
        return;

    m_pool.Deallocate(overlapped);
}

#endif

void LowLevelIOHook::Read(AkFileDesc& fileDesc, const AkIoHeuristics&, AkAsyncIOTransferInfo& transfer)
{
#if defined(_WIN32)
    AKASSERT(fileDesc.hFile != INVALID_HANDLE_VALUE
        && transfer.uRequestedSize > 0
        && transfer.uBufferSize >= transfer.uRequestedSize);

    transfer.pUserData = this;
    OVERLAPPED* overlapped = AcquireOverlapped(&transfer);
    if (!overlapped)
    {
        transfer.pCallback(&transfer, AK_InsufficientMemory);
        return;
    }

    overlapped->Offset = (DWORD)(transfer.uFilePosition & 0xFFFFFFFF);
    overlapped->OffsetHigh = (DWORD)((transfer.uFilePosition >> 32) & 0xFFFFFFFF);

    // Read the whole buffer so the transfer stays valid past the end of the file.
    if (::ReadFileEx(fileDesc.hFile, transfer.pBuffer, transfer.uBufferSize, overlapped, CompletionRoutine))
        return;

    ReleaseOverlapped(overlapped);
    AKRESULT result = AK_Fail;
    if (GetLastError() == ERROR_ACCESS_DENIED)
        result = AK_FilePermissionError;
    transfer.pCallback(&transfer, result);

#elif defined(__APPLE__)
    AkFileHandle handle = fileDesc.hFile;
    if (lseek(fileno(handle), transfer.uFilePosition, SEEK_SET) == (off_t)transfer.uFilePosition)
    {
        AkUInt32 transferred = (AkUInt32)read(fileno(handle), transfer.pBuffer, transfer.uRequestedSize);
        if (transferred == transfer.uRequestedSize)
        {
            transfer.pCallback(&transfer, AK_Success);
            return;
        }
    }

    transfer.pCallback(&transfer, AK_Fail);
#else
#error "LowLevelIOHook: unsupported platform"
#endif
}

void LowLevelIOHook::Write(AkFileDesc& fileDesc, const AkIoHeuristics&, AkAsyncIOTransferInfo& transfer)
{
#if defined(_WIN32)
    AKASSERT(fileDesc.hFile != INVALID_HANDLE_VALUE && transfer.uRequestedSize > 0);

    transfer.pUserData = this;
    OVERLAPPED* overlapped = AcquireOverlapped(&transfer);
    if (!overlapped)
    {
        transfer.pCallback(&transfer, AK_InsufficientMemory);
        return;
    }

    overlapped->Offset = (DWORD)(transfer.uFilePosition & 0xFFFFFFFF);
    overlapped->OffsetHigh = (DWORD)((transfer.uFilePosition >> 32) & 0xFFFFFFFF);

    if (::WriteFileEx(fileDesc.hFile, transfer.pBuffer, transfer.uRequestedSize, overlapped, CompletionRoutine))
        return;

    ReleaseOverlapped(overlapped);
    AKRESULT result = AK_Fail;
    if (GetLastError() == ERROR_ACCESS_DENIED)
        result = AK_FilePermissionError;
    transfer.pCallback(&transfer, result);

#elif defined(__APPLE__)
    AkFileHandle handle = fileDesc.hFile;
    if (lseek(fileno(handle), transfer.uFilePosition, SEEK_SET) == (off_t)transfer.uFilePosition)
    {
        AkUInt32 transferred = (AkUInt32)write(fileno(handle), transfer.pBuffer, transfer.uRequestedSize);
        if (transferred == transfer.uRequestedSize)
        {
            transfer.pCallback(&transfer, AK_Success);
            fflush(handle);
            return;
        }
    }

    transfer.pCallback(&transfer, AK_Fail);
#else
#error "LowLevelIOHook: unsupported platform"
#endif
}
