/* 
	*************************************************************************************

	AudEmitterPersisted.h

	Author:    Andri Mar
	Created:   December 2008
	OS:        Win32
	Project:   Audio2

	Description:   

		An audio entity ingame. Wrapper for the GameObject concept in Wwise.
		This is a specialation that takes an extra argument. An event that is
		to be posted to WWise on the objects construction.


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AudEmitterPersisted_H_
#define _AudEmitterPersisted_H_

#include "Audio2.h"

#include "AudEmitter.h"

struct Vector3;

BLUE_CLASS( AudEmitterPersisted ) : public INotify, public AudEmitter
{
public:
	AudEmitterPersisted( IRoot* lockobj = NULL );
	virtual ~AudEmitterPersisted();

	EXPOSE_TO_BLUE();

	// Initialize
	virtual bool Initialize() override;

	// INotify
	virtual bool OnModified( Be::Var* value ) override;

	//Other
	void Py__init__( const std::string& name, const std::wstring& playEvent, bool playOnLoad = true );

protected:
	std::wstring m_playEvent;
	bool m_playOnLoad;
};

TYPEDEF_BLUECLASS( AudEmitterPersisted );

#endif