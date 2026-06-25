// Copyright (c) 2026 CCP Games

#include "AudPathResolver.h"

#include "Audio2.h"
#include "AudStaticDataRepository.h"

#include <string>

BLUE_DECLARE_INTERFACE(IBlueStream);

namespace
{
    // Enough room for a uint32 file id plus a .wem or .bnk extension and a null.
    constexpr int kFileNameSize = 16;

    void AppendSeparator(AkOSChar* path)
    {
        if (!path)
            return;

        size_t length = AKPLATFORM::OsStrLen(path);
        if (length > 0 && path[length - 1] != AK_PATH_SEPARATOR[0])
            AKPLATFORM::SafeStrCat(path, AK_PATH_SEPARATOR, AK_MAX_PATH);
    }

    std::wstring ToWString(const AkOSChar* str)
    {
        if (!str)
            return std::wstring();
#if defined(_WIN32)
        return std::wstring(str);
#else
        CA2W converted(str);
        return std::wstring((const wchar_t*)converted);
#endif
    }

    const AkOSChar* FileNameFor(const AkFileOpenData& request, AkOSChar* buffer)
    {
        if (request.pszFileName)
            return request.pszFileName;
        if (!request.pFlags)
            return nullptr;

        const AkOSChar* format = AKTEXT("%u.wem");
        if (request.pFlags->uCodecID == AKCODECID_BANK)
            format = AKTEXT("%u.bnk");

        if (!buffer)
            return nullptr;

        AK_OSPRINTF(buffer, kFileNameSize - 1, format, (unsigned int)request.fileID);
        return buffer;
    }

    bool IsFileEssential(const AkFileOpenData& request)
    {
        if (request.pFlags->uCodecID == AKCODECID_BANK)
            return g_staticDataRepository->SoundBankIsEssential(ToWString(request.pszFileName));

        return g_staticDataRepository->SourceIsEssential(request.fileID);
    }

    // Ask BluePaths for the real file path and download the file if it is missing.
    AKRESULT ResolveResPath(const AkOSChar* resPath, AkOSChar* outPath)
    {
        if (!resPath || !outPath)
            return AK_InvalidParameter;

#if defined(_WIN32)
        const wchar_t* wideResPath = resPath;
#else
        CA2W converted(resPath);
        const wchar_t* wideResPath = (const wchar_t*)converted;
#endif

        if (BePaths->FileNeedsDownload(wideResPath))
        {
            IBlueStreamPtr stream;
            BePaths->GetStreamFromPathW(wideResPath, &stream);
        }

        std::wstring onDisk = BePaths->ResolvePathW(wideResPath);
        if (onDisk.size() > AK_MAX_PATH)
            return AK_Fail;

#if defined(_WIN32)
        AKPLATFORM::SafeStrCpy(outPath, onDisk.c_str(), AK_MAX_PATH);
#else
        CW2A narrow(onDisk.c_str());
        AKPLATFORM::SafeStrCpy(outPath, narrow, AK_MAX_PATH);
#endif
        return AK_Success;
    }
}

AudPathResolver::AudPathResolver()
{
    m_basePath[0] = '\0';
    m_essentialPath[0] = '\0';
    m_audioSrcPath[0] = '\0';
}

AKRESULT AudPathResolver::SetBasePath(const AkOSChar* path)
{
    if (!path)
        return AK_InvalidParameter;

    AKPLATFORM::SafeStrCpy(m_basePath, path, AK_MAX_PATH);
    return AK_Success;
}

AKRESULT AudPathResolver::SetEssentialPath(const AkOSChar* path)
{
    if (!path)
        return AK_InvalidParameter;

    AKPLATFORM::SafeStrCpy(m_essentialPath, path, AK_MAX_PATH);
    return AK_Success;
}

AKRESULT AudPathResolver::SetAudioSrcPath(const AkOSChar* path)
{
    if (!path)
        return AK_InvalidParameter;

    AKPLATFORM::SafeStrCpy(m_audioSrcPath, path, AK_MAX_PATH);
    return AK_Success;
}

AKRESULT AudPathResolver::Resolve(const AkFileOpenData& request, AkOSChar* outPath)
{
    if (!outPath)
        return AK_InvalidParameter;
    outPath[0] = '\0';

    AkOSChar nameBuffer[kFileNameSize] = { 0 };

    if (request.pFlags && request.eOpenMode == AK_OpenModeRead)
    {
        AkOSChar fullPath[AK_MAX_PATH] = { 0 };
        AKPLATFORM::SafeStrCpy(fullPath, m_basePath, AK_MAX_PATH);
        AppendSeparator(fullPath);

        // Language files are checked first. They only get the language folder.
        if (request.pFlags->bIsLanguageSpecific)
        {
            const AkOSChar* language = AK::StreamMgr::GetCurrentLanguage();
            if (AKPLATFORM::OsStrLen(language) > 0)
            {
                AKPLATFORM::SafeStrCat(fullPath, language, AK_MAX_PATH);
                AKPLATFORM::SafeStrCat(fullPath, AK_PATH_SEPARATOR, AK_MAX_PATH);
            }
        }
        else
        {
            // Banks here stay at the base folder while media goes in the audio source folder.
            if (IsFileEssential(request))
                AKPLATFORM::SafeStrCat(fullPath, m_essentialPath, AK_MAX_PATH);
            else if (request.pFlags->uCodecID != AKCODECID_BANK)
                AKPLATFORM::SafeStrCat(fullPath, m_audioSrcPath, AK_MAX_PATH);

            AppendSeparator(fullPath);
        }

        const AkOSChar* fileName = FileNameFor(request, nameBuffer);
        if (!fileName)
            return AK_FileNotFound;

        AKPLATFORM::SafeStrCat(fullPath, fileName, AK_MAX_PATH);
        return ResolveResPath(fullPath, outPath);
    }

    const AkOSChar* fileName = FileNameFor(request, nameBuffer);
    if (!fileName)
        return AK_FileNotFound;

    AKPLATFORM::SafeStrCpy(outPath, fileName, AK_MAX_PATH);
    return AK_Success;
}
