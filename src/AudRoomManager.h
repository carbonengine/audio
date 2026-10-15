////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SpatialAudio/Common/AkSpatialAudioTypes.h>

#include <CcpMutex.h>
#include "Matrix.h"
#include "Vector3.h"

#include <atomic>
#include <unordered_map>

class AudManager;
class AudPortal;
class AudRoom;

/**
 * @brief Sends every AudRoom and AudPortal to Wwise and assigns game objects to rooms.
 *
 * Lock order: m_mutex can be held while taking AudManager's SoundBank and action-log locks, never the other way round.
 */
class AudRoomManager
{
public:
	/// Room ID of the Wwise outdoor room.
	static constexpr AkUInt64 OUTDOOR_ROOM_ID = AK_OUTDOORS_ROOM_ID;

	explicit AudRoomManager( AudManager* audioManager );
	~AudRoomManager();

	/// Called by AudRoom on construction and destruction.
	void RegisterRoom( AudRoom* room );
	void UnregisterRoom( AudRoom* room );

	/// Creates or updates the room in Wwise.
	void Push( AudRoom& room );
	/// Removes the room from Wwise.
	void Remove( AudRoom& room );
	/// Stores the room's box under the lock and sends the room.
	void SetTransform( AudRoom& room, const Matrix& unitBoxToWorld );
	/// Clears the room's box and removes the room from Wwise.
	void RemoveShape( AudRoom& room );

	/// Called by AudPortal on construction and destruction.
	void RegisterPortal( AudPortal* portal );
	void UnregisterPortal( AudPortal* portal );

	/// Stores the portal's box under the lock. The portal is sent on the next Update().
	void SetPortalTransform( AudPortal& portal, const Matrix& unitBoxToWorld );
	/// Clears the portal's box and removes the portal from Wwise.
	void RemovePortalShape( AudPortal& portal );

	/// Removes every portal and room from Wwise, for Disable() and the rooms switch.
	void RemoveAllFromWwise();
	/// Sends every enabled room to Wwise, for Enable() and the rooms switch.
	void ResendAll();
	/// Resets the Wwise state after the sound engine is terminated.
	void ForgetWwiseState();

	/// Assigns the game object to the room it is in. Position is in right-handed world space.
	void UpdateGameObjectPosition( AkGameObjectID gameObjectID, const Vector3& position );
	/// Removes a game object that was unregistered from Wwise.
	void ForgetGameObject( AkGameObjectID gameObjectID );
	/// Sends portals, assigns game objects again when rooms changed and retries room tones waiting for SoundBanks.
	void Update();

	size_t GetRoomCount() const;
	size_t GetTrackedGameObjectCount() const;

private:
	struct TrackedGameObject
	{
		Vector3 position;
		/// Room last sent with SetGameObjectInRoom.
		AkUInt64 roomID;
		/// Whether SetGameObjectInRoom was called for this object.
		bool assigned;

		TrackedGameObject() : position( 0.0f, 0.0f, 0.0f ), roomID( 0 ), assigned( false ) {}
	};

	static constexpr AkUInt64 ROOM_SPATIAL_ID_TAG = 1ull << 62;
	static constexpr AkUInt64 SHARED_CUBE_GEOMETRY_SET_ID = ROOM_SPATIAL_ID_TAG | 1ull;

	/// Geometry instance ID of the room, tagged so it can't collide with trinity's geometry IDs.
	static AkUInt64 GeometryInstanceIDForRoom( const AudRoom& room );

	/// Sends the shared unit cube once. Caller holds m_mutex.
	bool EnsureSharedCubeGeometry();
	/// Removes the shared unit cube. Caller holds m_mutex.
	void ReleaseSharedCubeGeometry();
	/// Sets the outdoor room to no transmission loss and no reverb. Caller holds m_mutex.
	bool EnsureOutdoorRoomConfigured();
	/// Sends one room. Caller holds m_mutex.
	void PushLocked( AudRoom& room );
	/// Removes one room with its room tone and game object assignments. Caller holds m_mutex.
	void RemoveLocked( AudRoom& room );
	/// Stops the room tone. Caller holds m_mutex.
	void StopRoomToneLocked( AudRoom& room );
	/// Plays the room tone, or waits for its SoundBanks. Caller holds m_mutex.
	void PostRoomToneLocked( AudRoom& room, const std::wstring& eventName );

	/// Returns the room containing the position, or the outdoor room. The smaller room wins when priorities are equal. Caller holds m_mutex.
	AkUInt64 ResolveRoomLocked( const Vector3& position ) const;
	/// Calls SetGameObjectInRoom when the object's room changed. Caller holds m_mutex.
	void AssignLocked( AkGameObjectID gameObjectID, TrackedGameObject& tracked );

	/// Updates every portal in Wwise. Caller holds m_mutex.
	void SyncPortalsLocked();
	/// Sends one portal between the rooms at its two faces, or removes it. Caller holds m_mutex.
	void SyncPortalLocked( AudPortal& portal );
	/// Sends the portal's obstruction and occlusion when they changed. Returns false when Wwise refused them. Caller holds m_mutex.
	bool SendPortalObstructionOcclusionLocked( AudPortal& portal );
	/// Removes one portal from Wwise. Caller holds m_mutex.
	void RemovePortalLocked( AudPortal& portal );
	/// Removes every portal from Wwise. Caller holds m_mutex.
	void RemoveAllPortalsLocked();
	/// Removes the portals connected to the room before the room leaves Wwise. Caller holds m_mutex.
	void RemovePortalsConnectedToLocked( AkUInt64 roomID );

	AudManager* m_audioManager;
	std::unordered_map<AkUInt64, AudRoom*> m_rooms;
	std::unordered_map<AkUInt64, AudPortal*> m_portals;
	std::unordered_map<AkGameObjectID, TrackedGameObject> m_trackedObjects;
	bool m_cubeSent;
	bool m_outdoorConfigured;
	/// Set when rooms changed, so Update() assigns game objects again.
	bool m_assignmentsDirty;
	/// Number of rooms in Wwise, read without the lock.
	std::atomic<size_t> m_roomsInWwise;
	/// Room tones waiting for SoundBanks.
	size_t m_pendingRoomTones;
	mutable CcpMutex m_mutex;
};
