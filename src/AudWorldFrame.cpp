////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudWorldFrame.h"

#include <AK/Tools/Common/AkVectors.h>

#include <algorithm>
#include <cmath>

AudWorldFrame& AudWorldFrame::Get()
{
	static AudWorldFrame s_frame;
	return s_frame;
}

AudWorldFrame::AudWorldFrame() :
	m_mutex( "AudWorldFrame", "m_mutex" ),
	m_sceneOrigin(),
	m_wwiseOrigin(),
	m_hasSceneOrigin( false ),
	m_owner( 0 ),
	m_hasOwner( false ),
	m_otherCalls( 0 ),
	m_rebaseDistance( DEFAULT_REBASE_DISTANCE ),
	m_offsetGeneration( 0 ),
	m_rebaseCount( 0 )
{
}

void AudWorldFrame::SetSceneOrigin( const AkVector64& origin, uint64_t source )
{
	if( !Ak3DVector64( origin ).IsFinite() )
	{
		return;
	}

	CcpAutoMutex lock( m_mutex );

	if( m_hasOwner && source != m_owner && ++m_otherCalls < TAKEOVER_CALLS )
	{
		// The owner is still calling, or only just stopped.
		return;
	}
	m_owner = source;
	m_hasOwner = true;
	m_otherCalls = 0;

	if( !m_hasSceneOrigin )
	{
		// Wwise has had positions relative to the scene origin so far, the same as measuring from it.
		m_sceneOrigin = origin;
		m_wwiseOrigin = origin;
		m_hasSceneOrigin = true;
		return;
	}

	// An unchanged origin is still checked, so the Wwise origin catches up once a warp ends.
	const Ak3DVector64 offsetBefore = Ak3DVector64( m_sceneOrigin ) - Ak3DVector64( m_wwiseOrigin );
	const bool inTransit = ( Ak3DVector64( origin ) - Ak3DVector64( m_sceneOrigin ) ).Length() > TRANSIT_STEP;
	m_sceneOrigin = origin;
	if( !inTransit && ( Ak3DVector64( m_sceneOrigin ) - Ak3DVector64( m_wwiseOrigin ) ).Length() > m_rebaseDistance )
	{
		m_wwiseOrigin = m_sceneOrigin;
		++m_rebaseCount;
	}

	if( !( offsetBefore == Ak3DVector64( m_sceneOrigin ) - Ak3DVector64( m_wwiseOrigin ) ) )
	{
		++m_offsetGeneration;
	}
}

void AudWorldFrame::ReleaseSource( uint64_t source )
{
	CcpAutoMutex lock( m_mutex );
	if( m_hasOwner && m_owner == source )
	{
		m_hasOwner = false;
		m_otherCalls = 0;
	}
}

AudWorldFrame::Offset AudWorldFrame::GetOffset() const
{
	CcpAutoMutex lock( m_mutex );
	return { m_sceneOrigin - m_wwiseOrigin, m_offsetGeneration, m_rebaseCount };
}

double AudWorldFrame::StillDistance( const Matrix& sceneRelative )
{
	constexpr double fraction = 1.0e-4;
	const Ak3DVector64 translation( sceneRelative._41, sceneRelative._42, sceneRelative._43 );
	return std::max( MIN_STILL_DISTANCE, fraction * translation.Length() );
}

double AudWorldFrame::GetRebaseDistance() const
{
	CcpAutoMutex lock( m_mutex );
	return m_rebaseDistance;
}

void AudWorldFrame::SetRebaseDistance( double distance )
{
	CcpAutoMutex lock( m_mutex );
	// Infinity never rebases.
	m_rebaseDistance = std::isnan( distance ) ? DEFAULT_REBASE_DISTANCE : std::max( 0.0, distance );
}
