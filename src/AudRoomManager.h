////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>
#include "Matrix.h"
#include "Vector3.h"

#include <atomic>
#include <unordered_map>

class AudManager;
class AudRoom;

/**
 * @brief Owns the Wwise side of every AudRoom: the shared room geometry, room registration, containment and lifecycle.
 *
 * Every room is a scaled instance of one shared unit cube geometry set. The instance is marked as
 * containment-only (not used for reflection or diffraction) so it costs nothing in the ray tracer;
 * it gives Wwise the room's bounding box, shows the room in the Game Object 3D Viewer, and lets
 * Wwise place game objects we have not assigned ourselves.
 *
 * Containment is also done explicitly, the way the Wwise UE integration does it: every emitter and
 * the listener report their position here, the manager tests them against the room boxes and calls
 * AK::SpatialAudio::SetGameObjectInRoom when the room changes. Wwise never tells the game which room
 * an object is in, so doing the test ourselves is what lets other systems (obstruction, portals later)
 * know the room too.
 *
 * The manager mirrors the audio engine lifecycle: rooms are removed from Wwise when audio is disabled
 * or Spatial Audio geometry is turned off, and re-sent when it comes back.
 */
class AudRoomManager
{
public:
	explicit AudRoomManager( AudManager* audioManager );
	~AudRoomManager();

	/// Called by AudRoom on construction and destruction.
	void RegisterRoom( AudRoom* room );
	void UnregisterRoom( AudRoom* room );

	/// Creates or updates the room in Wwise from the room's current state.
	void Push( AudRoom& room );
	/// Removes the room from Wwise. The room stays known to the manager.
	void Remove( AudRoom& room );
	/// Stores the room's box under the lock and sends the room. The box is read for containment by emitter
	/// position reports, which also arrive from trinity worker threads.
	void SetTransform( AudRoom& room, const Matrix& unitBoxToWorld );
	/// Forgets the room's box under the lock and removes the room from Wwise. Pairs with ITr2VolumeObject::Remove.
	void RemoveShape( AudRoom& room );

	/// Removes every room from Wwise but keeps them known so they can be re-sent. For Disable() and the geometry switch turning off.
	void RemoveAllFromWwise();
	/// Re-sends every enabled, placed room. For Enable() and the geometry switch turning on.
	void ResendAll();
	/// Forgets what Wwise knows without talking to it. For after the sound engine has been terminated.
	void ForgetWwiseState();

	/// Called whenever a registered game object (emitter or listener) sends a position to Wwise.
	/// Position is right-handed world space, the same space rooms are placed in. Assigns the object
	/// to the room containing it if that changed.
	void UpdateGameObjectPosition( AkGameObjectID gameObjectID, const Vector3& position );
	/// Called when a game object is unregistered from Wwise; Wwise drops its room assignment with it.
	void ForgetGameObject( AkGameObjectID gameObjectID );
	/// Once per audio tick: re-evaluates every tracked object when rooms were added, moved or removed.
	void Update();

	size_t GetRoomCount() const;
	size_t GetTrackedGameObjectCount() const;

private:
	struct TrackedGameObject
	{
		Vector3 position;
		/// Room last sent with SetGameObjectInRoom. Only meaningful when assigned is true.
		AkUInt64 roomID;
		/// Whether our SetGameObjectInRoom override is in place for this object. While false, Wwise's own containment applies.
		bool assigned;

		TrackedGameObject() : position( 0.0f, 0.0f, 0.0f ), roomID( 0 ), assigned( false ) {}
	};

	static constexpr AkUInt64 ROOM_SPATIAL_ID_TAG = 1ull << 62;
	static constexpr AkUInt64 SHARED_CUBE_GEOMETRY_SET_ID = ROOM_SPATIAL_ID_TAG | 1ull;

	/// Geometry instance ID of a room, tagged so it can never collide with trinity's geometry IDs.
	static AkUInt64 GeometryInstanceIDForRoom( const AudRoom& room );

	/// Uploads the shared unit cube once. Caller holds m_mutex.
	bool EnsureSharedCubeGeometry();
	/// Releases the shared unit cube. Only when all rooms leave Wwise, so toggling one room does not re-upload it. Caller holds m_mutex.
	void ReleaseSharedCubeGeometry();
	/// Parameterizes Wwise's built-in outdoor room once: no transmission loss (as in the Wwise SDK samples) and no reverb. Caller holds m_mutex.
	bool EnsureOutdoorRoomConfigured();
	/// Sends one room. Caller holds m_mutex.
	void PushLocked( AudRoom& room );
	/// Removes one room. Caller holds m_mutex.
	void RemoveLocked( AudRoom& room );

	/// Room containing the position, or the outdoor room. Highest priority wins, then the smallest box. Caller holds m_mutex.
	AkUInt64 ResolveRoomLocked( const Vector3& position ) const;
	/// Sends SetGameObjectInRoom for one object if its room changed. Caller holds m_mutex.
	void AssignLocked( AkGameObjectID gameObjectID, TrackedGameObject& tracked );

	AudManager* m_audioManager;
	std::unordered_map<AkUInt64, AudRoom*> m_rooms;
	std::unordered_map<AkGameObjectID, TrackedGameObject> m_trackedObjects;
	bool m_cubeSent;
	bool m_outdoorConfigured;
	/// Set when rooms changed in Wwise, so Update() re-evaluates every tracked object.
	bool m_assignmentsDirty;
	/// Rooms currently in Wwise. Written under m_mutex, read without it so position reports skip all work while there are none.
	std::atomic<size_t> m_roomsInWwise;
	mutable CcpMutex m_mutex;
};
