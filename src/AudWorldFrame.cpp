////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudWorldFrame.h"

#include "IEveAcousticGeometrySource.h"

#include <cmath>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudWorldFrame" );

namespace
{
	/// The origin follows the ego past this distance when nothing static is in Wwise, which costs nothing to send again.
	constexpr double SOFT_REBASE_DISTANCE = 20000.0;
	/// Past this distance it follows the ego anyway, so positions in Wwise stay small.
	constexpr double HARD_REBASE_DISTANCE = 100000.0;
}

AudWorldFrame::AudWorldFrame() :
	m_origin{},
	m_epoch( 0 ),
	m_sourceGeneration( 0 ),
	m_rebases( 0 ),
	m_active( false ),
	m_mutex( "AudWorldFrame", "m_mutex" )
{
}

AudWorldFrame::Offset AudWorldFrame::GetOffset() const
{
	CcpAutoMutex lock( m_mutex );
	return m_offset;
}

void AudWorldFrame::SetOffsetLocked( double x, double y, double z )
{
	if( m_offset.value[0] == x && m_offset.value[1] == y && m_offset.value[2] == z )
	{
		return;
	}
	m_offset.value[0] = x;
	m_offset.value[1] = y;
	m_offset.value[2] = z;
	++m_offset.generation;
}

void AudWorldFrame::Reset()
{
	if( !m_active )
	{
		return;
	}

	m_active = false;
	++m_epoch;
	CcpAutoMutex lock( m_mutex );
	SetOffsetLocked( 0.0, 0.0, 0.0 );
}

void AudWorldFrame::Update( IEveAcousticGeometrySource* source, bool wanted, bool pinned )
{
	double reference[3];
	int64_t frameTime = 0;
	uint64_t generation = 0;
	if( !wanted || source == nullptr || !source->GetAudioSpaceOrigin( reference, &frameTime, &generation )
		|| !std::isfinite( reference[0] ) || !std::isfinite( reference[1] ) || !std::isfinite( reference[2] ) )
	{
		Reset();
		return;
	}

	const double dx = reference[0] - m_origin[0];
	const double dy = reference[1] - m_origin[1];
	const double dz = reference[2] - m_origin[2];
	const double distance = std::sqrt( dx * dx + dy * dy + dz * dz );

	const bool activating = !m_active;
	const bool rebase = activating
		|| generation != m_sourceGeneration
		|| distance > HARD_REBASE_DISTANCE
		|| ( distance > SOFT_REBASE_DISTANCE && !pinned );

	if( rebase )
	{
		if( !activating )
		{
			++m_rebases;
			CCP_LOG_CH( s_ch, "World frame origin moved %.0f m.", distance );
		}
		m_origin[0] = reference[0];
		m_origin[1] = reference[1];
		m_origin[2] = reference[2];
		m_sourceGeneration = generation;
		m_active = true;
		++m_epoch;
	}

	CcpAutoMutex lock( m_mutex );
	SetOffsetLocked( reference[0] - m_origin[0], reference[1] - m_origin[1], reference[2] - m_origin[2] );
}
