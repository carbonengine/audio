//////////////////////////////////////////////////////////////////////
//
// CCPFXSrcAudioStream.h
//
// Allow the game to set data for a source.
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#ifndef _CCPFXSRC_AUDIOSTREAM_H_
#define _CCPFXSRC_AUDIOSTREAM_H_

#include "CCPFXSrcAudioStreamParams.h"
#include <AK/Plugin/PluginServices/AkValueRamp.h>

//-----------------------------------------------------------------------------
// Name: class CAkFXSrcAudioInput
// Desc: Demo implementation of audio input
//-----------------------------------------------------------------------------
class CCCPFXSrcAudioStream : public AK::IAkSourcePlugin
{
public:

    // Constructor.
    CCCPFXSrcAudioStream();

	// Destructor.
    ~CCCPFXSrcAudioStream();

    // Initialize.
    AKRESULT Init(	AK::IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
					AK::IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
                    AK::IAkPluginParam *			in_pParams,			// Effect parameters.
                    AkAudioFormat &					io_rFormat			// Supported audio output format.
                    );

    // Effect termination.
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset.
	AKRESULT Reset( );

    // Effect info query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
	void Execute(	AkAudioBuffer *							io_pBuffer
#ifdef AK_PS3	
					, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif					
					);

	AkReal32 GetDuration( ) const {return 0;}

	virtual AKRESULT StopLooping();

private:

    // Internal state variables.
    AkUInt32			m_uSampleRate;			// Sample frequency
	AkUInt32			m_uNumChannels;			// Number of output channels

    // Shared parameters structure
    CCCPFxSrcAudioStreamParams * m_pSharedParams;

	// Source FX context interface
	AK::IAkSourcePluginContext * m_pSourceFXContext;

	// Input to get data from
	AkUInt32			m_uAudioInput;

	// Current location from the input
	AkUInt32			m_uAudioPosition;

	// Has the stream been preloaded?
	bool				m_preloaded;

	// Gain ramp interpolator
	AK::CAkValueRamp	m_GainRamp;
};

#endif  _CCPFXSRC_AUDIOSTREAM_H_
