////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Creation Date: June 2010
// Copyright (c) 2010-2022, CCP Games
//

#pragma once
#ifndef _AUDPOSITION_H_
#define _AUDPOSITION_H_

#include "Audio2.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <IBluePlacementObserver.h>

struct Vector3;

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
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos ) override;
	// INotify
	virtual bool OnModified( Be::Var* value );

	AkSoundPosition		m_value;
};

TYPEDEF_BLUECLASS( AudPosition );
BLUE_DECLARE_VECTOR( AudPosition );

#endif