////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Eric Nielsen
// Created:		August 2020
// Copyright:	CCP 2020

#include "stdafx.h"
#include "AudioInputMgr.h"
#include "Audio2.h"

#include <AK/Plugin/AkAudioInputPlugin.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

class AudioInputMgr* g_audioInputMgr = nullptr;

AudioInputMgr::AudioInputMgr() :
	m_channels( 2 ),
	m_bitsPerSamples( 16 ),
	m_sampleRate( 48000 ),
	m_playingID( 0 ),
	m_inputSink( nullptr )
{
	g_audioInputMgr = this;
}

AudioInputMgr::~AudioInputMgr()
{
	m_inputSink = nullptr;
	g_audioInputMgr = nullptr;
}

// Start the Wwise Audio Input plugin and set callbacks Wwise will call.
void AudioInputMgr::StartInput( uint32_t channels, uint32_t bps, uint32_t rate )
{
	m_channels = channels;
	m_bitsPerSamples = bps;
	m_sampleRate = rate;

	// An event using the source plugin Audio Input must exist in the a loaded Wwise soundbank and
	// be triggered to start the Audio Input plugin.
	AK::SoundEngine::PostEvent( INPUT_PLUGIN_EVENT.c_str(), UI_GAME_OBJ_ID );

	SetAudioInputCallbacks( Execute, GetFormatCallback );
}

// Stop the audio input plugin which will stop Wwise callbacks.
void AudioInputMgr::StopInput()
{
	// Stopping the event called in StartInput will kill the Audio Input plugin in Wwise.
	AK::SoundEngine::StopPlayingID( m_playingID );
}

// The callback Wwise will call every audio frame. Passes a pointer to Wwises buffer
// to the videoplayer which should have registered its own callback.
void AudioInputMgr::Execute( AkPlayingID in_playingID, AkAudioBuffer* io_pBufferOut )
{
	if( g_audioInputMgr == nullptr )
	{
		// Signal to Wwise to kill the Audio Input plugin.
		io_pBufferOut->eState = AK_NoMoreData;
		return;
	}

	if( !g_audioInputMgr->m_playingID )
	{
		g_audioInputMgr->m_playingID = in_playingID;
	}

	if( g_audioInputMgr->m_inputSink )
	{
		BufferData bufferData;
		bufferData.numChannels = io_pBufferOut->NumChannels();
		bufferData.numSamples = io_pBufferOut->MaxFrames() * io_pBufferOut->NumChannels(); // Number of samples Wwise can accept
		bufferData.data = reinterpret_cast<int16_t*>( io_pBufferOut->GetInterleavedData() );
		io_pBufferOut->uValidFrames = g_audioInputMgr->m_inputSink->FillBuffer( bufferData );
		if( io_pBufferOut->uValidFrames > 0 )
		{
			io_pBufferOut->eState = AK_DataReady;
		}
		else
		{
			// Signals to Wwise to pause playback.
			io_pBufferOut->eState = AK_NoDataReady;
		}
	}
}

// The callback Wwise will call when it wants to know the format of the audio that will be 
// passed to it.
void AudioInputMgr::GetFormatCallback( AkPlayingID in_playingID, AkAudioFormat& io_AudioFormat )
{
	AkUInt32 channelBitmask = ( g_audioInputMgr->m_channels == 2 ? AK_SPEAKER_SETUP_STEREO : AK_SPEAKER_SETUP_MONO );
	AkUInt32 bytesPerSample = ( g_audioInputMgr->m_bitsPerSamples / 8 ) * g_audioInputMgr->m_channels; // Bytes per sample * number of channels (8 bits in a byte).

	io_AudioFormat.SetAll(
		g_audioInputMgr->m_sampleRate, 
		AkChannelConfig( g_audioInputMgr->m_channels, channelBitmask ),
		g_audioInputMgr->m_bitsPerSamples, 
		bytesPerSample, 
		AK_INT, // feeding integers(signed)
		AK_INTERLEAVED 
	);
}

void AudioInputMgr::SetSink( IAudioInputSink* inputSink )
{
	m_inputSink = inputSink;
}
