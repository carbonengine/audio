////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 Fenris Creations
//

#include "stdafx.h"
#include "AudPortal.h"

#include "AudManager.h"
#include "AudRoomManager.h"

AudPortal::AudPortal( IRoot* lockobj ) :
	m_portalID( AllocateGameObjectID() ),
	m_open( true ),
	m_adjacentRoomBleed( 1.0f ),
	m_obstruction( 0.0f ),
	m_occlusion( 0.0f ),
	m_obstructionOcclusionTransition( false ),
	m_enabled( true ),
	m_shapeEnabled( true ),
	m_warnedDegenerate( false ),
	m_sentToWwise( false ),
	m_registeredWithManager( false ),
	m_frontRoomID( AudRoomManager::OUTDOOR_ROOM_ID ),
	m_backRoomID( AudRoomManager::OUTDOOR_ROOM_ID ),
	m_foundFrontRoomID( AudRoomManager::OUTDOOR_ROOM_ID ),
	m_foundBackRoomID( AudRoomManager::OUTDOOR_ROOM_ID ),
	m_foundTicks( 0 ),
	m_obstructionOcclusionSent( false ),
	m_sentObstruction( 0.0f ),
	m_sentOcclusion( 0.0f ),
	m_sentTransition( false ),
	m_setPortalFailureLogged( false ),
	m_obstructionOcclusionFailureLogged( false )
{
	EnsureRegistered();
}

AudPortal::~AudPortal()
{
	if( g_audioManager != nullptr && m_registeredWithManager )
	{
		g_audioManager->GetRoomManager().UnregisterPortal( this );
	}
}

bool AudPortal::EnsureRegistered()
{
	if( m_registeredWithManager )
	{
		return true;
	}

	if( g_audioManager == nullptr )
	{
		return false;
	}

	g_audioManager->GetRoomManager().RegisterPortal( this );
	m_registeredWithManager = true;
	return true;
}

void AudPortal::SetTransform( const Matrix& unitBoxToWorld )
{
	if( EnsureRegistered() )
	{
		g_audioManager->GetRoomManager().SetPortalTransform( *this, unitBoxToWorld );
	}
	else
	{
		ApplyTransform( unitBoxToWorld );
	}
}

void AudPortal::ApplyTransform( const Matrix& unitBoxToWorld )
{
	if( !m_box.SetTransform( unitBoxToWorld ) && !m_warnedDegenerate )
	{
		CCP_LOGWARN( "Portal '%s' (%llu) has a box with a zero-length axis; it is not sent to Wwise until it is scaled.", m_name.c_str(), m_portalID );
		m_warnedDegenerate = true;
	}
}

void AudPortal::SetEnabled( bool enabled )
{
	m_shapeEnabled = enabled;
	EnsureRegistered();
}

void AudPortal::Remove()
{
	if( EnsureRegistered() )
	{
		g_audioManager->GetRoomManager().RemovePortalShape( *this );
	}
	else
	{
		m_box.ClearTransform();
	}
}

bool AudPortal::OnModified( Be::Var* value )
{
	EnsureRegistered();
	return true;
}
