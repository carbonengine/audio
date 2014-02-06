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

struct Vector3 : public AkVector
{
	Vector3()
	{}
	Vector3(float x, float y, float z)
	{
		X = x;
		Y = y;
		Z = z;
	}
};

#endif