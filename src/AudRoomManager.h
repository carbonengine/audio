////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fenris Creations
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

	/**
	 * @brief Called by AudRoom on construction and destruction.
	 *
	 * @param room The room to add or remove.
	 */
	void RegisterRoom( AudRoom* room );
	void UnregisterRoom( AudRoom* room );

	/**
	 * @brief Creates or updates the room in Wwise.
	 *
	 * @param room The room to send.
	 */
	void Push( AudRoom& room );

	/**
	 * @brief Removes the room from Wwise.
	 *
	 * @param room The room to remove.
	 */
	void Remove( AudRoom& room );

	/**
	 * @brief Stores the room's box under the lock and sends the room.
	 *
	 * @param room           The room that moved.
	 * @param unitBoxToWorld Maps the unit box to world space.
	 */
	void SetTransform( AudRoom& room, const Matrix& unitBoxToWorld );

	/**
	 * @brief Clears the room's box and removes the room from Wwise.
	 *
	 * @param room The room whose box is removed.
	 */
	void RemoveShape( AudRoom& room );

	/**
	 * @brief Called by AudPortal on construction and destruction.
	 *
	 * @param portal The portal to add or remove.
	 */
	void RegisterPortal( AudPortal* portal );
	void UnregisterPortal( AudPortal* portal );

	/**
	 * @brief Stores the portal's box under the lock. The portal is sent on the next Update().
	 *
	 * @param portal         The portal that moved.
	 * @param unitBoxToWorld Maps the unit box to world space.
	 */
	void SetPortalTransform( AudPortal& portal, const Matrix& unitBoxToWorld );

	/**
	 * @brief Clears the portal's box and removes the portal from Wwise.
	 *
	 * @param portal The portal whose box is removed.
	 */
	void RemovePortalShape( AudPortal& portal );

	/**
	 * @brief Removes every portal and room from Wwise, for Disable() and the rooms switch.
	 */
	void RemoveAllFromWwise();

	/**
	 * @brief Sends every enabled room to Wwise, for Enable() and the rooms switch.
	 */
	void ResendAll();

	/**
	 * @brief Resets the Wwise state after the sound engine is terminated.
	 */
	void ForgetWwiseState();

	/**
	 * @brief Assigns the game object to the room it is in.
	 *
	 * @param gameObjectID The game object that moved.
	 * @param position     Its position in right-handed world space.
	 */
	void UpdateGameObjectPosition( AkGameObjectID gameObjectID, const Vector3& position );

	/**
	 * @brief Removes a game object that was unregistered from Wwise.
	 *
	 * @param gameObjectID The game object to forget.
	 */
	void ForgetGameObject( AkGameObjectID gameObjectID );

	/**
	 * @brief Sends portals, assigns game objects again when rooms changed and retries room tones waiting for SoundBanks.
	 */
	void Update();

	/**
	 * @brief Number of rooms registered with the manager.
	 */
	size_t GetRoomCount() const;

	/**
	 * @brief Number of game objects whose room is tracked.
	 */
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

	/**
	 * @brief Geometry instance ID of the room, tagged so it can't collide with trinity's geometry IDs.
	 *
	 * @param room The room.
	 *
	 * @return The tagged geometry instance ID.
	 */
	static AkUInt64 GeometryInstanceIDForRoom( const AudRoom& room );

	/**
	 * @brief Sends the shared unit cube once. Caller holds m_mutex.
	 *
	 * @return True when the cube is in Wwise.
	 */
	bool EnsureSharedCubeGeometry();

	/**
	 * @brief Removes the shared unit cube. Caller holds m_mutex.
	 */
	void ReleaseSharedCubeGeometry();

	/**
	 * @brief Sets the outdoor room to no transmission loss and no reverb. Caller holds m_mutex.
	 *
	 * @return True when the outdoor room is set.
	 */
	bool EnsureOutdoorRoomConfigured();

	/**
	 * @brief Sends one room. Caller holds m_mutex.
	 *
	 * @param room The room to send.
	 */
	void PushLocked( AudRoom& room );

	/**
	 * @brief Removes one room with its room tone and game object assignments. Caller holds m_mutex.
	 *
	 * @param room The room to remove.
	 */
	void RemoveLocked( AudRoom& room );

	/**
	 * @brief Stops the room tone. Caller holds m_mutex.
	 *
	 * @param room The room whose tone stops.
	 */
	void StopRoomToneLocked( AudRoom& room );

	/**
	 * @brief Plays the room tone, or waits for its SoundBanks. Caller holds m_mutex.
	 *
	 * @param room      The room that plays the tone.
	 * @param eventName The room tone event.
	 */
	void PostRoomToneLocked( AudRoom& room, const std::wstring& eventName );

	/**
	 * @brief Returns the room containing the position, or the outdoor room. Caller holds m_mutex.
	 *
	 * The smaller room wins when priorities are equal.
	 *
	 * @param position Position in right-handed world space.
	 *
	 * @return The room ID.
	 */
	AkUInt64 ResolveRoomLocked( const Vector3& position ) const;

	/**
	 * @brief Calls SetGameObjectInRoom when the object's room changed. Caller holds m_mutex.
	 *
	 * @param gameObjectID The game object to assign.
	 * @param tracked      Its tracked position and room.
	 */
	void AssignLocked( AkGameObjectID gameObjectID, TrackedGameObject& tracked );

	/**
	 * @brief Updates every portal in Wwise. Caller holds m_mutex.
	 */
	void SyncPortalsLocked();

	/**
	 * @brief Sends one portal between the rooms on its two sides, or removes it. Caller holds m_mutex.
	 *
	 * @param portal The portal to send.
	 */
	void SyncPortalLocked( AudPortal& portal );

	/**
	 * @brief Sends the portal's obstruction and occlusion when they changed. Caller holds m_mutex.
	 *
	 * @param portal The portal to send them for.
	 *
	 * @return False when Wwise refused them.
	 */
	bool SendPortalObstructionOcclusionLocked( AudPortal& portal );

	/**
	 * @brief Removes one portal from Wwise. Caller holds m_mutex.
	 *
	 * @param portal The portal to remove.
	 */
	void RemovePortalLocked( AudPortal& portal );

	/**
	 * @brief Removes every portal from Wwise. Caller holds m_mutex.
	 */
	void RemoveAllPortalsLocked();

	/**
	 * @brief Removes the portals connected to the room before the room leaves Wwise. Caller holds m_mutex.
	 *
	 * @param roomID The room that leaves Wwise.
	 */
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
