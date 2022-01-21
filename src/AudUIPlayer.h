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
	// Post an event meant for dialogue. Allows for getting the duration of the playing event.
	unsigned int PostDialogueEvent( const std:: wstring& eventName );
	// Get elapsed time of the playing event in milliseconds for the given playingID.  
	int32_t GetEventPlayPosition( const unsigned int playingID );
	void EventFinishedCallback( AkEventCallbackInfo* cbInfo ) override; 
	static void PropagatePythonCallback( void* cookie );

	BlueScriptCallback	m_callback;

protected:
	std::wstring		m_callbackEventName;

private:
	//using AudGameObjResource::SendEvent; // Silence warning about this hidden function

};

TYPEDEF_BLUECLASS( AudUIPlayer );

#endif