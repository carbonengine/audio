////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "AudRoomManager.h"

#include "AudManager.h"
#include "AudPortal.h"
#include "Utilities.h"

#include <algorithm>
#include <string>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudRoomManager" );

namespace
{
	/// Updates in a row a portal can have the same room on both sides before it is reported.
	constexpr int PORTAL_SAME_ROOM_WARNING_TICKS = 30;
	/// Smallest adjacent room bleed sent to Wwise, which needs it above zero.
	constexpr float MIN_ADJACENT_ROOM_BLEED = 1e-3f;
}

void AudRoomManager::RegisterPortal( AudPortal* portal )
{
	CcpAutoMutex lock( m_mutex );
	m_portals[portal->GetPortalID()] = portal;
}

void AudRoomManager::UnregisterPortal( AudPortal* portal )
{
	CcpAutoMutex lock( m_mutex );
	RemovePortalLocked( *portal );
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
	RemovePortalLocked( portal );
}

void AudRoomManager::SyncPortalsLocked()
{
	for( auto& entry : m_portals )
	{
		SyncPortalLocked( *entry.second );
	}
}

void AudRoomManager::SyncPortalLocked( AudPortal& portal )
{
	const AudUnitBox& box = portal.GetBox();
	if( !portal.IsEnabled() || !box.IsUsable() )
	{
		portal.m_foundTicks = 0;
		RemovePortalLocked( portal );
		return;
	}

	const AkUInt64 frontRoomID = ResolveRoomLocked( box.ToWorld( Vector3( 0.0f, 0.0f, -0.5f ) ) );
	const AkUInt64 backRoomID = ResolveRoomLocked( box.ToWorld( Vector3( 0.0f, 0.0f, 0.5f ) ) );
	if( frontRoomID == portal.m_foundFrontRoomID && backRoomID == portal.m_foundBackRoomID )
	{
		portal.m_foundTicks = std::min( portal.m_foundTicks + 1, PORTAL_SAME_ROOM_WARNING_TICKS + 1 );
	}
	else
	{
		portal.m_foundFrontRoomID = frontRoomID;
		portal.m_foundBackRoomID = backRoomID;
		portal.m_foundTicks = 1;
	}

	if( frontRoomID == backRoomID )
	{
		RemovePortalLocked( portal );
		if( portal.m_foundTicks == PORTAL_SAME_ROOM_WARNING_TICKS )
		{
			CCP_LOGWARN_CH( s_ch, "Portal '%s' (%llu) has room %llu on both sides; it is not sent to Wwise until it crosses a room wall.",
				portal.GetName().c_str(), portal.GetPortalID(), frontRoomID );
		}
		return;
	}

	AkTransform transform;
	RH2LH::convertTransform( box.GetUnitBoxToWorld(), transform );
	const AkVector scale = RH2LH::extractScale( box.GetUnitBoxToWorld() );

	AkPortalParams params;
	params.Transform = AK::ConvertAkTransformToAkWorldTransform( transform );
	params.Extent = AkExtent( 0.5f * scale.X, 0.5f * scale.Y, 0.5f * scale.Z );
	params.FrontRoom = AkRoomID( frontRoomID );
	params.BackRoom = AkRoomID( backRoomID );
	params.AdjacentRoomBleed = std::max( portal.GetAdjacentRoomBleed(), MIN_ADJACENT_ROOM_BLEED );
	params.bEnabled = portal.IsOpen();

	const std::string name = portal.GetName().empty() ? "Portal " + std::to_string( portal.GetPortalID() ) : portal.GetName();
	const AKRESULT result = AK::SpatialAudio::SetPortal( portal.GetPortalID(), params, name.c_str() );
	if( result != AK_Success )
	{
		if( !portal.m_failureLogged )
		{
			CCP_LOGERR_CH( s_ch, "Failed to set portal '%s' (%llu), AKRESULT: %d", name.c_str(), portal.GetPortalID(), result );
			portal.m_failureLogged = true;
		}
		return;
	}

	if( !portal.m_sentToWwise || portal.m_frontRoomID != frontRoomID || portal.m_backRoomID != backRoomID )
	{
		CCP_LOG_CH( s_ch, "Portal '%s' (%llu) sent to Wwise with front room %llu and back room %llu.",
			name.c_str(), portal.GetPortalID(), frontRoomID, backRoomID );
	}

	portal.m_sentToWwise = true;
	portal.m_frontRoomID = frontRoomID;
	portal.m_backRoomID = backRoomID;
	portal.m_failureLogged = false;
}

void AudRoomManager::RemovePortalLocked( AudPortal& portal )
{
	if( !portal.m_sentToWwise )
	{
		return;
	}

	portal.m_sentToWwise = false;
	portal.m_frontRoomID = OUTDOOR_ROOM_ID;
	portal.m_backRoomID = OUTDOOR_ROOM_ID;

	if( AK::SoundEngine::IsInitialized() )
	{
		AK::SpatialAudio::RemovePortal( portal.GetPortalID() );
	}
}

void AudRoomManager::RemoveAllPortalsLocked()
{
	for( auto& entry : m_portals )
	{
		RemovePortalLocked( *entry.second );
	}
}
