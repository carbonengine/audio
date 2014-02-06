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
		listenerLH->OrientationFront.X *= -1.f;
		listenerLH->OrientationFront.Y *= -1.f;

		listenerLH->OrientationTop.Z *= -1.f;

		listenerLH->Position.Z *= -1.f;
	}
	// convert emitter
	static void convertEmitter( AkSoundPosition* emitterLH, const AkSoundPosition* emitterRH )
	{
		memcpy( emitterLH, emitterRH, sizeof( AkSoundPosition ) );
		emitterLH->Position.Z *= -1.f;

		emitterLH->Orientation.Z *= -1.f;
	}
};

#endif
