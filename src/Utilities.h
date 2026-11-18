// Copyright © 2008 CCP ehf.

#pragma once
#ifndef _AUD_UTILITIES_H_
#define _AUD_UTILITIES_H_

#include <AK/Tools/Common/AkVectors.h>

#include <algorithm>
#include <cmath>

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

		pos.Z *= -1.f;
		top.Z *= -1.f;

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

	/// convertTransform, with the position moved by offset (right-handed) in double precision.
	static void convertTransformOffset( const Matrix& matrix, const double offset[3], AkWorldTransform& out )
	{
		AkTransform transform;
		convertTransform( matrix, transform );
		const AkVector64 position = { matrix._41 + offset[0], matrix._42 + offset[1], -( matrix._43 + offset[2] ) };
		out.Set( position, transform.OrientationFront(), transform.OrientationTop() );
	}

	/// A right-handed rigid pose (position, unit quaternion x y z w) in double precision, the same way convertTransform maps axes.
	static void convertRigid( const double position[3], const double rotation[4], AkWorldTransform& out )
	{
		const double x = rotation[0], y = rotation[1], z = rotation[2], w = rotation[3];
		// Rotated local Y and Z axes.
		const double yAxis[3] = { 2.0 * ( x * y - w * z ), 1.0 - 2.0 * ( x * x + z * z ), 2.0 * ( y * z + w * x ) };
		const double zAxis[3] = { 2.0 * ( x * z + w * y ), 2.0 * ( y * z - w * x ), 1.0 - 2.0 * ( x * x + y * y ) };

		const auto unit = []( double a, double b, double c )
		{
			const double length = std::sqrt( a * a + b * b + c * c );
			AkVector v;
			v.X = static_cast<AkReal32>( a / length );
			v.Y = static_cast<AkReal32>( b / length );
			v.Z = static_cast<AkReal32>( c / length );
			return v;
		};

		const AkVector64 lhPosition = { position[0], position[1], -position[2] };
		out.Set( lhPosition, unit( -zAxis[0], -zAxis[1], zAxis[2] ), unit( yAxis[0], yAxis[1], -yAxis[2] ) );
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

	/// How far a static transform from trinity can seem to move in the world frame: trinity's positions are floats
	/// relative to the ego, so they jitter by the float rounding at their distance from it.
	static double sceneJitterTolerance( const Matrix& matrix )
	{
		const double x = matrix._41, y = matrix._42, z = matrix._43;
		return ( std::max )( 1e-2, std::sqrt( x * x + y * y + z * z ) * 4e-6 );
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
