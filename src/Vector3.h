/* 
	*************************************************************************************

	Vector3.h

	Creator:	Andri Mar
	Created:	November 2008
	Project:	Destiny

	Bare bones Vector3 support for .

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef VECTOR3_H
#define VECTOR3_H

#include <AK/SoundEngine/Common/AkTypes.h>

inline AkVector MakeAkVector( const Vector3& v )
{
	AkVector r;
	r.X = v.x;
	r.Y = v.y;
	r.Z = v.z;
	return r;
}

#endif