//////////////////////////////////////////////////////////////////////
//
// AudioStreamPlugin.h
//
// Audio Stream Wwise plugin implementation.
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/AudioPlugin.h>

// Audio input plugin property names
static LPCTSTR szAudioStreamInput       = L"AudioStreamInput";
static LPCTSTR szAudioStreamGain        = L"AudioStreamGain";

class AudioStreamPlugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	AudioStreamPlugin();
	~AudioStreamPlugin();

	// AK::Wwise::IPluginBase
	virtual void Destroy();

	// AK::Wwise::IAudioPlugin

	virtual void InitToDefault() {}

	virtual void Delete() {}

	/// Load file 
	virtual bool Load( AK::IXmlTextReader* in_pReader );
	/// Save file
	virtual bool Save( AK::IXmlTextWriter* in_pWriter );
	virtual bool CopyInto( IAudioPlugin* io_pWObject ) const { return true; }

	virtual bool IsPlayable() const;
	virtual void SetPluginPropertySet( AK::Wwise::IPluginPropertySet * in_pPSet );
	virtual void SetPluginObjectStore( AK::Wwise::IPluginObjectStore * in_pObjectStore ){}
	virtual void SetPluginObjectMedia( AK::Wwise::IPluginObjectMedia * in_pObjectStore ){}

	virtual void NotifyCurrentPlatformChanged( const GUID & in_guidCurrentPlatform );
	virtual void NotifyPropertyChanged( const GUID & in_guidPlatform, LPCWSTR in_szPropertyName );
	virtual void NotifyInnerObjectPropertyChanged( AK::Wwise::IPluginPropertySet* in_pPSet, const GUID & in_guidPlatform, LPCWSTR in_pszPropertyName ){}
	virtual void NotifyInnerObjectAddedRemoved( AK::Wwise::IPluginPropertySet* in_pPSet, unsigned int in_uiIndex, AK::Wwise::IAudioPlugin::NotifyInnerObjectOperation in_eOperation	){}

	virtual HINSTANCE GetResourceHandle() const;
	virtual bool GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable ) const;
	virtual bool WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );

    virtual bool GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const;
	virtual bool GetPluginData( const GUID & in_guidPlatform, AkPluginParamID in_idParam, AK::Wwise::IWriteData* in_pDataWriter ) const;

	virtual bool DisplayNameForProp( LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount ) const;
	virtual bool DisplayNamesForPropValues( LPCWSTR in_szPropertyName, LPWSTR out_szValuesName, UINT in_unCharCount ) const;

	virtual void NotifyMonitorData( void * in_pData, unsigned int in_uDataSize, bool in_bNeedsByteSwap ) {}

	virtual AK::Wwise::IPluginMediaConverter* GetPluginMediaConverterInterface();

	virtual bool Help( HWND in_hWnd, eDialog in_eDialog ) const;

	static void GetFormatCallbackFunc(
		AkPlayingID		in_playingID,   ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
		AkAudioFormat&  io_AudioFormat  ///< Already filled format, modify it if required.
		);

	static void ExecuteCallbackFunc(
		AkPlayingID		in_playingID,  ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
		AkAudioBuffer*	io_pBufferOut  ///< Buffer to fill
	    );

	static const short CompanyID;
	static const short PluginID;

private:
	AK::Wwise::IPluginPropertySet * m_pPSet;
};
