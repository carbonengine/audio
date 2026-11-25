// Copyright © 2026 Fenris Creations

#include "stdafx.h"
#include "AudPcmSource.h"

#include <AK/SoundEngine/Common/AkWavDefs.h>

#include <limits>

namespace
{
	constexpr int PCM_BYTES_PER_SAMPLE = 2;

	template<typename T>
	void Append( std::vector<uint8_t>& out, const T& value )
	{
		const uint8_t* bytes = reinterpret_cast<const uint8_t*>( &value );
		out.insert( out.end(), bytes, bytes + sizeof( T ) );
	}
}

std::vector<uint8_t> BuildPcmWem( const uint8_t* pcm, size_t pcmSize, int sampleRate, int channels )
{
	if( channels != 1 && channels != 2 )
	{
		CCP_LOGERR( "In-memory PCM must be mono or stereo, not %d channels.", channels );
		return {};
	}
	if( sampleRate <= 0 )
	{
		CCP_LOGERR( "In-memory PCM sample rate %d is not positive.", sampleRate );
		return {};
	}
	const size_t blockAlign = static_cast<size_t>( channels ) * PCM_BYTES_PER_SAMPLE;
	if( pcm == nullptr || pcmSize == 0 || pcmSize % blockAlign != 0 )
	{
		CCP_LOGERR( "In-memory PCM of %zu bytes is not a whole number of 16-bit %d-channel frames.", pcmSize, channels );
		return {};
	}

	WemFormatExtensible format = {};
	format.wFormatTag = extensibleWavFormat;
	format.nChannels = static_cast<AkUInt16>( channels );
	format.nSamplesPerSec = static_cast<AkUInt32>( sampleRate );
	format.nAvgBytesPerSec = static_cast<AkUInt32>( sampleRate * blockAlign );
	format.nBlockAlign = static_cast<AkUInt16>( blockAlign );
	format.wBitsPerSample = PCM_BYTES_PER_SAMPLE * 8;
	format.cbSize = sizeof( WemFormatExtensible ) - sizeof( WaveFormatEx );
	format.wSamplesPerBlock = 0;
	AkChannelConfig channelConfig;
	channelConfig.SetStandard( channels == 1 ? AK_SPEAKER_SETUP_MONO : AK_SPEAKER_SETUP_STEREO );
	format.uChannelConfig = channelConfig.Serialize();

	// RIFF size counts everything after its own header: the WAVE id, then the fmt and data chunks.
	const size_t riffSize = sizeof( AkFourcc ) + sizeof( AkChunkHeader ) + sizeof( format ) + sizeof( AkChunkHeader ) + pcmSize;
	if( riffSize > std::numeric_limits<AkUInt32>::max() - sizeof( AkChunkHeader ) )
	{
		CCP_LOGERR( "In-memory PCM of %zu bytes is too big for a WEM.", pcmSize );
		return {};
	}

	std::vector<uint8_t> wem;
	wem.reserve( sizeof( AkChunkHeader ) + riffSize );
	Append( wem, AkChunkHeader{ RIFFChunkId, static_cast<AkUInt32>( riffSize ) } );
	Append( wem, WAVEChunkId );
	Append( wem, AkChunkHeader{ fmtChunkId, sizeof( format ) } );
	Append( wem, format );
	Append( wem, AkChunkHeader{ dataChunkId, static_cast<AkUInt32>( pcmSize ) } );
	wem.insert( wem.end(), pcm, pcm + pcmSize );
	return wem;
}

AudPcmSourceBuffers::AudPcmSourceBuffers() : m_mutex( "AudPcmSourceBuffers", "m_mutex" )
{
}

AkPlayingID AudPcmSourceBuffers::PostHolding( std::vector<uint8_t>&& wem, AkUInt32 sourceCookie, AkUniqueID eventID, AkGameObjectID gameObjectID, AkUInt32 flags, AkCallbackFunc callback )
{
	CcpAutoMutex lock( m_mutex );
	// Moving the vector into the map keeps its data where this source points.
	AkExternalSourceInfo source( wem.data(), static_cast<AkUInt32>( wem.size() ), sourceCookie, AKCODECID_PCM );
	const AkPlayingID playingID = AK::SoundEngine::PostEvent( eventID, gameObjectID, flags, callback, this, 1, &source );
	if( playingID != AK_INVALID_PLAYING_ID )
	{
		m_buffers.emplace( playingID, std::move( wem ) );
	}
	return playingID;
}

void AudPcmSourceBuffers::Release( AkPlayingID playingID )
{
	std::vector<uint8_t> released;
	{
		CcpAutoMutex lock( m_mutex );
		auto it = m_buffers.find( playingID );
		if( it == m_buffers.end() )
		{
			return;
		}
		released = std::move( it->second );
		m_buffers.erase( it );
	}
	// released is freed here, outside the lock, since a buffer can be megabytes and this runs on Wwise's thread.
}

size_t AudPcmSourceBuffers::GetHeldCount()
{
	CcpAutoMutex lock( m_mutex );
	return m_buffers.size();
}
