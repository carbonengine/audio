////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Contributors: Eric Nielsen
// Creation Date: November 2008
// Copyright (c) 2008-2022, CCP Games
//
// Bare bones Vector3 support for audio2
//

#pragma once
#ifndef VECTOR3_H
#define VECTOR3_H

inline AkVector MakeAkVector( const Vector3& v )
{
	AkVector r;
	r.X = v.x;
	r.Y = v.y;
	r.Z = v.z;
	return r;
}

#endif