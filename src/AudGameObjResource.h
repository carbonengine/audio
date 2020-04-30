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
#ifndef _AUDGAMEOBJRESOURCE_H_
#define _AUDGAMEOBJRESOURCE_H_

#include "Audio2.h"
#include "AudResource.h"
#include "AudParameter.h"

BLUE_DECLARE( AudPosition );

#include <AK/SoundEngine/Common/AkTypes.h>

// Blue headers specific to this file
#include <blue/include/IBluePlacementObserver.h>
#include <blue/include/IBlueEventListener.h>


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
	virtual void HandleEvent( const wchar_t* evtName ) override;
	// IInitialized
	virtual bool Initialize() override;
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList ) override;

	virtual unsigned int SendEvent( const std::wstring& name );
	virtual int SetAttenuationScalingFactor( float value );
	virtual int SetObstructionAndOcclusion( unsigned int listenerID, float obstruction, float occlusion );

	void Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position );
	void SendSoundEvent( const wchar_t* eventName );
	void SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState );
	void SetRTPC( const std::wstring& rtpcName, float rtpcValue );

protected:
	virtual void CreateWwiseObject();
	virtual void LogInfo();
	virtual int SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position );

	AkGameObjectID		m_ID;
	std::string			m_name;
	AkPlayingID			m_playID;
	std::wstring		m_playEvent;
	bool				m_playOnLoad;
	PAudParameterVector m_parameters;
	std::wstring		m_eventPrefix;
	float				m_scalingFactor;
	Vector3				m_position;
};

TYPEDEF_BLUECLASS( AudGameObjResource );

#endif
