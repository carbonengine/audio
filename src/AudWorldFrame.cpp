////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudWorldFrame.h"

#include <algorithm>
#include <cmath>

namespace
{
	AkVector64 MakeVector64( AkReal64 x, AkReal64 y, AkReal64 z )
	{
		AkVector64 v;
		v.X = x;
		v.Y = y;
		v.Z = z;
		return v;
	}

	bool SameVector64( const AkVector64& a, const AkVector64& b )
	{
		return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
	}

	AkReal64 Distance( const AkVector64& a, const AkVector64& b )
	{
		const AkVector64 d = a - b;
		return std::sqrt( d.X * d.X + d.Y * d.Y + d.Z * d.Z );
	}
}

AudWorldFrame& AudWorldFrame::Get()
{
	static AudWorldFrame s_frame;
	return s_frame;
}

AudWorldFrame::AudWorldFrame() :
	m_mutex( "AudWorldFrame", "m_mutex" ),
	m_sceneOrigin( MakeVector64( 0.0, 0.0, 0.0 ) ),
	m_wwiseOrigin( MakeVector64( 0.0, 0.0, 0.0 ) ),
	m_firstSceneOrigin( MakeVector64( 0.0, 0.0, 0.0 ) ),
	m_hasSceneOrigin( false ),
	m_rebaseDistance( DEFAULT_REBASE_DISTANCE ),
	m_offsetGeneration( 0 ),
	m_rebaseCount( 0 )
{
}

void AudWorldFrame::SetSceneOrigin( const AkVector64& origin )
{
	if( !std::isfinite( origin.X ) || !std::isfinite( origin.Y ) || !std::isfinite( origin.Z ) )
	{
		return;
	}

	CcpAutoMutex lock( m_mutex );

	if( !m_hasSceneOrigin )
	{
		// Wwise has had positions relative to the scene origin so far, the same as measuring from it.
		m_sceneOrigin = origin;
		m_wwiseOrigin = origin;
		m_firstSceneOrigin = origin;
		m_hasSceneOrigin = true;
		return;
	}

	// An unchanged origin is still checked, so the Wwise origin catches up once a warp ends.
	const AkVector64 offsetBefore = m_sceneOrigin - m_wwiseOrigin;
	const bool inTransit = Distance( origin, m_sceneOrigin ) > TRANSIT_STEP;
	m_sceneOrigin = origin;
	if( !inTransit && Distance( m_sceneOrigin, m_wwiseOrigin ) > m_rebaseDistance )
	{
		m_wwiseOrigin = m_sceneOrigin;
		++m_rebaseCount;
	}

	if( !SameVector64( offsetBefore, m_sceneOrigin - m_wwiseOrigin ) )
	{
		++m_offsetGeneration;
	}
}

AudWorldFrame::State AudWorldFrame::GetState() const
{
	CcpAutoMutex lock( m_mutex );
	return { m_sceneOrigin, m_wwiseOrigin, m_hasSceneOrigin, m_firstSceneOrigin };
}

AkVector64 AudWorldFrame::GetSceneToWwise() const
{
	CcpAutoMutex lock( m_mutex );
	return m_sceneOrigin - m_wwiseOrigin;
}

AkVector64 AudWorldFrame::ToWwise( const Vector3& sceneRelative ) const
{
	const AkVector64 offset = GetSceneToWwise();
	return MakeVector64( sceneRelative.x + offset.X, sceneRelative.y + offset.Y, sceneRelative.z + offset.Z );
}

Matrix AudWorldFrame::ToWwise( const Matrix& sceneRelative ) const
{
	const AkVector64 offset = GetSceneToWwise();
	Matrix m = sceneRelative;
	m._41 = static_cast<float>( m._41 + offset.X );
	m._42 = static_cast<float>( m._42 + offset.Y );
	m._43 = static_cast<float>( m._43 + offset.Z );
	return m;
}

double AudWorldFrame::StillDistance( const Matrix& sceneRelative )
{
	constexpr double minimum = 1.0e-3;
	constexpr double fraction = 1.0e-4;
	const double x = sceneRelative._41;
	const double y = sceneRelative._42;
	const double z = sceneRelative._43;
	return std::max( minimum, fraction * std::sqrt( x * x + y * y + z * z ) );
}

double AudWorldFrame::GetRebaseDistance() const
{
	CcpAutoMutex lock( m_mutex );
	return m_rebaseDistance;
}

void AudWorldFrame::SetRebaseDistance( double distance )
{
	CcpAutoMutex lock( m_mutex );
	// Infinity never rebases, so Wwise gets absolute coordinates.
	m_rebaseDistance = std::isnan( distance ) ? DEFAULT_REBASE_DISTANCE : std::max( 0.0, distance );
}

uint64_t AudWorldFrame::GetOffsetGeneration() const
{
	CcpAutoMutex lock( m_mutex );
	return m_offsetGeneration;
}

uint64_t AudWorldFrame::GetRebaseCount() const
{
	CcpAutoMutex lock( m_mutex );
	return m_rebaseCount;
}
