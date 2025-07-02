/*******************************************************************************

  Author: Phevos Rinis
  Created: May 2025
  Description: Custom IO hook for Wwise that handles specific path resolution
               for bank files in Essential_Media and audio files in Media folders.
               Integrates with BluePaths system for file resolution and on-demand
               content downloading.
*******************************************************************************/

#pragma once

#include "LowLevelIO/Common/AkFilePackageLowLevelIODeferred.h"
#include "LowLevelIO/Common/AkFileHelpersBase.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkAssert.h>
#include "Audio2.h"


#define MAX_NUMBER_STRING_SIZE      (10) 
#define MAX_EXTENSION_SIZE          (4) 
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE+MAX_EXTENSION_SIZE+1)

BLUE_DECLARE_INTERFACE(IBlueStream);  // Forward declaration for BluePaths

class CCPFilePackageLowLevelIO : public CAkFilePackageLowLevelIODeferred
{
public:

    CCPFilePackageLowLevelIO()
    {
        m_szBasePath[0] = '\0';
        m_szEssentialPath[0] = '\0';
        m_szAudioSrcPath[0] = '\0';
    }

    virtual AKRESULT Open(const AkFileOpenData& in_FileOpen, AkFileDesc*& out_pFileDesc) override
    {
        out_pFileDesc = CreateDescriptor();
        if (!out_pFileDesc)
            return AK_InsufficientMemory;

        // Create full file path based on file type
        AkOSChar szFullFilePath[AK_MAX_PATH] = { 0 };

        if (in_FileOpen.pFlags && in_FileOpen.eOpenMode == AK_OpenModeRead)
        {
            // Get base path
            AKPLATFORM::SafeStrCpy(szFullFilePath, m_szBasePath, AK_MAX_PATH);

            // Make sure the base path ends with a separator
            size_t baseLen = AKPLATFORM::OsStrLen(szFullFilePath);
            if (baseLen > 0 && szFullFilePath[baseLen - 1] != AK_PATH_SEPARATOR[0])
                AKPLATFORM::SafeStrCat(szFullFilePath, AK_PATH_SEPARATOR, AK_MAX_PATH);

            // IMPORTANT: Check language-specific FIRST
            if (in_FileOpen.pFlags->bIsLanguageSpecific)
            {
                // For language-specific files, ONLY add the language folder
                const AkOSChar* language = AK::StreamMgr::GetCurrentLanguage();
                size_t uLanguageStrLen = AKPLATFORM::OsStrLen(language);
                if (uLanguageStrLen > 0)
                {
                    AKPLATFORM::SafeStrCat(szFullFilePath, language, AK_MAX_PATH);
                    AKPLATFORM::SafeStrCat(szFullFilePath, AK_PATH_SEPARATOR, AK_MAX_PATH);
                }
            }
            else
            {
                // Only for NON-language-specific files:
                if (in_FileOpen.pFlags->uCodecID == AKCODECID_BANK)
                {
                    // Bank files go in Essential_Media
                    AKPLATFORM::SafeStrCat(szFullFilePath, m_szEssentialPath, AK_MAX_PATH);
                }
                else
                {
                    // Other files (WEM) go in Media
                    AKPLATFORM::SafeStrCat(szFullFilePath, m_szAudioSrcPath, AK_MAX_PATH);
                }

                // Add separator after subfolder
                size_t pathLen = AKPLATFORM::OsStrLen(szFullFilePath);
                if (pathLen > 0 && szFullFilePath[pathLen - 1] != AK_PATH_SEPARATOR[0])
                    AKPLATFORM::SafeStrCat(szFullFilePath, AK_PATH_SEPARATOR, AK_MAX_PATH);
            }

            // Add the file name
            const AkOSChar* pFileName = in_FileOpen.pszFileName;
            AkOSChar szFileName[MAX_FILETITLE_SIZE + 1] = { 0 };
            if (!pFileName && in_FileOpen.pFlags)
            {
                if (in_FileOpen.pFlags->uCodecID == AKCODECID_BANK)
                    AK_OSPRINTF(szFileName, MAX_FILETITLE_SIZE, AKTEXT("%u.bnk"), (unsigned int)in_FileOpen.fileID);
                else
                    AK_OSPRINTF(szFileName, MAX_FILETITLE_SIZE, AKTEXT("%u.wem"), (unsigned int)in_FileOpen.fileID);
                pFileName = szFileName;
            }
            AKPLATFORM::SafeStrCat(szFullFilePath, pFileName, AK_MAX_PATH);

            // Process through BluePaths
            AkOSChar resolvedPath[AK_MAX_PATH];
            if (GetFullFilePathFromResPath(szFullFilePath, resolvedPath) == AK_Success)
            {
                // Open the file using platform-specific method
                AKRESULT eResult = CAkFileHelpers::Open(resolvedPath, in_FileOpen.eOpenMode, true, *out_pFileDesc);
                if (eResult == AK_Success)
                {
                    out_pFileDesc->deviceID = m_deviceID;
                    return AK_Success;
                }
            }
        }
        else
        {
            // For non-read operations, use standard behavior
            return CAkFilePackageLowLevelIODeferred::Open(in_FileOpen, out_pFileDesc);
        }

        AkDelete(AkMemID_Streaming, out_pFileDesc);
        out_pFileDesc = nullptr;
        return AK_FileNotFound;
    }

    // BluePaths integration
    AKRESULT GetFullFilePathFromResPath(AkOSChar* in_pszResFilePath, AkOSChar* out_pszFullFilePath)
    {
#if _WIN32
        wchar_t* resFilePath = in_pszResFilePath;
#else
        CA2W tmp_wchar(in_pszResFilePath);
        auto resFilePath = (const wchar_t*)tmp_wchar;
#endif

        if (BePaths->FileNeedsDownload(resFilePath))
        {
            // File does not exist locally - reference it to force a download
            IBlueStreamPtr dummy;
            BePaths->GetStreamFromPathW(resFilePath, &dummy);
        }

        std::wstring fileOnDisk = BePaths->ResolvePathW(resFilePath);

        if (fileOnDisk.size() > AK_MAX_PATH)
        {
            return AK_Fail;
        }

#if _WIN32
        AKPLATFORM::SafeStrCpy(out_pszFullFilePath, fileOnDisk.c_str(), AK_MAX_PATH);
#else
        CW2A tmp_char(fileOnDisk.c_str());
        AKPLATFORM::SafeStrCpy(out_pszFullFilePath, tmp_char, AK_MAX_PATH);
#endif
        return AK_Success;
    }

    // Path members and setters
    AkOSChar m_szBasePath[AK_MAX_PATH];
    AkOSChar m_szEssentialPath[AK_MAX_PATH];
    AkOSChar m_szAudioSrcPath[AK_MAX_PATH];

    AKRESULT SetBasePath(const AkOSChar* in_pszBasePath)
    {
        AKPLATFORM::SafeStrCpy(m_szBasePath, in_pszBasePath, AK_MAX_PATH);
        return AK_Success;
    }

    AKRESULT SetEssentialPath(const AkOSChar* in_pszEssentialPath)
    {
        AKPLATFORM::SafeStrCpy(m_szEssentialPath, in_pszEssentialPath, AK_MAX_PATH);
        return AK_Success;
    }

    AKRESULT SetAudioSrcPath(const AkOSChar* in_pszAudioSrcPath)
    {
        AKPLATFORM::SafeStrCpy(m_szAudioSrcPath, in_pszAudioSrcPath, AK_MAX_PATH);
        return AK_Success;
    }
};