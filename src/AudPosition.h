// Copyright © 2010 CCP ehf.

#pragma once
#ifndef _AUDPOSITION_H_
#define _AUDPOSITION_H_

#include "Audio2.h"

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