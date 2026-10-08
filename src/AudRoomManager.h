////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

#include <CcpMutex.h>

#include <unordered_map>

class AudManager;
class AudRoom;

/**
 * @brief Owns the Wwise side of every AudRoom: the shared room geometry, room registration and lifecycle.
 *
 * Every room is a scaled instance of one shared unit cube geometry set. The instance is marked as
 * containment-only (not used for reflection or diffraction) so it costs nothing in the ray tracer;
 * it gives Wwise the room's bounding box, shows the room in the Game Object 3D Viewer, and lets
 * Wwise place game objects in the room.
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

	/// Removes every room from Wwise but keeps them known so they can be re-sent. For Disable() and the geometry switch turning off.
	void RemoveAllFromWwise();
	/// Re-sends every enabled, placed room. For Enable() and the geometry switch turning on.
	void ResendAll();
	/// Forgets what Wwise knows without talking to it. For after the sound engine has been terminated.
	void ForgetWwiseState();

	size_t GetRoomCount() const;

private:
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

	AudManager* m_audioManager;
	std::unordered_map<AkUInt64, AudRoom*> m_rooms;
	bool m_cubeSent;
	bool m_outdoorConfigured;
	mutable CcpMutex m_mutex;
};
