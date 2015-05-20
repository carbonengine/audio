/* 
	*************************************************************************************

	AudEmitterDoppler.h

	Author:    Jon Hallur Haraldsson
	Created:   April 2015
	OS:        Win32
	Project:   Audio2

	Description:   

		An audio entity ingame. Wrapper for the GameObject concept in Wwise which does
		doppler shift calculations.


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUDEMITTERDOPPLER_H_
#define _AUDEMITTERDOPPLER_H_

#include "Audio2.h"
#include "AudEmitter.h"

BLUE_CLASS( AudEmitterDoppler ) : public AudEmitter
{
public:
	AudEmitterDoppler( IRoot* lockobj = NULL );
	virtual ~AudEmitterDoppler();

	EXPOSE_TO_BLUE();

	//--------------------------
	// Blue interfaces
	//--------------------------
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos );

protected:
	void Py__init__( const std::string& name, const std::string& rtpc, const int duration);
	
	AkTimeMs m_dopplerChangeDuration;
	AkReal32 m_lastKnowDistanceSq;
	AkUInt32 m_rtpcID;
	boolean m_rtpcIsDirty;

};

TYPEDEF_BLUECLASS( AudEmitterDoppler );

#endif