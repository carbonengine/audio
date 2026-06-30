// Copyright © 2020 CCP ehf.

#include "stdafx.h"

#include <AK/Plugin/AkAudioInputPlugin.h>

#include "AudioInputMgr.h"
#include "Audio2.h"

std::map<int, AudioInputMgr*> g_audioInputMgrMap;
CcpMutex g_inputMgrMapMutex( "AudioInputMgr", "g_inputMgrMapMutex" );

AudioInputMgr::AudioInputMgr() :
	m_channels( 2 ),
	m_bitsPerSamples( 16 ),
	m_sampleRate( 48000 ),
	m_playingID( 0 ),
	m_inputSink( nullptr )
{
}

AudioInputMgr::~AudioInputMgr()
{
	m_inputSink = nullptr;

	if( m_playingID > 0 )
	{
		CcpAutoMutex lock( g_inputMgrMapMutex );
		g_audioInputMgrMap.erase( m_playingID );
	}
}

// Start the Wwise Audio Input plugin and set callbacks Wwise will call.
void AudioInputMgr::StartInput( uint32_t channels, uint32_t bps, uint32_t rate )
{
	if( !g_audioInitialized )
	{
		return;
	}

	m_channels = channels;
	m_bitsPerSamples = bps;
	m_sampleRate = rate;

	// An event using the source plugin Audio Input must exist in the a loaded Wwise soundbank and
	// be triggered to start the Audio Input plugin.
	m_playingID = AK::SoundEngine::PostEvent( INPUT_PLUGIN_EVENT.c_str(), UI_GAME_OBJ_ID );
	if( m_playingID == AK_INVALID_PLAYING_ID )
	{
		CCP_LOGERR( "Failed to post %S to audio emitter %d. Video playback will fail.", INPUT_PLUGIN_EVENT.c_str(), UI_GAME_OBJ_ID );
		return;
	}

	CcpAutoMutex lock( g_inputMgrMapMutex );
	g_audioInputMgrMap.insert( std::make_pair( m_playingID, this ) );
	SetAudioInputCallbacks( Execute, GetFormatCallback );
}

// Stop the audio input plugin which will stop Wwise callbacks.
void AudioInputMgr::StopInput()
{
	if( !g_audioInitialized )
	{
		return;
	}
	if ( m_playingID > 0 )
	{
		// Stopping the event called in StartInput will kill the Audio Input plugin in Wwise.
		AK::SoundEngine::StopPlayingID( m_playingID );

		CcpAutoMutex lock( g_inputMgrMapMutex );
		g_audioInputMgrMap.erase( m_playingID );

		m_playingID = 0;
	}
}

void AudioInputMgr::SetVolume( float volume )
{
	if( !g_audioInitialized || m_playingID == 0 )
	{
		return;
	}

	// Volume must be between 0 and 1
	if( volume < 0 )
	{
		volume = 0;
	}
	else if( volume > 1 )
	{
		volume = 1;
	}

	AK::SoundEngine::SetRTPCValueByPlayingID( VOLUME_RTPC.c_str(), volume, m_playingID );
}

// The callback Wwise will call every audio frame. Passes a pointer to Wwises buffer
// to the videoplayer which should have registered its own callback.
void AudioInputMgr::Execute( AkPlayingID in_playingID, AkAudioBuffer* io_pBufferOut )
{
	CcpAutoMutex lock( g_inputMgrMapMutex );
	auto it = g_audioInputMgrMap.find( in_playingID );
	if( it == g_audioInputMgrMap.end() )
	{
		// Signal to Wwise to kill the Audio Input plugin.
		io_pBufferOut->eState = AK_NoMoreData;
		return;
	}

	AudioInputMgr* inputMgr = it->second;

	if( inputMgr->m_inputSink )
	{
		BufferData bufferData;
		bufferData.numChannels = io_pBufferOut->NumChannels();
		bufferData.numSamples = io_pBufferOut->MaxFrames() * io_pBufferOut->NumChannels(); // Number of samples Wwise can accept
		bufferData.data = reinterpret_cast<int16_t*>( io_pBufferOut->GetInterleavedData() );
		io_pBufferOut->uValidFrames = inputMgr->m_inputSink->FillBuffer( bufferData );
		if( io_pBufferOut->uValidFrames > 0 )
		{
			io_pBufferOut->eState = AK_DataReady;
		}
	}
}

// The callback Wwise will call when it wants to know the format of the audio that will be
// passed to it.
void AudioInputMgr::GetFormatCallback( AkPlayingID in_playingID, AkAudioFormat& io_AudioFormat )
{
	CcpAutoMutex lock( g_inputMgrMapMutex );
	auto it = g_audioInputMgrMap.find( in_playingID );
	if( it == g_audioInputMgrMap.end() )
	{
		return;
	}

	AudioInputMgr* inputMgr = it->second;
	AkUInt32 channelBitmask = ( inputMgr->m_channels == 2 ? AK_SPEAKER_SETUP_STEREO : AK_SPEAKER_SETUP_MONO );
	AkUInt32 bytesPerSample = ( inputMgr->m_bitsPerSamples / 8 ) * inputMgr->m_channels; // Bytes per sample * number of channels (8 bits in a byte).

	io_AudioFormat.SetAll(
		inputMgr->m_sampleRate,
		AkChannelConfig( inputMgr->m_channels, channelBitmask ),
		inputMgr->m_bitsPerSamples,
		bytesPerSample,
		AK_INT, // feeding integers(signed)
		AK_INTERLEAVED );
}

void AudioInputMgr::SetSink( IAudioInputSink* inputSink )
{
	m_inputSink = inputSink;
}
