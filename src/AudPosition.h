/* 
	*************************************************************************************

	AudPositon.h

	Author:    Andri Mar
	Created:   June 2010
	OS:        Win32
	Project:   Audio2

	Description:   

		Represents a position in game for audio objects.


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUDPOSITION_H_
#define _AUDPOSITION_H_

#include "Audio2.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <blue/include/IBluePlacementObserver.h>

struct Vector3;

BLUE_DECLARE( AudEmitterMulti );

BLUE_CLASS( AudPosition ) : public IBluePlacementObserver
{
public:
	AudPosition( IRoot* lockobj = NULL );
	~AudPosition();

	EXPOSE_TO_BLUE();

	//--------------------------
	// Blue interfaces
	//--------------------------
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos );
	// INotify
	virtual bool OnModified( Be::Var* value );

	AkSoundPosition		m_value;
};

TYPEDEF_BLUECLASS( AudPosition );
BLUE_DECLARE_VECTOR( AudPosition );

#endif