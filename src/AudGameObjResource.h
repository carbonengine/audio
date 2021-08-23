/*
	*************************************************************************************

	AudGameObjResource.h

	Author:    Andri Mar
	Created:   February 2009
	OS:        Win32
	Project:   Audio2

	Description:

		Audio2 resource for game objects and derivatives.


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once

#include "Audio2.h"
#include "AudResource.h"
#include "AudParameter.h"

BLUE_DECLARE( AudPosition );

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCallback.h>

// Blue headers specific to this file
#include <blue/Include/IBluePlacementObserver.h>
#include <blue/Include/IBlueEventListener.h>


struct Vector3;

BLUE_CLASS( AudGameObjResource ) : public IBlueEventListener
								 , public IInitialize
	                             , public IListNotify
	                             , public AudResource
{
public:
	AudGameObjResource( IRoot* lockobj = NULL );
	virtual ~AudGameObjResource();

	EXPOSE_TO_BLUE();

	//--------------------------
	// Blue interfaces
	//--------------------------
	// IBlueEventListener
	void HandleEvent( const wchar_t* evtName ) override;
	// IInitialized
	bool Initialize() override;
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList ) override;

	virtual unsigned int PySendEvent( const std::wstring& event, bool bypassPrefix = false ); // Exposed through blue.
	virtual void StopSound( AkPlayingID playingID );
	virtual int SetAttenuationScalingFactor( float value );
	virtual int SetObstructionAndOcclusion( unsigned int listenerID, float obstruction, float occlusion );

	unsigned int PostEvent( const std::wstring& name, bool bypassPrefix = false, AkUInt32 in_uFlags = 0, AkCallbackFunc in_pfnCallback = NULL, void* in_pCookie = NULL );
	void Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position );
	void SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState );
	void SetRTPC( const std::wstring& rtpcName, float rtpcValue );

protected:
	void CreateWwiseObject() override;
	void LogInfo() override;
	virtual int SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position );
	std::wstring PrepareEvent( const std::wstring& event, bool bypassPrefix );

	AkGameObjectID m_ID;
	std::string m_name;
	AkPlayingID m_playID;
	std::wstring m_playEvent;
	bool m_playOnLoad;
	PAudParameterVector m_parameters;
	std::wstring m_eventPrefix;
	float m_scalingFactor;
	Vector3 m_position;
	bool bypassPrefix;
};

TYPEDEF_BLUECLASS( AudGameObjResource );
