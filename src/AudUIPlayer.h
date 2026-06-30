// Copyright © 2009 CCP ehf.

#pragma once
#ifndef _AUDUIPLAYER_H_
#define _AUDUIPLAYER_H_

#include "AudEmitter.h"

BLUE_CLASS( AudUIPlayer ) : public AudEmitter 
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
};

TYPEDEF_BLUECLASS( AudUIPlayer );

#endif