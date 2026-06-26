// Copyright © 2026 CCP ehf.

#pragma once

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

/**
 * @class AudPathResolver
 * @brief Builds the full file path for a Wwise open request.
 *
 * Resolves the essential and audio source folders and uses them with the
 * request codec and language to build the path a file is read from.
 */
class AudPathResolver
{
public:
    AudPathResolver();

    /** @brief Set the base folder prepended to every resolved path. */
    AKRESULT SetBasePath(const AkOSChar* path);
    /** @brief Set the folder holding the essential banks and media, a more compact version of the music and sfx. */
    AKRESULT SetEssentialPath(const AkOSChar* path);
    /** @brief Set the folder that holds streamed audio source files. */
    AKRESULT SetAudioSrcPath(const AkOSChar* path);

    /**
     * @brief Build the file path for an open request.
     * @param request The Wwise open request with its codec, file id, flags and name.
     * @param outPath Buffer of AK_MAX_PATH that receives the resolved path.
     * @return AK_Success when a path was produced.
     */
    AKRESULT Resolve(const AkFileOpenData& request, AkOSChar* outPath);

private:
    AkOSChar m_basePath[AK_MAX_PATH];
    AkOSChar m_essentialPath[AK_MAX_PATH];
    AkOSChar m_audioSrcPath[AK_MAX_PATH];
};
