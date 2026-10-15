////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fenris Creations
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
	m_warnedDegenerate( false ),
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
	if( IsEnabled() && m_box.IsUsable() )
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
		g_audioManager->GetRoomManager().SetTransform( *this, unitBoxToWorld );
	}
	else
	{
		ApplyTransform( unitBoxToWorld );
	}
}

void AudRoom::ApplyTransform( const Matrix& unitBoxToWorld )
{
	if( !m_box.SetTransform( unitBoxToWorld ) && !m_warnedDegenerate )
	{
		CCP_LOGWARN( "Room '%s' (%llu) has a box with a zero-length axis; it is not sent to Wwise until it is scaled.", m_name.c_str(), m_roomID );
		m_warnedDegenerate = true;
	}
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
		g_audioManager->GetRoomManager().RemoveShape( *this );
	}
	else
	{
		m_box.ClearTransform();
	}
}

bool AudRoom::OnModified( Be::Var* value )
{
	Sync();
	return true;
}
