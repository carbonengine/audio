/* 
	*************************************************************************************

	AudManager.h

	Author:    Andri Mar
	Created:   October 2008
	OS:        Win32
	Project:   Audio2

	Description:   

		TBA


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUD_UTILITIES_H_
#define _AUD_UTILITIES_H_

// Inherit this privatly to inhibit copying
class NoCopy
{
protected:
	NoCopy();
	~NoCopy();
private:
	NoCopy(const NoCopy&);
	NoCopy& operator=(const NoCopy&);
};

// bunch of funtions to help with converting LH data into RH
class RH2LH
{
public:
	// convert listener
	static void convertListener( AkListenerPosition* listenerLH, const AkListenerPosition* listenerRH )
	{
		memcpy( listenerLH, listenerRH, sizeof( AkListenerPosition ) );
		AkVector front = listenerLH->OrientationFront();
		AkVector top = listenerLH->OrientationTop();
		AkVector pos = listenerLH->Position();

		front.X *= -1.f;
		front.Y *= -1.f;
		top.Z *= -1.f;
		pos.Z *= -1.f; 

		listenerLH->Set(pos, front, top);
	}
	// convert emitter
	static void convertEmitter( AkSoundPosition* emitterLH, const AkSoundPosition* emitterRH )
	{
		memcpy( emitterLH, emitterRH, sizeof( AkSoundPosition ) );
		AkVector pos = emitterLH->Position();
		AkVector front = emitterLH->OrientationFront();
		AkVector top = emitterLH->OrientationTop();
		
		pos.Z *= -1.f;
		front.Z *= -1.f;
		top.Z *= -1.f;

		emitterLH->Set(pos, front, top);
	}
};

#endif
