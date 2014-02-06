/* 
	*************************************************************************************

	AudEmitterMulti.h

	Author:    Andri Mar
	Created:   June 2010
	OS:        Win32
	Project:   Audio2

	Description:   

		An audio entity in game. Can stream one source to multiple positions.


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUDEMITTERMULTI_H_
#define _AUDEMITTERMULTI_H_

#include "AudGameObjResource.h"
#include "AudPosition.h"

// Blue headers specific to this file
#include <blue/include/IBluePlacementObserver.h>
#include <blue/include/IBlueEventListener.h>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <vector>

struct Vector3;

BLUE_CLASS( AudEmitterMulti ) :	public IBlueMultiPlacementObserver
							  , public AudGameObjResource
{
public:
	AudEmitterMulti( IRoot* lockobj = NULL );
	virtual ~AudEmitterMulti();

	EXPOSE_TO_BLUE();
	
	void Py__init__( const std::string& name );
	virtual void UpdatePlacements( const PositionDescriptionVector& positions );
};

TYPEDEF_BLUECLASS( AudEmitterMulti );
#endif
