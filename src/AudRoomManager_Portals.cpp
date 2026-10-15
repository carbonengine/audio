////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "AudRoomManager.h"

#include "AudPortal.h"

void AudRoomManager::RegisterPortal( AudPortal* portal )
{
	CcpAutoMutex lock( m_mutex );
	m_portals[portal->GetPortalID()] = portal;
}

void AudRoomManager::UnregisterPortal( AudPortal* portal )
{
	CcpAutoMutex lock( m_mutex );
	m_portals.erase( portal->GetPortalID() );
}

void AudRoomManager::SetPortalTransform( AudPortal& portal, const Matrix& unitBoxToWorld )
{
	CcpAutoMutex lock( m_mutex );
	portal.ApplyTransform( unitBoxToWorld );
}

void AudRoomManager::RemovePortalShape( AudPortal& portal )
{
	CcpAutoMutex lock( m_mutex );
	portal.m_box.ClearTransform();
}
