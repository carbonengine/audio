////////////////////////////////////////////////////////////
//
//    Creator:   Eric Nielsen
//    Created:   August 2020
//    Copyright: CCP 2020

#pragma once

BLUE_DECLARE_INTERFACE( IAudioInputSink );

// An interface for modules outside of audio2 to interact with.
BLUE_INTERFACE( IAudioInputMgr ) :
	public IRoot
{
	struct BufferData
	{
		int numSamples; // Number of sample frames Wwises buffer can hold (use this to know how much data to provide).
		int numChannels; // Number of channels according to Wwise.
		int16_t* data; // The interleaved buffer to fill from Wwise.
	};
	virtual void SetSink( IAudioInputSink* inputSink) = 0;

	virtual void StartInput( uint32_t channels, uint32_t bps, uint32_t rate) = 0;
	virtual void StopInput() = 0;
	virtual void SetVolume( float volume ) = 0;
};

// The interface your class must inherit from to provide an audio stream to Wwise.
BLUE_INTERFACE( IAudioInputSink ): IRoot
{
	// A callback that will be called on every audio frame allowing you to fill Wwises audio buffer.
	// You must register your class using SetSink from IAudioInputMgr as well as calling
	// StartInput for this to begin being called.
	virtual int FillBuffer( IAudioInputMgr::BufferData& ) = 0;
};
