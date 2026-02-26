////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Contributors: Eric Nielsen
// Creation Date: October 2008
// Copyright (c) 2008-2022, CCP Games
//

#pragma once
#ifndef _AUD_UTILITIES_H_
#define _AUD_UTILITIES_H_

#include <AK/Tools/Common/AkVectors.h>

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

	static void convertTransform( const Matrix& matrix, AkTransform& out )
	{
		constexpr float kEpsilon = 1e-10f;

		Ak3DVector32 position( matrix._41, matrix._42, -matrix._43 );
		if( !position.IsFinite() )
			position = Ak3DVector32( 0.0f, 0.0f, 0.0f );

		Ak3DVector32 front( -matrix._31, -matrix._32, matrix._33 );
		if( !front.IsFinite() || front.LengthSquared() <= kEpsilon )
			front = Ak3DVector32( 0.0f, 0.0f, 1.0f );
		else
			front.Normalize();

		Ak3DVector32 up( matrix._21, matrix._22, -matrix._23 );
		if( !up.IsFinite() || up.LengthSquared() <= kEpsilon )
			up = Ak3DVector32( 0.0f, 1.0f, 0.0f );
		else
			up.Normalize();

		Ak3DVector32 right = up.Cross( front );
		if( right.LengthSquared() <= kEpsilon )
		{
			up = ( std::fabs( front.Y ) < 0.9f )
				? Ak3DVector32( 0.0f, 1.0f, 0.0f )
				: Ak3DVector32( 0.0f, 0.0f, 1.0f );
			right = up.Cross( front );
		}
		right.Normalize();
		up = front.Cross( right );
		up.Normalize();

		out.SetPosition( position );
		out.SetOrientation( front, up );
	}

	static AkVector extractScale( const Matrix& matrix )
	{
		auto axisLength = []( float x, float y, float z ) -> float
		{
			const Ak3DVector32 axis( x, y, z );
			if( !axis.IsFinite() || axis.LengthSquared() <= 1e-10f )
				return 1.0f;
			return axis.Length();
		};

		AkVector scale;
		scale.X = axisLength( matrix._11, matrix._12, matrix._13 );
		scale.Y = axisLength( matrix._21, matrix._22, matrix._23 );
		scale.Z = axisLength( matrix._31, matrix._32, matrix._33 );
		return scale;
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
