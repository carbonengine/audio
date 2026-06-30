// Copyright © 2008 CCP ehf.

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