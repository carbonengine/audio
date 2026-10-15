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
#include <cmath>
#include <string>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudRoomManager" );

namespace
{
	/// Updates in a row a portal can have the same room on both sides before it is reported.
	constexpr int PORTAL_SAME_ROOM_WARNING_TICKS = 30;
	/// Updates in a row new rooms must be found at a portal's faces before they replace the rooms in Wwise.
	constexpr int PORTAL_ROOM_CHANGE_TICKS = 3;
	/// Smallest adjacent room bleed sent to Wwise, which needs it above zero.
	constexpr float MIN_ADJACENT_ROOM_BLEED = 1e-3f;

	/// Compares positions with a 1 mm tolerance.
	bool PositionsNearlyEqual( const AkVector64& a, const AkVector64& b )
	{
		constexpr AkReal64 epsilon = 1e-3;
		return std::fabs( a.X - b.X ) <= epsilon && std::fabs( a.Y - b.Y ) <= epsilon && std::fabs( a.Z - b.Z ) <= epsilon;
	}

	/// Compares orientations with a small tolerance for float errors.
	bool OrientationsNearlyEqual( const AkVector& a, const AkVector& b )
	{
		constexpr float epsilon = 1e-4f;
		return std::fabs( a.X - b.X ) <= epsilon && std::fabs( a.Y - b.Y ) <= epsilon && std::fabs( a.Z - b.Z ) <= epsilon;
	}

	/// Compares every parameter SetPortal uses.
	bool PortalParamsEqual( const AkPortalParams& a, const AkPortalParams& b )
	{
		return PositionsNearlyEqual( a.Transform.Position(), b.Transform.Position() )
			&& OrientationsNearlyEqual( a.Transform.OrientationFront(), b.Transform.OrientationFront() )
			&& OrientationsNearlyEqual( a.Transform.OrientationTop(), b.Transform.OrientationTop() )
			&& a.Extent.halfWidth == b.Extent.halfWidth
			&& a.Extent.halfHeight == b.Extent.halfHeight
			&& a.Extent.halfDepth == b.Extent.halfDepth
			&& a.FrontRoom == b.FrontRoom
			&& a.BackRoom == b.BackRoom
			&& a.AdjacentRoomBleed == b.AdjacentRoomBleed
			&& a.bEnabled == b.bEnabled;
	}
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

	const AkUInt64 foundFrontRoomID = ResolveRoomLocked( box.ToWorld( Vector3( 0.0f, 0.0f, -0.5f ) ) );
	const AkUInt64 foundBackRoomID = ResolveRoomLocked( box.ToWorld( Vector3( 0.0f, 0.0f, 0.5f ) ) );
	if( foundFrontRoomID == portal.m_foundFrontRoomID && foundBackRoomID == portal.m_foundBackRoomID )
	{
		portal.m_foundTicks = std::min( portal.m_foundTicks + 1, PORTAL_SAME_ROOM_WARNING_TICKS + 1 );
	}
	else
	{
		portal.m_foundFrontRoomID = foundFrontRoomID;
		portal.m_foundBackRoomID = foundBackRoomID;
		portal.m_foundTicks = 1;
	}

	const bool keepSentRooms = portal.m_sentToWwise && portal.m_foundTicks < PORTAL_ROOM_CHANGE_TICKS;
	const AkUInt64 frontRoomID = keepSentRooms ? portal.m_frontRoomID : foundFrontRoomID;
	const AkUInt64 backRoomID = keepSentRooms ? portal.m_backRoomID : foundBackRoomID;

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
	const bool portalChanged = !portal.m_sentToWwise
		|| !PortalParamsEqual( params, portal.m_sentPortalParams )
		|| name != portal.m_sentName;

	if( portalChanged )
	{
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

		portal.m_sentPortalParams = params;
		portal.m_sentName = name;
	}

	if( !portal.m_sentToWwise || portal.m_frontRoomID != frontRoomID || portal.m_backRoomID != backRoomID )
	{
		CCP_LOG_CH( s_ch, "Portal '%s' (%llu) sent to Wwise with front room %llu and back room %llu.",
			name.c_str(), portal.GetPortalID(), frontRoomID, backRoomID );
	}

	portal.m_sentToWwise = true;
	portal.m_frontRoomID = frontRoomID;
	portal.m_backRoomID = backRoomID;

	if( SendPortalObstructionOcclusionLocked( portal ) )
	{
		portal.m_failureLogged = false;
	}
}

bool AudRoomManager::SendPortalObstructionOcclusionLocked( AudPortal& portal )
{
	const float obstruction = std::clamp( portal.GetObstruction(), 0.0f, 1.0f );
	const float occlusion = std::clamp( portal.GetOcclusion(), 0.0f, 1.0f );
	const bool transition = portal.GetObstructionOcclusionTransition();

	if( portal.m_obstructionOcclusionSent
		&& obstruction == portal.m_sentObstruction
		&& occlusion == portal.m_sentOcclusion
		&& transition == portal.m_sentTransition )
	{
		return true;
	}

	const AKRESULT result = AK::SpatialAudio::SetPortalObstructionAndOcclusion( portal.GetPortalID(), obstruction, occlusion, transition );
	if( result != AK_Success )
	{
		if( !portal.m_failureLogged )
		{
			CCP_LOGWARN_CH( s_ch, "Failed to set obstruction and occlusion on portal '%s' (%llu), AKRESULT: %d",
				portal.GetName().c_str(), portal.GetPortalID(), result );
			portal.m_failureLogged = true;
		}
		return false;
	}

	portal.m_obstructionOcclusionSent = true;
	portal.m_sentObstruction = obstruction;
	portal.m_sentOcclusion = occlusion;
	portal.m_sentTransition = transition;
	return true;
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
	portal.m_obstructionOcclusionSent = false;

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

void AudRoomManager::RemovePortalsConnectedToLocked( AkUInt64 roomID )
{
	for( auto& entry : m_portals )
	{
		AudPortal& portal = *entry.second;
		if( portal.m_frontRoomID == roomID || portal.m_backRoomID == roomID )
		{
			RemovePortalLocked( portal );
		}
	}
}
