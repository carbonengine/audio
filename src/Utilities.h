// Copyright © 2008 CCP ehf.

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

class RH2LH
{
public:
	static void convertListener( AkListenerPosition* listenerLH, const AkListenerPosition* listenerRH )
	{
		memcpy( listenerLH, listenerRH, sizeof( AkListenerPosition ) );
		AkVector front = listenerLH->OrientationFront();
		AkVector top = listenerLH->OrientationTop();
		AkVector64 pos = listenerLH->Position();

		front.X *= -1.f;
		front.Y *= -1.f;
		top.Z *= -1.f;
		pos.Z *= -1.f; 

		listenerLH->Set(pos, front, top);
	}
	static void convertEmitter( AkSoundPosition* emitterLH, const AkSoundPosition* emitterRH )
	{
		memcpy( emitterLH, emitterRH, sizeof( AkSoundPosition ) );
		AkVector front = emitterLH->OrientationFront();
		AkVector top = emitterLH->OrientationTop();
		AkVector64 pos = emitterLH->Position();
		
		pos.Z *= -1.f;
		front.Z *= -1.f;
		top.Z *= -1.f;

		emitterLH->Set(pos, front, top);
	}
};

class StringUtils
{
public:
	// Trim whitespace from both ends of string
	static std::wstring trim(const std::wstring& str)
	{
		const std::wstring whitespace = L" \t\n\r\f\v";

		size_t start = str.find_first_not_of( whitespace );
		if( start == std::wstring::npos ) 
		{
			return L"";
		}

		size_t end = str.find_last_not_of( whitespace );
		return str.substr( start, end - start + 1 );
	}
};

#endif
