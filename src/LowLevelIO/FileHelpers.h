// Copyright (c) 2026 CCP Games

#pragma once

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/Tools/Common/AkAssert.h>

#include <filesystem>
#include <system_error>

#if defined(__APPLE__)
#include <stdio.h>
#include <sys/errno.h>
#endif

/**
 * @class FileHelpers
 * @brief Opens and closes files for the Wwise Stream Manager.
 *
 * Reports file size to the Stream Manager. On Windows files are opened for
 * overlapped IO so streaming reads do not block.
 */
class FileHelpers
{
public:

    /**
     * @brief Open a file and fill in the descriptor with its handle and size.
     * @param path Path of the file to open.
     * @param mode Open mode such as read, write, overwrite or read/write.
     * @param overlapped When true the file is opened for asynchronous IO on Windows.
     * @param desc Descriptor filled with the file handle and size on success.
     * @return AK_Success, or an error code if the file could not be opened.
     */
    static AKRESULT Open(
        const AkOSChar* path,
        AkOpenMode      mode,
        bool            overlapped,
        AkFileDesc&     desc)
    {
        if (!path)
        {
            AKASSERT(!"NULL file name");
            return AK_InvalidParameter;
        }

#if defined(_WIN32)
        struct Win32Mode { AkOpenMode mode; DWORD access; DWORD share; DWORD disposition; };
        static const Win32Mode modeTable[] =
        {
            { AK_OpenModeRead,       GENERIC_READ,                 FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING },
            { AK_OpenModeWrite,      GENERIC_WRITE,                FILE_SHARE_READ,                    OPEN_ALWAYS   },
            { AK_OpenModeWriteOvrwr, GENERIC_WRITE,                FILE_SHARE_READ,                    CREATE_ALWAYS },
            { AK_OpenModeReadWrite,  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,                    OPEN_ALWAYS   },
        };

        const Win32Mode* selected = nullptr;
        for (const Win32Mode& entry : modeTable)
        {
            if (entry.mode == mode)
            {
                selected = &entry;
                break;
            }
        }
        if (!selected)
        {
            AKASSERT(!"Invalid open mode");
            desc.hFile = INVALID_HANDLE_VALUE;
            return AK_InvalidParameter;
        }

        CREATEFILE2_EXTENDED_PARAMETERS params = {};
        params.dwSize = sizeof(params);
        params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        params.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
        if (overlapped)
            params.dwFileFlags |= FILE_FLAG_OVERLAPPED;

        desc.hFile = ::CreateFile2(
            path,
            selected->access,
            selected->share,
            selected->disposition,
            &params);

        if (desc.hFile == INVALID_HANDLE_VALUE)
        {
            switch (::GetLastError())
            {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return AK_FileNotFound;
            case ERROR_ACCESS_DENIED:
            case ERROR_SHARING_VIOLATION:
                return AK_FilePermissionError;
            default:
                return AK_UnknownFileError;
            }
        }

        std::error_code ec;
        auto fileSize = std::filesystem::file_size(std::filesystem::path(path), ec);
        if (ec)
            desc.iFileSize = 0;
        else
            desc.iFileSize = (AkInt64)fileSize;
        return AK_Success;

#elif defined(__APPLE__)
        AK_UNUSEDVAR(overlapped);

        const char* stdioMode;
        switch (mode)
        {
        case AK_OpenModeRead:       stdioMode = "r";  break;
        case AK_OpenModeWrite:      stdioMode = "w";  break;
        case AK_OpenModeWriteOvrwr: stdioMode = "w+"; break;
        case AK_OpenModeReadWrite:  stdioMode = "a";  break;
        default:
            AKASSERT(!"Invalid open mode");
            desc.hFile = NULL;
            return AK_InvalidParameter;
        }

        desc.hFile = fopen(path, stdioMode);
        if (!desc.hFile)
        {
            if (errno == EACCES)
                return AK_FilePermissionError;
            return AK_FileNotFound;
        }

        std::error_code ec;
        auto fileSize = std::filesystem::file_size(std::filesystem::path(path), ec);
        if (ec)
            desc.iFileSize = 0;
        else
            desc.iFileSize = (AkInt64)fileSize;
        return AK_Success;
#else
#error "FileHelpers: unsupported platform"
#endif
    }

    /**
     * @brief Close the file referenced by the descriptor.
     * @param desc Descriptor whose file handle is closed.
     * @return AK_Success, or AK_Fail if the handle could not be closed.
     */
    static AKRESULT CloseFile(const AkFileDesc& desc)
    {
#if defined(_WIN32)
        ::FlushFileBuffers(desc.hFile);
        if (::CloseHandle(desc.hFile))
            return AK_Success;

        AKASSERT(!"Failed to close file handle");
        return AK_Fail;
#elif defined(__APPLE__)
        if (!fclose(desc.hFile))
            return AK_Success;

        AKASSERT(!"Failed to close file handle");
        return AK_Fail;
#else
#error "FileHelpers: unsupported platform"
#endif
    }
};
