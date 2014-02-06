//////////////////////////////////////////////////////////////////////
//
// CCPFXSrcAudioStreamParams.h
//
// Allows for audio source to come from an external input.
// 
// Note: Target output format is currently determined by the source itself.
// 
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#ifndef _CCPFXSRC_AUDIOSTREAMPARAMS_H_
#define _CCPFXSRC_AUDIOSTREAMPARAMS_H_

#include "..\include\CCPAudioStreamSourceFactory.h"
#include <assert.h>
#include <math.h>

// Parameters IDs.
const AkPluginParamID CCP_SRCAUDIOSTREAM_FXPARAM_INPUT_ID		= 0;
const AkPluginParamID CCP_SRCAUDIOSTREAM_FXPARAM_GAIN_ID		= 1;

// Parameter range values
const AkUInt32 INPUT_MAX_INPUTS			= 16;		// NOTE: Change this for more voices
const AkReal32 INPUT_GAIN_MIN			= -96.3f;	// dB FS
const AkReal32 INPUT_GAIN_MAX			= 0.f;		// db FS

// Parameters struture for this effect.
struct CCPFxSrcAudioStreamParams
{
	AkUInt32	 iInput;		// Input channel
	AkReal32     fGain;         // Gain (in dBFS).
};


//-----------------------------------------------------------------------------
// Name: class CAkFXSrcAudioInputParams
// Desc: Sample implementation the audio input source shared parameters.
//-----------------------------------------------------------------------------
class CCCPFxSrcAudioStreamParams : public AK::IAkPluginParam
{
public:

	// Allow effect to call accessor functions for retrieving parameter values.
	friend class CCCPFXSrcAudioStream;

    // Constructor.
    CCCPFxSrcAudioStreamParams();
	
	// Copy constructor.
    CCCPFxSrcAudioStreamParams( const CCCPFxSrcAudioStreamParams & in_rCopy );

	// Destructor.
    ~CCCPFxSrcAudioStreamParams();

    // Create parameter object duplicate.
	AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Initialization.
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator.						    
                   const void *				in_pParamsBlock,	// Pointer to param block.
                   AkUInt32					in_ulBlockSize		// Sise of the parm block.
                   );
   
	// Termination.
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    AKRESULT SetParamsBlock( const void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    AKRESULT SetParam( AkPluginParamID in_ParamID,
                       const void * in_pValue, 
                       AkUInt32 in_ulParamSize
                       );

private:

    AkUInt32	GetInput();
    AkReal32	GetGain();

private:

    // Parameter structure.
    CCPFxSrcAudioStreamParams	m_Params;
};

// Safely retrieve input.
inline AkUInt32 CCCPFxSrcAudioStreamParams::GetInput( )
{
    AkUInt32 iInput = m_Params.iInput;
	AKASSERT( iInput >= 0 && iInput < INPUT_MAX_INPUTS);
    return iInput;
}

// Safely retrieve gain.
inline AkReal32 CCCPFxSrcAudioStreamParams::GetGain( )
{
    AkReal32 fGain = m_Params.fGain;
	AKASSERT( fGain >= INPUT_GAIN_MIN && fGain <= INPUT_GAIN_MAX );
	fGain = powf( 10.f, ( fGain / 20.f ) ); // Make it a linear value	
    return fGain;
}

#endif // _CCPFXSRC_AUDIOSTREAMPARAMS_H_