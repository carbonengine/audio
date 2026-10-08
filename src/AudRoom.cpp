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
	m_shapeValid( false ),
	m_warnedDegenerate( false ),
	m_hasTransform( false ),
	m_sentToWwise( false ),
	m_registeredWithManager( false )
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
	m_unitBoxToWorld = unitBoxToWorld;
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

	// Sends the room, or removes it when the new box is degenerate.
	Sync();
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
	m_hasTransform = false;
	Sync();
}

bool AudRoom::OnModified( Be::Var* value )
{
	// Any authored attribute changed (e.g. edited in Graphite); re-send the room with the new parameters.
	Sync();
	return true;
}
