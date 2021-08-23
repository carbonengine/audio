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
#include <AK/SoundEngine/Common/AkCallback.h>

BLUE_CLASS( AudUIPlayer ) : public AudGameObjResource
{
public:
	AudUIPlayer( IRoot* lockobj = NULL );
	~AudUIPlayer();

	EXPOSE_TO_BLUE();
	
	unsigned int SendEventWithCallback( const std:: wstring& name );
	void Callback( std::wstring eventName );

protected:
	BlueScriptCallback	m_callback;
	std::wstring		m_callbackEventName;

private:
	//using AudGameObjResource::SendEvent; // Silence warning about this hidden function

};

TYPEDEF_BLUECLASS( AudUIPlayer );

#endif
