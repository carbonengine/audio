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
#include "Utilities.h"

#include <algorithm>
#include <cmath>
#include <iterator>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudRoomManager" );

namespace
{
	// Unit cube [-0.5, 0.5]^3. Defined directly in Wwise space: the cube is symmetric under the
	// right- to left-handed Z flip, and Wwise triangles are double-sided so winding does not matter.
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
		AkTriangle( 0, 1, 2, 0 ), AkTriangle( 0, 2, 3, 0 ), // -Z
		AkTriangle( 4, 6, 5, 0 ), AkTriangle( 4, 7, 6, 0 ), // +Z
		AkTriangle( 0, 5, 1, 0 ), AkTriangle( 0, 4, 5, 0 ), // -Y
		AkTriangle( 3, 2, 6, 0 ), AkTriangle( 3, 6, 7, 0 ), // +Y
		AkTriangle( 0, 3, 7, 0 ), AkTriangle( 0, 7, 4, 0 ), // -X
		AkTriangle( 1, 5, 6, 0 ), AkTriangle( 1, 6, 2, 0 ), // +X
	};
}

namespace
{
	const AkUInt64 OUTDOOR_ROOM_ID = static_cast<AkUInt64>( AK::SpatialAudio::kOutdoorRoomID );

	/// Room orientation is rebuilt from a rotation matrix on every move; ignore float noise (same tolerance as trinity's EveVolumeObject).
	bool OrientationsNearlyEqual( const AkVector& a, const AkVector& b )
	{
		constexpr float epsilon = 1e-4f;
		return std::fabs( a.X - b.X ) <= epsilon && std::fabs( a.Y - b.Y ) <= epsilon && std::fabs( a.Z - b.Z ) <= epsilon;
	}

	/// Field-wise comparison of everything SetRoom consumes, so an unchanged room is not re-sent on every move.
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
	// Every emitter position send lands here; skip the lock entirely while no room is in Wwise.
	if( m_roomsInWwise.load( std::memory_order_acquire ) == 0 )
	{
		return;
	}

	CcpAutoMutex lock( m_mutex );
	// The last room may have left while this thread waited for the lock.
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
		// Keep the flags; the next tick after rooms come back will catch up.
		return;
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

		// Same rule as Wwise's own containment: highest priority, and the inner room on a tie. The priority
		// is the one Wwise holds, so both containments agree while an edited priority is not sent yet.
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
		// Never placed in a room by us and not in one now: leave Wwise's own containment in charge.
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

	// One placeholder surface for the whole cube until rooms get acoustic materials. Containment-only geometry
	// is not ray traced, so the room's own TransmissionLoss applies to direct paths; Wwise still uses room
	// geometry surfaces for the transmission of reverb and room tones through the walls.
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

	// Wwise applies room transmission loss from both the emitter's and the listener's room. Give the
	// outdoor room (space, station exteriors) none, so an interior's own TransmissionLoss is what is heard.
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

	// The room shape: the shared unit cube placed and scaled by the unit box transform.
	AkTransform transform;
	RH2LH::convertTransform( room.GetUnitBoxToWorld(), transform );

	AkGeometryInstanceParams instanceParams;
	instanceParams.PositionAndOrientation = transform;
	instanceParams.Scale = RH2LH::extractScale( room.GetUnitBoxToWorld() );
	instanceParams.GeometrySetID = SHARED_CUBE_GEOMETRY_SET_ID;
	instanceParams.UseForReflectionAndDiffraction = false; // containment only, keeps it out of the ray tracer
	instanceParams.BypassPortalSubtraction = false;
	instanceParams.IsSolid = false;

	AKRESULT result = AK::SpatialAudio::SetGeometryInstance( instanceID, instanceParams );
	if( result != AK_Success )
	{
		CCP_LOGERR_CH( s_ch, "Failed to set geometry instance for room '%s' (%llu), AKRESULT: %d",
			room.GetName().c_str(), room.GetRoomID(), result );
		return;
	}

	AkRoomParams roomParams;
	roomParams.Front = transform.OrientationFront();
	roomParams.Up = transform.OrientationTop();
	roomParams.ReverbAuxBus = room.GetReverbAuxBus().empty()
		? AK_INVALID_AUX_ID
		: AK::SoundEngine::GetIDFromString( room.GetReverbAuxBus().c_str() );
	// Wwise documents these three as valid in [0, 1].
	roomParams.ReverbLevel = std::clamp( room.GetReverbLevel(), 0.0f, 1.0f );
	roomParams.TransmissionLoss = std::clamp( room.GetTransmissionLoss(), 0.0f, 1.0f );
	roomParams.RoomGameObj_AuxSendLevelToSelf = std::clamp( room.GetAuxSendLevelToSelf(), 0.0f, 1.0f );
	roomParams.RoomGameObj_KeepRegistered = room.GetKeepRegistered();
	roomParams.GeometryInstanceID = instanceID;
	roomParams.RoomPriority = room.GetPriority();

	// Only Front/Up depend on the transform, so a pure move or resize only needs the geometry instance above.
	const bool firstSend = !room.m_sentToWwise;
	const bool roomChanged = firstSend
		|| !RoomParamsEqual( roomParams, room.m_sentRoomParams )
		|| room.GetName() != room.m_sentName;

	if( roomChanged )
	{
		// Calling SetRoom again with the same ID updates the room.
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

		// GetIDFromString hashes any name, so a mistyped bus is silent. Log what was resolved for the Profiler.
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

	room.m_sentToWwise = true;
	// The room appeared or moved; objects may have entered or left it.
	m_assignmentsDirty = true;
}

void AudRoomManager::RemoveLocked( AudRoom& room )
{
	if( !room.m_sentToWwise )
	{
		return;
	}

	room.m_sentToWwise = false;
	m_assignmentsDirty = true;
	const bool lastRoom = m_roomsInWwise.fetch_sub( 1, std::memory_order_acq_rel ) == 1;
	const bool soundEngineUp = AK::SoundEngine::IsInitialized();

	if( soundEngineUp )
	{
		// Objects we placed in this room go back to Wwise's own containment before the room disappears,
		// so none is left on a stale override; the next Update() places them again if another room holds them.
		for( auto& entry : m_trackedObjects )
		{
			TrackedGameObject& tracked = entry.second;
			if( tracked.assigned && tracked.roomID == room.GetRoomID() )
			{
				AK::SpatialAudio::UnsetGameObjectInRoom( entry.first );
				tracked.assigned = false;
			}
		}

		// Remove the room before its geometry so nothing in Wwise references the instance. The shared cube stays.
		AK::SpatialAudio::RemoveRoom( room.GetRoomID() );
		AK::SpatialAudio::RemoveGeometryInstance( GeometryInstanceIDForRoom( room ) );
	}

	if( lastRoom )
	{
		// No room left: the remaining overrides (explicit outdoor) go back to Wwise too, and nothing is tracked
		// until a room returns. Objects then fall back to Wwise's containment until their next position report.
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
	// Sends the room, or removes it when the new box is degenerate.
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
	// The outdoor room is Wwise's own, always-present room, so it is only parameterized here, never removed.
	// Its parameters survive until the sound engine terminates, see ForgetWwiseState().
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
		entry.second->m_sentToWwise = false;
	}
	m_roomsInWwise.store( 0, std::memory_order_release );
	m_cubeSent = false;
	m_outdoorConfigured = false;
	// Game objects are gone with the sound engine; they re-report their position when re-registered.
	m_trackedObjects.clear();
	m_assignmentsDirty = false;
}
