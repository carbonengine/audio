//////////////////////////////////////////////////////////////////////
//
// CCPFXSrcAudioStream.cpp
//
// Sample capture of audio stream to be used as a source.
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#include "CCPFXSrcAudioStream.h"
#include <assert.h>
#include <math.h>

#include "stdio.h"
#include "CCPMemory.h"

// Holds audio input information
struct AudioStreamStream
{
	AkReal32 *m_pData;
	AkUInt32  m_uDataSize;
	AkUInt32  m_curWritePos;
	AudioStreamStream() : m_pData(NULL), m_uDataSize(0){}
};

// This is the global list of audio inputs that will be used by the 
// plug-in
AudioStreamStream g_InputStreams[INPUT_MAX_INPUTS];

HANDLE g_Mutex;
AkUInt32 maxDataSize = 81920; // In samples, not frames
AkUInt32 preloadSize = 32768; // In samples, not frames
AkUInt32 maxDataInFrames = maxDataSize / 2;

// Useful definitions
static const AkReal32 RAMPMAXTIME	= 0.1f;		// 100 ms ramps, worst-case


// Plugin mechanism. FX create function and register its address to the FX manager.
AK::IAkPlugin* CreateAudioStreamSource( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CCCPFXSrcAudioStream() );
}

// Constructor.
CCCPFXSrcAudioStream::CCCPFXSrcAudioStream()
{
	// Initialize members
	m_uSampleRate      = 0;
	m_uNumChannels     = 1;
	m_pSharedParams    = NULL;
	m_pSourceFXContext = NULL;

	// Input stream parameters
	m_uAudioInput	   = 0;
	m_uAudioPosition   = 0;
}

// Destructor.
CCCPFXSrcAudioStream::~CCCPFXSrcAudioStream()
{

}

// Initialization.
AKRESULT CCCPFXSrcAudioStream::Init(	AK::IAkPluginMemAlloc *			in_pAllocator,    		// Memory allocator interface.
									AK::IAkSourcePluginContext *	in_pSourceFXContext,	// Source FX context
									AK::IAkPluginParam *			in_pParams,				// Effect parameters.
									AkAudioFormat &					io_rFormat				// Output audio format.
                           )
{
	m_preloaded = false;

	// Keep source FX context (looping info etc.)
	m_pSourceFXContext = in_pSourceFXContext;

	// Output format set to stereo to support ccp bink video format
	io_rFormat.uChannelMask = AK_SPEAKER_SETUP_STEREO;
	m_uNumChannels = io_rFormat.GetNumChannels();

    // Set parameters access
    m_pSharedParams = reinterpret_cast<CCCPFxSrcAudioStreamParams*>(in_pParams);

	// Save audio format internally
	io_rFormat.uSampleRate = 44100;
	m_uSampleRate = io_rFormat.uSampleRate;
	
	// Reset the position to 0
	m_uAudioPosition = 0;

	// Gain ramp initialization
	AkReal32 fGainIncrement = 1.f/(RAMPMAXTIME*m_uSampleRate);
	m_GainRamp.RampSetup( fGainIncrement, m_pSharedParams->GetGain() );

	// Point to the correct input stream
	m_uAudioInput = m_pSharedParams->GetInput();

	g_InputStreams[m_uAudioInput].m_uDataSize = 0;

	if (g_InputStreams[m_uAudioInput].m_pData == NULL)
	{
		g_InputStreams[m_uAudioInput].m_pData = (AkReal32*) CCP_MALLOC("AudioStream input stream", sizeof(AkReal32) * maxDataSize);
	}


    return AK_Success;
}

// Term: The effect must destroy itself herein
AKRESULT CCCPFXSrcAudioStream::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}


// Reset. Reinitialize processing state.
AKRESULT CCCPFXSrcAudioStream::Reset( )
{
	m_uAudioPosition = 0;

	return AK_Success;
}

// Effect info query.
AKRESULT CCCPFXSrcAudioStream::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeSource;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
    return AK_Success;
}

//Effect processing.
void CCCPFXSrcAudioStream::Execute(	AkAudioBuffer *							io_pBufferOut		// Output buffer interface.
#ifdef AK_PS3									
									, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	// the process that needs to run
#endif
									)
{
	AkReal32 *pData     = g_InputStreams[m_uAudioInput].m_pData;
	AkUInt32  uDataSize = g_InputStreams[m_uAudioInput].m_uDataSize;

	m_uAudioPosition = 0;

	// Change target gain if necessary (RTPC value)
	m_GainRamp.SetTarget( m_pSharedParams->GetGain() );
	
	AkUInt32 uMaxFrames = io_pBufferOut->MaxFrames();

	io_pBufferOut->eState = AK_DataReady;

	AkSampleType * AK_RESTRICT pBufOutL = io_pBufferOut->GetChannel(0);
	AkSampleType * AK_RESTRICT pBufOutR = io_pBufferOut->GetChannel(1);

	if ( m_preloaded == false && uDataSize >= preloadSize )
	{
		m_preloaded = true;
	}

	if ( uDataSize > 0 && m_preloaded == true )
	{	
		if ( (uDataSize / 2) < uMaxFrames )
			uMaxFrames = uDataSize / 2;

		io_pBufferOut->uValidFrames = (AkUInt16) uMaxFrames;

		while ( uMaxFrames-- )
		{
			// Tick gain interpolation ramp
			AkReal32 fCurGain = m_GainRamp.Tick();

			// Copy over what is in the set data buffer
			*pBufOutL++ = pData[m_uAudioPosition++] * fCurGain;
			*pBufOutR++ = pData[m_uAudioPosition++] * fCurGain;

			// Start from the beginning when we hit the end of the buffer
			if(m_uAudioPosition >= maxDataSize)
			{
				m_uAudioPosition = 0;
			}
		}

		AkUInt32 validSamples = io_pBufferOut->uValidFrames * 2;
		if ( validSamples < uDataSize )
		{	// TODO: This will be much faster as a circular buffer
			g_InputStreams[m_uAudioInput].m_uDataSize = uDataSize - validSamples;
			memmove(pData, pData + validSamples, g_InputStreams[m_uAudioInput].m_uDataSize * sizeof(AkReal32) );
		}
		else
		{
			g_InputStreams[m_uAudioInput].m_uDataSize = 0;
		}
	}
	else 
	{
		io_pBufferOut->uValidFrames = (AkUInt16) uMaxFrames;
		// No data set; just fill the output with silence
		while ( uMaxFrames-- )
		{
			*pBufOutL++ = 0.0f;
			*pBufOutR++ = 0.0f;
		}
		m_preloaded = false;
	}
}

AKRESULT CCCPFXSrcAudioStream::StopLooping()
{
	// Since this plug-in is infinite playing, a stoplooping should result in stoping the sound.
	// returning AK_Fail will do it.
	return AK_Fail;
}

//--------------------------------------------------------------------------------------------------------
// Set stream data. This is declared public in AkAudioStreamSourceFactory (since we do not declare the 
// CakFXSrcAudioStream class to the application)
//--------------------------------------------------------------------------------------------------------
AkUInt32 __stdcall SetAudioStreamData(AkReal32 *const data , AkUInt32 const dataSize, int const input )
{

	AkUInt32 result = 0;

	if ((input >= 0) && (input < INPUT_MAX_INPUTS))
	{
		if (g_InputStreams[input].m_pData == NULL)
			return 0;
		if ( data == NULL && dataSize == 0 )
			return maxDataSize - g_InputStreams[input].m_uDataSize;
		if (g_InputStreams[input].m_uDataSize + dataSize <= maxDataSize)
		{	// TODO: This will be much faster as a circular buffer
			memmove(g_InputStreams[input].m_pData + g_InputStreams[input].m_uDataSize, data, dataSize*sizeof(AkReal32));
			g_InputStreams[input].m_uDataSize += dataSize;
			
			result = maxDataSize - g_InputStreams[input].m_uDataSize;
		}
	}

	return result;
}

//--------------------------------------------------------------------------------------------------------
// Get maximum inputs This is declared public in AkAudioStreamSourceFactory (since we do not declare the 
// CakFXSrcAudioInput class to the application)
// Note that INPUT_MAX_INPUTS can be changed to fit the needs of the application
//--------------------------------------------------------------------------------------------------------
AkUInt32 GetMaxAudioInputs()
{
	return INPUT_MAX_INPUTS;
}
