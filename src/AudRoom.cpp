////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "AudRoom.h"

#include "AudManager.h"
#include "AudRoomManager.h"

#include <cmath>

AudRoom::AudRoom( IRoot* lockobj ) :
	m_roomID( AllocateGameObjectID() ),
	m_reverbLevel( 1.0f ),
	m_transmissionLoss( 1.0f ),
	m_priority( 100.0f ),
	m_auxSendLevelToSelf( 0.0f ),
	m_keepRegistered( false ),
	m_enabled( true ),
	m_shapeEnabled( true ),
	m_unitBoxToWorld( IdentityMatrix() ),
	m_volume( 0.0f ),
	m_shapeValid( false ),
	m_warnedDegenerate( false ),
	m_hasTransform( false ),
	m_sentToWwise( false ),
	m_registeredWithManager( false ),
	m_roomTonePlayingID( AK_INVALID_PLAYING_ID ),
	m_roomTonePending( false )
{
	EnsureRegistered();
}

AudRoom::~AudRoom()
{
	if( g_audioManager != nullptr && m_registeredWithManager )
	{
		g_audioManager->GetRoomManager().UnregisterRoom( this );
	}
}

bool AudRoom::EnsureRegistered()
{
	if( m_registeredWithManager )
	{
		return true;
	}

	if( g_audioManager == nullptr )
	{
		return false;
	}

	g_audioManager->GetRoomManager().RegisterRoom( this );
	m_registeredWithManager = true;
	return true;
}

void AudRoom::Sync()
{
	if( !EnsureRegistered() )
	{
		return;
	}

	AudRoomManager& manager = g_audioManager->GetRoomManager();
	if( IsEnabled() && m_hasTransform && m_shapeValid )
	{
		manager.Push( *this );
	}
	else
	{
		manager.Remove( *this );
	}
}

void AudRoom::SetTransform( const Matrix& unitBoxToWorld )
{
	if( EnsureRegistered() )
	{
		// Emitter position reports, some from trinity worker threads, test against the box; the manager stores it under its lock.
		g_audioManager->GetRoomManager().SetTransform( *this, unitBoxToWorld );
	}
	else
	{
		// No audio manager yet: nothing reads the box, so store it for when the manager appears.
		ApplyTransform( unitBoxToWorld );
	}
}

void AudRoom::ApplyTransform( const Matrix& unitBoxToWorld )
{
	m_unitBoxToWorld = unitBoxToWorld;
	// The determinant of the affine part is the volume scale of the unit cube.
	m_volume = std::fabs( Determinant( m_unitBoxToWorld ) );
	m_hasTransform = true;

	// A box with a zero-length axis (e.g. a freshly placed, unscaled volume) must not become a 1 m cube in Wwise.
	const Matrix& m = m_unitBoxToWorld;
	constexpr float minAxisLengthSq = 1e-6f; // 1 mm
	m_shapeValid = LengthSq( Vector3( m._11, m._12, m._13 ) ) > minAxisLengthSq
		&& LengthSq( Vector3( m._21, m._22, m._23 ) ) > minAxisLengthSq
		&& LengthSq( Vector3( m._31, m._32, m._33 ) ) > minAxisLengthSq;
	if( !m_shapeValid && !m_warnedDegenerate )
	{
		CCP_LOGWARN( "Room '%s' (%llu) has a box with a zero-length axis; it is not sent to Wwise until it is scaled.", m_name.c_str(), m_roomID );
		m_warnedDegenerate = true;
	}
}

bool AudRoom::ContainsPoint( const Vector3& worldPosition ) const
{
	if( !m_hasTransform || !m_shapeValid )
	{
		return false;
	}

	// Row-vector convention: rows 1..3 are the box axes in world space (scaled), row 4 is the centre.
	// A point is inside when its offset from the centre projects within half of each axis; no inverse needed.
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
		const float lengthSq = LengthSq( axis );
		if( lengthSq <= 1e-12f )
		{
			return false; // degenerate box, contains nothing
		}
		if( std::fabs( Dot( offset, axis ) ) > 0.5f * lengthSq )
		{
			return false;
		}
	}
	return true;
}

void AudRoom::SetEnabled( bool enabled )
{
	if( m_shapeEnabled == enabled )
	{
		return;
	}

	m_shapeEnabled = enabled;
	Sync();
}

void AudRoom::Remove()
{
	if( EnsureRegistered() )
	{
		// Clears the box under the manager's lock, then removes the room from Wwise.
		g_audioManager->GetRoomManager().RemoveShape( *this );
	}
	else
	{
		m_hasTransform = false;
	}
}

bool AudRoom::OnModified( Be::Var* value )
{
	// Any authored attribute changed (e.g. edited in Graphite); re-send the room with the new parameters.
	Sync();
	return true;
}
