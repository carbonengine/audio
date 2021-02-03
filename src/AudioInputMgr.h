////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Eric Nielsen
// Created:		August 2020
// Copyright:	CCP 2020
// Description: A class to interface with Wwises Audio Input plugin: https://www.audiokinetic.com/library/edge/?source=SDK&id=referencematerial_audioinput.html.
//				This class takes care of registering, catching and forwarding callbacks from Wwise for the purpose of playing external 
//				audio streams inside the game. A class that wants to use this must inherit from IAudioInputSink. Only 16 bit interleaved
//				buffers are supported.
//
//			    In particular this is used for our in game video player to stream audio directly from in game cinematics.

#pragma once
#ifndef AudioInputMgr_h_
#define AudioInputMgr_h_

#include "IAudioInputMgr.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>

BLUE_CLASS( AudioInputMgr ) :
	public IAudioInputMgr
{
public:
	AudioInputMgr();
	~AudioInputMgr();

	EXPOSE_TO_BLUE();

	// The event in Wwise that must be triggered in order to start the Audio Input Plugin.
	const std::string INPUT_PLUGIN_EVENT = "in_game_video_stream_play";
	// The name of the RTPC in the Wwise project that controls video volumes.
	const std::string VOLUME_RTPC = "volume_video";
	
	// IAudioInputMgr //
	void StartInput( uint32_t channels, uint32_t bps, uint32_t rate ) override;
	void StopInput() override;
	void SetVolume( float volume ) override;

	// Wwise callbacks //
	static void Execute( AkPlayingID in_playingID, AkAudioBuffer * io_pBufferOut );
	static void GetFormatCallback( AkPlayingID in_playingID, AkAudioFormat & io_AudioFormat );

	// Set the class that will take care of filling Wwises audio buffer.
	void SetSink( IAudioInputSink * inputSink ) override;

protected:
	uint32_t m_channels;
	uint32_t m_bitsPerSamples;
	uint32_t m_sampleRate;
	AkPlayingID m_playingID;

private:
	IAudioInputSink* m_inputSink;
};

TYPEDEF_BLUECLASS( AudioInputMgr );

#endif
