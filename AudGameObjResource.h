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
	virtual void HandleEvent( const wchar_t* evtName );
	// IInitialized
	virtual bool Initialize();
	// IListNotify
	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	virtual unsigned int SendEvent( const std::wstring& name );
	virtual int SetAttenuationScalingFactor( float value );
	virtual int SetObstructionAndOcclusion( unsigned int listenerID, float obstruction, float occlusion );
	virtual int SetSwitch( const wchar_t* groupName, const wchar_t* switchName );

protected:
	virtual void CreateWwiseObject();
	virtual void LogInfo();
	virtual int SetPositionHelper( const AkSoundPosition& position );

	AkGameObjectID		m_ID;
	std::string			m_name;
	AkPlayingID			m_playID;
	std::wstring		m_playEvent;
	bool				m_playOnLoad;
	PAudParameterVector m_parameters;
	std::wstring		m_eventPrefix;
	float				m_scalingFactor;
};

TYPEDEF_BLUECLASS( AudGameObjResource );

#endif
