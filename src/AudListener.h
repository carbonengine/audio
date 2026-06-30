// Copyright © 2008 CCP ehf.

#ifndef _AUDLISTENER_H_
#define _AUDLISTENER_H_

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
	int SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position ) override;
};

TYPEDEF_BLUECLASS( AudListener );

#endif