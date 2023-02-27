////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Contributors: Eric Nielsen
// Creation Date: October 2008
// Copyright (c) 2008-2022, CCP Games
//

#ifndef _AUDLISTENER_H_
#define _AUDLISTENER_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#include <IBluePlacementObserver.h>

#include "Audio2.h"
#include "AudGameObjResource.h"

struct Vector3;

// ------------------------------------------------------------------------
// Description:
//   The game object that represents the listener for audio in game. All calculations
//   about what the player can hear are relative to this game object's position.
// ------------------------------------------------------------------------
BLUE_CLASS( AudListener ) : public AudGameObjResource 
{
public:
	AudListener( IRoot* lockobj = NULL );
	~AudListener();
	
	EXPOSE_TO_BLUE();

	void RegisterWwiseObject() override;
};

TYPEDEF_BLUECLASS( AudListener );

#endif