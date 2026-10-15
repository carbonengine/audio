////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudUnitBox.h"

#include <cmath>

AudUnitBox::AudUnitBox() :
	m_unitBoxToWorld( IdentityMatrix() ),
	m_volume( 0.0f ),
	m_usable( false )
{
}

bool AudUnitBox::SetTransform( const Matrix& unitBoxToWorld )
{
	m_unitBoxToWorld = unitBoxToWorld;
	m_volume = std::fabs( Determinant( m_unitBoxToWorld ) );

	const Matrix& m = m_unitBoxToWorld;
	constexpr float minAxisLengthSq = 1e-6f;
	m_usable = LengthSq( Vector3( m._11, m._12, m._13 ) ) > minAxisLengthSq
		&& LengthSq( Vector3( m._21, m._22, m._23 ) ) > minAxisLengthSq
		&& LengthSq( Vector3( m._31, m._32, m._33 ) ) > minAxisLengthSq;
	return m_usable;
}

void AudUnitBox::ClearTransform()
{
	m_usable = false;
}

bool AudUnitBox::ContainsPoint( const Vector3& worldPosition ) const
{
	if( !m_usable )
	{
		return false;
	}

	const Matrix& m = m_unitBoxToWorld;
	const Vector3 axes[3] =
	{
		Vector3( m._11, m._12, m._13 ),
		Vector3( m._21, m._22, m._23 ),
		Vector3( m._31, m._32, m._33 ),
	};
	const Vector3 offset = worldPosition - Vector3( m._41, m._42, m._43 );

	for( const Vector3& axis : axes )
	{
		if( std::fabs( Dot( offset, axis ) ) > 0.5f * LengthSq( axis ) )
		{
			return false;
		}
	}
	return true;
}

Vector3 AudUnitBox::ToWorld( const Vector3& unitBoxPosition ) const
{
	return TransformCoord( unitBoxPosition, m_unitBoxToWorld );
}
