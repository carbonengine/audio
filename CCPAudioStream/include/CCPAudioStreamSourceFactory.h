//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////
// CCPAudioStreamSourceFactory.h

/// \file 
///! Plug-in unique ID and creation functions (hooks) necessary to register the audio stream plug-in to the sound engine.
/// <br><b>Wwise source name:</b>  AudioStream
/// <br><b>Library file:</b> AkAudioStreamSource.lib

#ifndef _CCP_AUDIOSTREAMSOURCEFACTORY_H_
#define _CCP_AUDIOSTREAMSOURCEFACTORY_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

///
/// - This is the plug-in's unique ID (combined with the AKCOMPANYID_AUDIOKINETIC company ID)
/// - This ID must be the same as the plug-in ID in the plug-in's XML definition file, and is persisted in project files. 
/// \akwarning
/// Changing this ID will cause existing projects not to recognize the plug-in anymore.
/// \endakwarning
const AkUInt32 CCPSOURCEID_AUDIOSTREAM = 223;

/// Static creation function that returns an instance of the sound engine plug-in parameter node to be hooked by the sound engine plug-in manager.
AK::IAkPluginParam * CreateAudioStreamSourceParams(
	AK::IAkPluginMemAlloc * in_pAllocator			///< Memory allocator interface
	);

/// Plugin mechanism. Source create function and register its address to the plug-in manager.
AK::IAkPlugin* CreateAudioStreamSource(
	AK::IAkPluginMemAlloc * in_pAllocator			///< Memory allocator interface
	);

//////////////////////////////////////////////////////////////////////////
///
/// NOTE: The following functions interface the plug-ins global data to 
/// the application. Use these to set buffer data that will be read by
/// active source voices
///
//////////////////////////////////////////////////////////////////////////
extern "C"
{
typedef AkUInt32 (__stdcall * SETAUDIOSTREAMCALL) (AkReal32 *const data, AkUInt32 const dataSize, int const input);

/// Update the buffer data corresponding to an input channel of the Audio Stream plug-in. This function should be called 
/// before any voices using the Audio Stream plug-in are started. The format of the data is a stereo (two-channel) 32-bit floating 
/// point buffer. The sample rate to use is 44.1k
AkUInt32 __stdcall SetAudioStreamData(
	AkReal32 *const data,							///< Pointer to data
	AkUInt32 const dataSize,						///< Size of data
	int const input									///< Input to set the data to 
	);
AkUInt32 __stdcall GetAudioStreamPosition(
	int const input									///< Input to set the data to 
	);
}

/// Get the number of available input channels. The current set size for the maximum number of inputs is 16. 
/// Refer to INPUT_MAX_INPUTS for more information. To change this value, set INPUT_MAX_INPUTS to the desired size. 
/// Also, you must change the field 'AudioStreamInput' MIN and MAX size in the AudioStream.xml file to reflect this change
AkUInt32 GetMaxAudioInputs();

/*
Use the following code to register your plug-in

AK::SoundEngine::RegisterPlugin( AkPluginTypeSource, 
								 AKCOMPANYID_AUDIOKINETIC, 
								 CCPSOURCEID_AUDIOSTREAM,
								 CreateAudioStreamSource,
								 CreateAudioStreamSourceParam );
*/

#endif // _CCP_AUDIOSTREAMSOURCEFACTORY_H_
