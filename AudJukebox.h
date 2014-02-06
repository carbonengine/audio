/* 
	*************************************************************************************

	AudJukebox.h

	Author:    Andri Mar
	Created:   February 2009
	Updated:   June 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		A logical representation of a jukebox ingame.


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDJUKEBOX_H_
#define _AUDJUKEBOX_H_

#include "Audio2.h"
#include "AudGameObjResource.h"
#include "AudMp3Info.h"

BLUE_DECLARE( AudPlaylistItem );
BLUE_DECLARE( AudMp3Tag );

// Blue headers specific to this file
#include <blue/include/IBlueEventListener.h>

struct PayLoad
{
	PayLoad( const std::wstring& evtName, IBlueEventListenerPtr evtListener ) :
		name( evtName ), eventListener( evtListener )
	{}
	std::wstring name;
	IBlueEventListenerPtr eventListener;
};

BLUE_CLASS( AudJukebox ) : public AudGameObjResource
{
public:
	AudJukebox( IRoot* lockobj = NULL );
	~AudJukebox();

	EXPOSE_TO_BLUE();

	// Overrides...
	virtual unsigned int SendEvent( const std::wstring& name );

	void Play( const std::wstring& absPath );

private:
	IBlueEventListenerPtr m_eventListener;
	//AudPlaylistItemPtr m_currentSong;
	CCPMP3BaseInfo m_baseInfo;
};
TYPEDEF_BLUECLASS( AudJukebox );

#endif