/* 
	*************************************************************************************

	AudListener.h

	Author:    Andri Mar
	Created:   October 2008
	OS:        Win32
	Project:   Audio2

	Description:   

		TBA


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#ifndef _AUDLISTENER_H_
#define _AUDLISTENER_H_

#include "Audio2.h"

#include <AK/SoundEngine/Common/AkTypes.h>

// Blue headers specific to this file
#include <blue/include/IBluePlacementObserver.h>

#include "AudResource.h"
struct Vector3;

BLUE_CLASS( AudListener ) : public IBluePlacementObserver,
							public AudResource
{
public:
	AudListener( IRoot* lockobj = NULL );
	~AudListener();
	
	EXPOSE_TO_BLUE();

	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos );
	// IInitialize
	void Initialize();

	virtual int SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos );

	AkGameObjectID m_ID;
protected:
	virtual void CreateWwiseObject();
	virtual void LogInfo();

};

TYPEDEF_BLUECLASS( AudListener );

#endif