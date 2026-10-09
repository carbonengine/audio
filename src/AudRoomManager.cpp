////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "AudRoomManager.h"

#include "AudManager.h"
#include "AudRoom.h"
#include "AudStaticDataRepository.h"
#include "Utilities.h"

#include <algorithm>
#include <cmath>
#include <iterator>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudRoomManager" );

namespace
{
	/// Unit cube [-0.5, 0.5]^3, the same in right- and left-handed space.
	const AkVertex CUBE_VERTICES[8] =
	{
		AkVertex( -0.5f, -0.5f, -0.5f ),
		AkVertex(  0.5f, -0.5f, -0.5f ),
		AkVertex(  0.5f,  0.5f, -0.5f ),
		AkVertex( -0.5f,  0.5f, -0.5f ),
		AkVertex( -0.5f, -0.5f,  0.5f ),
		AkVertex(  0.5f, -0.5f,  0.5f ),
		AkVertex(  0.5f,  0.5f,  0.5f ),
		AkVertex( -0.5f,  0.5f,  0.5f ),
	};

	const AkTriangle CUBE_TRIANGLES[12] =
	{
		AkTriangle( 0, 1, 2, 0 ), AkTriangle( 0, 2, 3, 0 ),
		AkTriangle( 4, 6, 5, 0 ), AkTriangle( 4, 7, 6, 0 ),
		AkTriangle( 0, 5, 1, 0 ), AkTriangle( 0, 4, 5, 0 ),
		AkTriangle( 3, 2, 6, 0 ), AkTriangle( 3, 6, 7, 0 ),
		AkTriangle( 0, 3, 7, 0 ), AkTriangle( 0, 7, 4, 0 ),
		AkTriangle( 1, 5, 6, 0 ), AkTriangle( 1, 6, 2, 0 ),
	};
}

namespace
{
	const AkUInt64 OUTDOOR_ROOM_ID = static_cast<AkUInt64>( AK::SpatialAudio::kOutdoorRoomID );

	/// Compares orientations with a small tolerance for float errors.
	bool OrientationsNearlyEqual( const AkVector& a, const AkVector& b )
	{
		constexpr float epsilon = 1e-4f;
		return std::fabs( a.X - b.X ) <= epsilon && std::fabs( a.Y - b.Y ) <= epsilon && std::fabs( a.Z - b.Z ) <= epsilon;
	}

	/// Compares every parameter SetRoom uses.
	bool RoomParamsEqual( const AkRoomParams& a, const AkRoomParams& b )
	{
		return OrientationsNearlyEqual( a.Front, b.Front )
			&& OrientationsNearlyEqual( a.Up, b.Up )
			&& a.ReverbAuxBus == b.ReverbAuxBus
			&& a.ReverbLevel == b.ReverbLevel
			&& a.TransmissionLoss == b.TransmissionLoss
			&& a.RoomGameObj_AuxSendLevelToSelf == b.RoomGameObj_AuxSendLevelToSelf
			&& a.GeometryInstanceID == b.GeometryInstanceID
			&& a.RoomPriority == b.RoomPriority
			&& a.DistanceBehavior == b.DistanceBehavior
			&& a.RoomGameObj_KeepRegistered == b.RoomGameObj_KeepRegistered;
	}
}

AudRoomManager::AudRoomManager( AudManager* audioManager ) :
	m_audioManager( audioManager ),
	m_cubeSent( false ),
	m_outdoorConfigured( false ),
	m_assignmentsDirty( false ),
	m_roomsInWwise( 0 ),
	m_pendingRoomTones( 0 ),
	m_mutex( "AudRoomManager", "m_mutex" )
{
}

AudRoomManager::~AudRoomManager()
{
}

AkUInt64 AudRoomManager::GeometryInstanceIDForRoom( const AudRoom& room )
{
	return room.GetRoomID() | ROOM_SPATIAL_ID_TAG;
}

void AudRoomManager::RegisterRoom( AudRoom* room )
{
	CcpAutoMutex lock( m_mutex );
	m_rooms[room->GetRoomID()] = room;
}

void AudRoomManager::UnregisterRoom( AudRoom* room )
{
	CcpAutoMutex lock( m_mutex );
	RemoveLocked( *room );
	m_rooms.erase( room->GetRoomID() );
}

size_t AudRoomManager::GetRoomCount() const
{
	CcpAutoMutex lock( m_mutex );
	return m_rooms.size();
}

size_t AudRoomManager::GetTrackedGameObjectCount() const
{
	CcpAutoMutex lock( m_mutex );
	return m_trackedObjects.size();
}

void AudRoomManager::UpdateGameObjectPosition( AkGameObjectID gameObjectID, const Vector3& position )
{
	if( m_roomsInWwise.load( std::memory_order_acquire ) == 0 )
	{
		return;
	}

	CcpAutoMutex lock( m_mutex );
	if( m_roomsInWwise.load( std::memory_order_relaxed ) == 0 )
	{
		return;
	}

	TrackedGameObject& tracked = m_trackedObjects[gameObjectID];
	tracked.position = position;
	AssignLocked( gameObjectID, tracked );
}

void AudRoomManager::ForgetGameObject( AkGameObjectID gameObjectID )
{
	CcpAutoMutex lock( m_mutex );
	m_trackedObjects.erase( gameObjectID );
}

void AudRoomManager::Update()
{
	CcpAutoMutex lock( m_mutex );
	if( m_audioManager == nullptr || !m_audioManager->AreRoomsReady() )
	{
		return;
	}

	if( m_pendingRoomTones > 0 )
	{
		for( auto& entry : m_rooms )
		{
			AudRoom& room = *entry.second;
			if( room.m_roomTonePending )
			{
				const std::wstring eventName = room.m_postedRoomTone;
				PostRoomToneLocked( room, eventName );
			}
		}
	}

	if( !m_assignmentsDirty )
	{
		return;
	}

	m_assignmentsDirty = false;
	for( auto& entry : m_trackedObjects )
	{
		AssignLocked( entry.first, entry.second );
	}
}

AkUInt64 AudRoomManager::ResolveRoomLocked( const Vector3& position ) const
{
	const AudRoom* best = nullptr;
	for( const auto& entry : m_rooms )
	{
		const AudRoom* room = entry.second;
		if( !room->m_sentToWwise || !room->ContainsPoint( position ) )
		{
			continue;
		}

		const float priority = room->m_sentRoomParams.RoomPriority;
		const float bestPriority = best != nullptr ? best->m_sentRoomParams.RoomPriority : 0.0f;
		if( best == nullptr
			|| priority > bestPriority
			|| ( priority == bestPriority && room->GetVolume() < best->GetVolume() ) )
		{
			best = room;
		}
	}

	return best != nullptr ? best->GetRoomID() : OUTDOOR_ROOM_ID;
}

void AudRoomManager::AssignLocked( AkGameObjectID gameObjectID, TrackedGameObject& tracked )
{
	if( m_audioManager == nullptr || !m_audioManager->AreRoomsReady() )
	{
		return;
	}

	const AkUInt64 roomID = ResolveRoomLocked( tracked.position );

	if( tracked.assigned && tracked.roomID == roomID )
	{
		return;
	}

	if( !tracked.assigned && roomID == OUTDOOR_ROOM_ID )
	{
		return;
	}

	const AKRESULT result = AK::SpatialAudio::SetGameObjectInRoom( gameObjectID, AkRoomID( roomID ) );
	if( result != AK_Success )
	{
		CCP_LOGWARN_CH( s_ch, "Failed to set room %llu on game object %llu, AKRESULT: %d", roomID, gameObjectID, result );
		return;
	}

	tracked.assigned = true;
	tracked.roomID = roomID;
}

bool AudRoomManager::EnsureSharedCubeGeometry()
{
	if( m_cubeSent )
	{
		return true;
	}

	AkAcousticSurface surface;
	surface.strName = "AudRoomCube";
	surface.textureID = AK_INVALID_UNIQUE_ID;
	surface.transmissionLoss = 1.0f;

	AkGeometryParams params;
	params.Vertices = const_cast<AkVertex*>( CUBE_VERTICES );
	params.NumVertices = static_cast<AkVertIdx>( std::size( CUBE_VERTICES ) );
	params.Triangles = const_cast<AkTriangle*>( CUBE_TRIANGLES );
	params.NumTriangles = static_cast<AkTriIdx>( std::size( CUBE_TRIANGLES ) );
	params.Surfaces = &surface;
	params.NumSurfaces = 1;
	params.EnableDiffraction = false;
	params.EnableDiffractionOnBoundaryEdges = false;

	AKRESULT result = AK::SpatialAudio::SetGeometry( SHARED_CUBE_GEOMETRY_SET_ID, params );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set the shared room geometry, AKRESULT: %d", result );
		return false;
	}

	m_cubeSent = true;
	return true;
}

void AudRoomManager::ReleaseSharedCubeGeometry()
{
	if( !m_cubeSent )
	{
		return;
	}

	AK::SpatialAudio::RemoveGeometry( SHARED_CUBE_GEOMETRY_SET_ID );
	m_cubeSent = false;
}

bool AudRoomManager::EnsureOutdoorRoomConfigured()
{
	if( m_outdoorConfigured )
	{
		return true;
	}

	AkRoomParams params;
	params.TransmissionLoss = 0.0f;
	params.ReverbAuxBus = AK_INVALID_AUX_ID;
	params.RoomGameObj_KeepRegistered = false;
	params.GeometryInstanceID = AkGeometryInstanceID();

	AKRESULT result = AK::SpatialAudio::SetRoom( AK::SpatialAudio::kOutdoorRoomID, params, "Outdoors" );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to configure the outdoor room, AKRESULT: %d", result );
		return false;
	}

	m_outdoorConfigured = true;
	return true;
}

void AudRoomManager::PushLocked( AudRoom& room )
{
	if( !room.IsEnabled() || !room.HasTransform() || !room.HasUsableShape() )
	{
		RemoveLocked( room );
		return;
	}

	if( m_audioManager == nullptr || !m_audioManager->AreRoomsReady() )
	{
		return;
	}

	if( !EnsureSharedCubeGeometry() || !EnsureOutdoorRoomConfigured() )
	{
		return;
	}

	const AkUInt64 instanceID = GeometryInstanceIDForRoom( room );

	AkTransform transform;
	RH2LH::convertTransform( room.GetUnitBoxToWorld(), transform );

	AkGeometryInstanceParams instanceParams;
	instanceParams.PositionAndOrientation = transform;
	instanceParams.Scale = RH2LH::extractScale( room.GetUnitBoxToWorld() );
	instanceParams.GeometrySetID = SHARED_CUBE_GEOMETRY_SET_ID;
	instanceParams.UseForReflectionAndDiffraction = false;
	instanceParams.BypassPortalSubtraction = false;
	instanceParams.IsSolid = false;

	AKRESULT result = AK::SpatialAudio::SetGeometryInstance( instanceID, instanceParams );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set geometry instance for room '%s' (%llu), AKRESULT: %d",
			room.GetName().c_str(), room.GetRoomID(), result );
		return;
	}

	const std::wstring roomTone = StringUtils::trim( room.GetRoomToneEvent() );
	if( room.m_postedRoomTone != roomTone )
	{
		StopRoomToneLocked( room );
	}

	AkRoomParams roomParams;
	roomParams.Front = transform.OrientationFront();
	roomParams.Up = transform.OrientationTop();
	roomParams.ReverbAuxBus = room.GetReverbAuxBus().empty()
		? AK_INVALID_AUX_ID
		: AK::SoundEngine::GetIDFromString( room.GetReverbAuxBus().c_str() );
	roomParams.ReverbLevel = std::clamp( room.GetReverbLevel(), 0.0f, 1.0f );
	roomParams.TransmissionLoss = std::clamp( room.GetTransmissionLoss(), 0.0f, 1.0f );
	roomParams.RoomGameObj_AuxSendLevelToSelf = std::clamp( room.GetAuxSendLevelToSelf(), 0.0f, 1.0f );
	roomParams.RoomGameObj_KeepRegistered = room.GetKeepRegistered() || !roomTone.empty();
	roomParams.GeometryInstanceID = instanceID;
	roomParams.RoomPriority = room.GetPriority();

	const bool firstSend = !room.m_sentToWwise;
	const bool roomChanged = firstSend
		|| !RoomParamsEqual( roomParams, room.m_sentRoomParams )
		|| room.GetName() != room.m_sentName;

	if( roomChanged )
	{
		result = AK::SpatialAudio::SetRoom( room.GetRoomID(), roomParams, room.GetName().c_str() );
		if( result != AK_Success )
		{
			CCP_LOGERR_CH( s_ch, "Failed to set room '%s' (%llu), AKRESULT: %d",
				room.GetName().c_str(), room.GetRoomID(), result );
			if( firstSend )
			{
				AK::SpatialAudio::RemoveGeometryInstance( instanceID );
			}
			return;
		}

		room.m_sentRoomParams = roomParams;
		room.m_sentName = room.GetName();
	}

	if( firstSend )
	{
		m_roomsInWwise.fetch_add( 1, std::memory_order_release );

		if( room.GetReverbAuxBus().empty() )
		{
			CCP_LOG_CH( s_ch, "Room '%s' (%llu) sent to Wwise without a reverb aux bus.", room.GetName().c_str(), room.GetRoomID() );
		}
		else
		{
			CCP_LOG_CH( s_ch, "Room '%s' (%llu) sent to Wwise, reverb aux bus '%s' -> %u.",
				room.GetName().c_str(), room.GetRoomID(), room.GetReverbAuxBus().c_str(), roomParams.ReverbAuxBus );
		}
	}

	if( !roomTone.empty() && room.m_postedRoomTone != roomTone )
	{
		PostRoomToneLocked( room, roomTone );
	}

	room.m_sentToWwise = true;
	m_assignmentsDirty = true;
}

void AudRoomManager::RemoveLocked( AudRoom& room )
{
	if( !room.m_sentToWwise )
	{
		return;
	}

	StopRoomToneLocked( room );

	room.m_sentToWwise = false;
	m_assignmentsDirty = true;
	const bool lastRoom = m_roomsInWwise.fetch_sub( 1, std::memory_order_acq_rel ) == 1;
	const bool soundEngineUp = AK::SoundEngine::IsInitialized();

	if( soundEngineUp )
	{
		for( auto& entry : m_trackedObjects )
		{
			TrackedGameObject& tracked = entry.second;
			if( tracked.assigned && tracked.roomID == room.GetRoomID() )
			{
				AK::SpatialAudio::UnsetGameObjectInRoom( entry.first );
				tracked.assigned = false;
			}
		}

		AK::SpatialAudio::RemoveRoom( room.GetRoomID() );
		AK::SpatialAudio::RemoveGeometryInstance( GeometryInstanceIDForRoom( room ) );
	}

	if( lastRoom )
	{
		if( soundEngineUp )
		{
			for( const auto& entry : m_trackedObjects )
			{
				if( entry.second.assigned )
				{
					AK::SpatialAudio::UnsetGameObjectInRoom( entry.first );
				}
			}
		}
		m_trackedObjects.clear();
	}
}

void AudRoomManager::StopRoomToneLocked( AudRoom& room )
{
	if( room.m_roomTonePlayingID != AK_INVALID_PLAYING_ID && AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::StopPlayingID( room.m_roomTonePlayingID );
	}
	room.m_roomTonePlayingID = AK_INVALID_PLAYING_ID;

	if( room.m_roomTonePending )
	{
		room.m_roomTonePending = false;
		--m_pendingRoomTones;
	}
	room.m_postedRoomTone.clear();
}

void AudRoomManager::PostRoomToneLocked( AudRoom& room, const std::wstring& eventName )
{
	room.m_postedRoomTone = eventName;

	if( g_staticDataRepository == nullptr )
	{
		CCP_LOGERR_CH( s_ch, "Room '%s' (%llu): room tone %S cannot play without the static data repository.",
			room.GetName().c_str(), room.GetRoomID(), eventName.c_str() );
		return;
	}

	const std::vector<std::wstring>& soundBanks = g_staticDataRepository->SoundBanksRequiredForEvent( eventName );
	if( soundBanks.empty() )
	{
		CCP_LOGERR_CH( s_ch, "Room '%s' (%llu): room tone %S is not in any SoundBank.",
			room.GetName().c_str(), room.GetRoomID(), eventName.c_str() );
		return;
	}

	for( const std::wstring& soundBank : soundBanks )
	{
		const SoundBankStatus status = m_audioManager->GetSoundBankStatus( soundBank );
		if( status != SoundBankStatus::LOADED )
		{
			if( !room.m_roomTonePending )
			{
				room.m_roomTonePending = true;
				++m_pendingRoomTones;
				if( status != SoundBankStatus::LOADING )
				{
					CCP_LOGWARN_CH( s_ch, "Room '%s' (%llu): room tone %S waits for SoundBank %S, which is not loaded.",
						room.GetName().c_str(), room.GetRoomID(), eventName.c_str(), soundBank.c_str() );
				}
			}
			return;
		}
	}

	if( room.m_roomTonePending )
	{
		room.m_roomTonePending = false;
		--m_pendingRoomTones;
	}

	const AkGameObjectID roomGameObjectID = AkRoomID( room.GetRoomID() ).AsGameObjectID();
	const AkUniqueID eventID = g_staticDataRepository->GetEventID( eventName );
	room.m_roomTonePlayingID = AK::SoundEngine::PostEvent( eventID, roomGameObjectID );
	m_audioManager->LogPostEvent( roomGameObjectID, room.m_roomTonePlayingID, eventID, eventName );

	if( room.m_roomTonePlayingID == AK_INVALID_PLAYING_ID )
	{
		CCP_LOGERR_CH( s_ch, "Room '%s' (%llu): room tone %S failed to play even though its SoundBanks are loaded.",
			room.GetName().c_str(), room.GetRoomID(), eventName.c_str() );
	}
}

void AudRoomManager::Push( AudRoom& room )
{
	CcpAutoMutex lock( m_mutex );
	PushLocked( room );
}

void AudRoomManager::Remove( AudRoom& room )
{
	CcpAutoMutex lock( m_mutex );
	RemoveLocked( room );
}

void AudRoomManager::SetTransform( AudRoom& room, const Matrix& unitBoxToWorld )
{
	CcpAutoMutex lock( m_mutex );
	room.ApplyTransform( unitBoxToWorld );
	PushLocked( room );
}

void AudRoomManager::RemoveShape( AudRoom& room )
{
	CcpAutoMutex lock( m_mutex );
	room.m_hasTransform = false;
	RemoveLocked( room );
}

void AudRoomManager::RemoveAllFromWwise()
{
	CcpAutoMutex lock( m_mutex );
	for( auto& entry : m_rooms )
	{
		RemoveLocked( *entry.second );
	}

	if( AK::SoundEngine::IsInitialized() )
	{
		ReleaseSharedCubeGeometry();
	}
	m_cubeSent = false;
}

void AudRoomManager::ResendAll()
{
	CcpAutoMutex lock( m_mutex );
	for( auto& entry : m_rooms )
	{
		PushLocked( *entry.second );
	}
}

void AudRoomManager::ForgetWwiseState()
{
	CcpAutoMutex lock( m_mutex );
	for( auto& entry : m_rooms )
	{
		AudRoom& room = *entry.second;
		room.m_sentToWwise = false;
		room.m_roomTonePlayingID = AK_INVALID_PLAYING_ID;
		room.m_roomTonePending = false;
		room.m_postedRoomTone.clear();
	}
	m_pendingRoomTones = 0;
	m_roomsInWwise.store( 0, std::memory_order_release );
	m_cubeSent = false;
	m_outdoorConfigured = false;
	m_trackedObjects.clear();
	m_assignmentsDirty = false;
}
