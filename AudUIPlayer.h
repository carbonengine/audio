/* 
	*************************************************************************************

	AudUIPlayer.h

	Author:    Andri Mar
	Created:   January 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		A convenience class for playing UI event sounds, which are inherently non 3d.

	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/
#ifndef _AUDUIPLAYER_H_
#define _AUDUIPLAYER_H_

#include "Audio2.h"
#include "AudGameObjResource.h"

BLUE_CLASS( AudUIPlayer ) : public AudGameObjResource
{
public:
	AudUIPlayer( IRoot* lockobj = NULL );
	~AudUIPlayer();

	EXPOSE_TO_BLUE();

	virtual unsigned int SendEvent( const std::wstring& name );
};

TYPEDEF_BLUECLASS( AudUIPlayer );

#endif