//////////////////////////////////////////////////////////////////////
//
// AudioStream.cpp
//
// AudioStream Wwise plugin: Defines the initialization routines for the DLL.
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AudioStream.h"
#include <AK/Wwise/Utilities.h>
#include "..\include\CCPAudioStreamSourceFactory.h"
#include "AudioStreamPlugin.h"

void* CCPMalloc( size_t size )
{
	return malloc( size );
}

void CCPFree( void* ptr )
{
	free( ptr );
}

void* CCPMallocWithTracking( size_t size, const char* name, const char* file, int line )
{
	return malloc( size );
}

void  CCPFreeWithTracking( void* ptr )
{
	free( ptr );
}

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

#include <math.h>

// CAudioStreamApp
BEGIN_MESSAGE_MAP(CAudioStreamApp, CWinApp)
END_MESSAGE_MAP()

static const AkReal32 TWOPI			= 6.283185307179586476925286766559f;
static const AkReal32 FREQUENCY		= 55.0f;	// Frequency (A)
static const AkReal32 SAMPLERATE	= 48000.0f;	// NOTE: This might need to be located somewhere..

// CAudioInputApp construction
CAudioStreamApp::CAudioStreamApp()
{
}


// The one and only CAudioInputApp object
CAudioStreamApp theApp;


// CAudioInputApp initialization
BOOL CAudioStreamApp::InitInstance()
{
	AK::Wwise::RegisterWwisePlugin();
	CWinApp::InitInstance();

	// Init the online help
//	EnableHtmlHelp() ; // TODO: Point this to core wiki page on audio stream

	TCHAR szModule[ MAX_PATH ];
	::GetModuleFileName( AfxGetInstanceHandle(), szModule, MAX_PATH ) ;

	PathRemoveFileSpec( szModule );
	PathAppend( szModule, _T("AudioInput.chm") ); // Point to audio input help for now

	m_pszHelpFilePath = _tcsdup( szModule ) ;

	// In-game, the audio data comes from the game. Since this DLL is used
	// in Wwise, let's simulate the audio data with a different test tone
	// for each input.
	m_fTestBuffers = (float **)malloc(GetMaxAudioInputs() * sizeof(float*));

	for (AkUInt32 i = 0; i < GetMaxAudioInputs(); i++)
	{
		AkUInt32 numsamples = (AkUInt32)(SAMPLERATE / (FREQUENCY * (i + 1)));
		m_fTestBuffers[i] = (float*)malloc(numsamples * sizeof(float));

		AkReal32 phaseincrement = TWOPI / numsamples; 
		AkReal32 phase = 0.0f;	

		for (AkUInt32 j = 0; j < numsamples; j++)
		{	
			m_fTestBuffers[i][j] = sinf(phase);
			phase += phaseincrement;
		}

		SetAudioStreamData(m_fTestBuffers[i], numsamples, i);
	}

	return TRUE;
}

// CAudioInputApp exit code
int CAudioStreamApp::ExitInstance()
{
	// Free the test tone buffers
	for (AkUInt32 i = 0; i < GetMaxAudioInputs(); i++)
	{
		SetAudioStreamData(NULL, 0, i);
		free (m_fTestBuffers[i]);
	}

	free (m_fTestBuffers);

	return 0;
}

/////////////// DLL exports ///////////////////

// Plugin creation
AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
	if ( in_usCompanyID == AudioStreamPlugin::CompanyID && in_usPluginID == AudioStreamPlugin::PluginID )
		return new AudioStreamPlugin;

	return NULL;
}

// Sound Engine callbacks
bool __stdcall AkGetSoundEngineCallbacks( unsigned short in_usCompanyID, unsigned short in_usPluginID, AkCreatePluginCallback & out_funcEffect, AkCreateParamCallback & out_funcParam )
{
	if ( in_usCompanyID == AudioStreamPlugin::CompanyID && in_usPluginID == AudioStreamPlugin::PluginID )
	{
		out_funcEffect = CreateAudioStreamSource;
		out_funcParam = CreateAudioStreamSourceParams;
		return true;
	}

	return false;
}

/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;
DEFINE_PLUGIN_REGISTER_HOOK;