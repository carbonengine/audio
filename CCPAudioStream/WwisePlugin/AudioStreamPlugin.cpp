//////////////////////////////////////////////////////////////////////
//
// AudioStreamPlugin.cpp
//
// Audio Stream Wwise plugin implementation.
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "AudioStreamPlugin.h"
#include "Help\TopicAlias.h"
#include "..\include\CCPAudioStreamSourceFactory.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace AK;
using namespace Wwise;

const char* g_moduleName = "audiostreamplugin";

void AudioStreamPlugin::GetFormatCallbackFunc(
    AkPlayingID		in_playingID,   ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
    AkAudioFormat&  io_AudioFormat  ///< Already filled format, modify it if required.
    )
{
}

void AudioStreamPlugin::ExecuteCallbackFunc(
    AkPlayingID		in_playingID,  ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
    AkAudioBuffer*	io_pBufferOut  ///< Buffer to fill
    )
{
	AkSampleType * pSamples = io_pBufferOut->GetChannel( 0 );

	for ( int i = 0; i < io_pBufferOut->MaxFrames(); ++i )
		pSamples[ i ] = (float) sin( (double) i / 256.0 * M_PI * 2.0 );

	io_pBufferOut->eState = AK_DataReady;
	io_pBufferOut->uValidFrames = io_pBufferOut->MaxFrames();
}

// These IDs must be the same as those specified in the plug-in's XML definition file.
// Note that there are restrictions on the values you can use for CompanyID, and PluginID
// must be unique for the specified CompanyID. Furthermore, these IDs are persisted
// in project files. NEVER CHANGE THEM or existing projects will not recognize this Plug-in.
// Be sure to read the SDK documentation regarding Plug-ins XML definition files.
const short AudioStreamPlugin::CompanyID = AKCOMPANYID_AUDIOKINETIC; // TODO: Update this to CCPs company ID
const short AudioStreamPlugin::PluginID = CCPSOURCEID_AUDIOSTREAM;

// Constructor
AudioStreamPlugin::AudioStreamPlugin()
	: m_pPSet( NULL )
{
}

// Destructor
AudioStreamPlugin::~AudioStreamPlugin()
{
}

// Implement the destruction of the Wwise source plugin.
void AudioStreamPlugin::Destroy()
{
	delete this;
}

// Wwise application must be able to know if the plugin is ready to be played or not
bool AudioStreamPlugin::IsPlayable() const
{
	return true; // Silence is always playable.
}

bool AudioStreamPlugin::Load( AK::IXmlTextReader* in_pReader )
{
	return false; //Nothing to read
}

bool AudioStreamPlugin::Save( AK::IXmlTextWriter* in_pWriter )
{
	return false; //Nothing to write
}

// Set internal values of the property set (allow persistence)
void AudioStreamPlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Necessary actions to take on platform change.
void AudioStreamPlugin::NotifyCurrentPlatformChanged( const GUID & in_guidCurrentPlatform )
{
}

// Take necessary action on property changes. 
// Note: not currently implemented, user also has the option of 
// catching appropriate message in WindowProc function.
void AudioStreamPlugin::NotifyPropertyChanged( const GUID & in_guidPlatform, LPCWSTR in_szPropertyName )
{
}

// Get access to UI resource handle.
HINSTANCE AudioStreamPlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Determine what dialog just get called and set the property names to UI control binding populated table.
bool AudioStreamPlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	switch ( in_eDialog )
	{
	case SettingsDialog:
		out_uiDialogID = IDD_AUDIOSTREAMPLUGIN_BIG;
		out_pTable = NULL;
		return true;

	case ContentsEditorDialog:
		out_uiDialogID = IDD_AUDIOSTREAMPLUGIN_SMALL;
		out_pTable = NULL;
		return true;
	}

	return false;
}

// Standard window function, user can intercept what ever message that is of interest to him to implement UI behavior.
bool AudioStreamPlugin::WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
	return false;
}

// Store current plugin settings into banks when asked to.
bool AudioStreamPlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
	CComVariant varProp;

	m_pPSet->GetValue( in_guidPlatform, szAudioStreamInput, varProp );
	in_pDataWriter->WriteInt32( varProp.uintVal );

	m_pPSet->GetValue( in_guidPlatform, szAudioStreamGain, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );

	return true;
}

bool AudioStreamPlugin::GetPluginData(const GUID & in_guidPlatform, AkPluginParamID in_idParam, AK::Wwise::IWriteData* in_pWriter) const
{
	return false;	//Nothing extra to pass to the sound engine
}

// Allow Wwise to retrieve a user friendly name for that property (e.g. Undo etc.).
bool AudioStreamPlugin::DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const
{
	// Get resource handle
	HINSTANCE hInst = AfxGetStaticModuleState()->m_hCurrentResourceHandle;

	if ( ! wcscmp( in_szPropertyName, szAudioStreamInput ) )
	{
		::LoadString( hInst, IDS_AUDIOSTREAM_INPUT, out_szDisplayName, in_unCharCount );
		return true;	
	}

	if ( ! wcscmp( in_szPropertyName, szAudioStreamGain ) )
	{
		::LoadString( hInst, IDS_AUDIOSTREAM_GAIN, out_szDisplayName, in_unCharCount );
		return true;	
	}

	return false;
}

// Allow Wwise to retrieve a user friendly name for that property's value (e.g. RTPC etc.).
bool AudioStreamPlugin::DisplayNamesForPropValues( LPCWSTR in_szPropertyName, LPWSTR out_szValuesName, UINT in_unCharCount ) const
{
	return false;
}

// Implement online help when the user clicks on the "?" icon .
bool AudioStreamPlugin::Help( HWND in_hWnd, eDialog in_eDialog ) const
{ // TODO: Modify help here
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	DWORD dwTopic = ONLINEHELP::Audio_Stream_Property;
	if ( in_eDialog == AK::Wwise::IAudioPlugin::ContentsEditorDialog )
		dwTopic = ONLINEHELP::Audio_Stream_Contents;

	::HtmlHelp( NULL, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, dwTopic );

	return true;
}

IPluginMediaConverter* AudioStreamPlugin::GetPluginMediaConverterInterface()
{
	return NULL;
}
