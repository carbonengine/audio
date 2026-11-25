// Copyright © 2026 Fenris Creations

#pragma once

#include <AK/SoundEngine/Common/AkCallback.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <CcpMutex.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

// Wraps interleaved 16-bit little-endian PCM in the WEM container Wwise reads for an in-memory External Source.
// Mono and stereo only. Logs and returns an empty vector if the PCM can't be wrapped.
std::vector<uint8_t> BuildPcmWem( const uint8_t* pcm, size_t pcmSize, int sampleRate, int channels );

// ------------------------------------------------------------------------
// Description:
//   Owns the WEMs that in-memory External Sources play from, keyed by playing ID.
//   Wwise reads a WEM for as long as its event plays, so each one is kept until that
//   event's AK_EndOfEvent, or until AudManager is destroyed.
//   AudManager owns the store rather than the game object, so a WEM outlives a game
//   object destroyed while its event is still fading out.
// ------------------------------------------------------------------------
class AudPcmSourceBuffers
{
public:
	AudPcmSourceBuffers();

	// Posts the event with the WEM as its External Source, keeping the WEM if the post succeeds.
	// The callback gets this store as its cookie.
	AkPlayingID PostHolding( std::vector<uint8_t>&& wem, AkUInt32 sourceCookie, AkUniqueID eventID, AkGameObjectID gameObjectID, AkUInt32 flags, AkCallbackFunc callback );
	// Frees the WEM held for a playing ID, if there is one.
	void Release( AkPlayingID playingID );
	// How many WEMs are held.
	size_t GetHeldCount();

private:
	std::map<AkPlayingID, std::vector<uint8_t>> m_buffers;
	// Held while posting, so an AK_EndOfEvent can't call Release before its WEM is recorded.
	CcpMutex m_mutex;
};
