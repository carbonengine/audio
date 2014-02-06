//////////////////////////////////////////////////////////////////////
//
// CCPFXSrcAudioStreamParams.cpp
//
// Allows for audio source to come from an external input.
// 
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#include "CCPFXSrcAudioStreamParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

#include "stdio.h"

// Plugin mechanism. Parameter node create function to be registered to the FX manager.
AK::IAkPluginParam * CreateAudioStreamSourceParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CCCPFxSrcAudioStreamParams() );
}

// Constructor.
CCCPFxSrcAudioStreamParams::CCCPFxSrcAudioStreamParams()
{

}

// Destructor.
CCCPFxSrcAudioStreamParams::~CCCPFxSrcAudioStreamParams()
{

}

// Copy constructor.
CCCPFxSrcAudioStreamParams::CCCPFxSrcAudioStreamParams( const CCCPFxSrcAudioStreamParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;	 
}

// Create parameters node duplicate.
AK::IAkPluginParam * CCCPFxSrcAudioStreamParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	return AK_PLUGIN_NEW( in_pAllocator, CCCPFxSrcAudioStreamParams(*this) );
}

// Shared parameters initialization.
AKRESULT CCCPFxSrcAudioStreamParams::Init( AK::IAkPluginMemAlloc *	in_pAllocator,								   
										 const void *				in_pParamsBlock, 
										 AkUInt32					in_ulBlockSize 
                                 )
{
    if ( in_ulBlockSize == 0)
    {
		// Init with default values if we got invalid parameter block.
        m_Params.iInput = 0;     // Input Channel
        m_Params.fGain  = -12.f; // Gain (in dB FS)
        return AK_Success;
    }

    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Parameter node termination.
AKRESULT CCCPFxSrcAudioStreamParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CCCPFxSrcAudioStreamParams::SetParamsBlock( const void * in_pParamsBlock, 
												   AkUInt32 in_ulBlockSize
                                           )
{
	AKRESULT eResult = AK_Success;
	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	//memcpy( &m_Params, in_pParamsBlock, sizeof( CCPFxSrcAudioStreamParams ) );
	m_Params.iInput = READBANKDATA( AkUInt32, pParamsBlock, in_ulBlockSize );
	m_Params.fGain = READBANKDATA( AkReal32, pParamsBlock, in_ulBlockSize );
	CHECKBANKDATASIZE( in_ulBlockSize, eResult );
    return eResult;
}

// Update single parameter.
AKRESULT CCCPFxSrcAudioStreamParams::SetParam( AkPluginParamID	in_ParamID,
											 const void *		in_pValue, 
											 AkUInt32			in_ulParamSize
                                     )
{
	// Consistency check.
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

    // Set parameter value.
    switch ( in_ParamID )
    {
	case CCP_SRCAUDIOSTREAM_FXPARAM_INPUT_ID:
		m_Params.iInput = *reinterpret_cast<const AkUInt32*>(in_pValue);
		break;
	case CCP_SRCAUDIOSTREAM_FXPARAM_GAIN_ID:
		m_Params.fGain = *reinterpret_cast<const AkReal32*>(in_pValue);
		break;
	default:
		return AK_InvalidParameter;
	}

    return AK_Success;
}