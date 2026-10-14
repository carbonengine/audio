// Copyright © 2026 CCP ehf.

#pragma once

#include <BlueExposure.h>
#include <Blue.h>
#include <ITr2AudEmitter.h>
#include <ITr2BoxVolume.h>
#include <CarbonAudioHost.h>

#include <cstdint>
#include <string>
#include <vector>

namespace AudioLab
{
	/// Everything lives on the ground plane: x to the right, z down the screen (right-handed, y up).
	struct LabListener
	{
		float x = 0.0f;
		float z = 0.0f;
		/// Facing, clockwise from up the screen (-z).
		float yawDegrees = 0.0f;
	};

	struct LabEmitter
	{
		int id = 0;
		std::string name;
		float x = 0.0f;
		float z = 0.0f;
		std::string eventName;
		uint32_t lastPlayingID = 0;
		uint64_t gameObjectID = 0;
		/// What the lab last sent through ITr2AudEmitter, which has no getters.
		float scalingFactor = 1.0f;
		bool muted = false;
		BluePtr<ITr2AudEmitter> object;
	};

	/// A straight wall that blocks sightlines, for occlusion. The lab's own: CarbonAudio only sees which sightlines it
	/// blocks, through the lab's answer to its sightline query (LabSightlines.h).
	struct LabWall
	{
		int id = 0;
		std::string name;
		float x0 = 0.0f;
		float z0 = 0.0f;
		float x1 = 0.0f;
		float z1 = 0.0f;
	};

	struct LabRoom
	{
		int id = 0;
		float x = 0.0f;
		float z = 0.0f;
		float width = 10.0f;
		float depth = 10.0f;
		float height = 6.0f;
		/// Rotation around y.
		float yawDegrees = 0.0f;
		CarbonAudioHost::RoomSettings settings;
		BluePtr<ITr2BoxVolume> object;
	};

	/// An opening between two rooms, or a room and outdoors, that sound passes through, e.g. a door.
	struct LabPortal
	{
		int id = 0;
		float x = 0.0f;
		float z = 0.0f;
		/// Across the opening.
		float width = 2.0f;
		float height = 3.0f;
		/// Through the wall it sits in.
		float depth = 1.0f;
		/// Rotation around y. At 0 the front room is up the screen (-z).
		float yawDegrees = 0.0f;
		CarbonAudioHost::PortalSettings settings;
		BluePtr<ITr2BoxVolume> object;
	};

	/// What a scene ID names.
	enum class LabKind
	{
		None,
		Listener,
		Emitter,
		Room,
		Portal,
		Wall,
	};

	/// The lab's scene. Emitters, rooms and portals are CarbonAudio's own Blue objects, created and placed the way trinity
	/// does (AudEmitter through ITr2AudEmitter, AudRoom and AudPortal through ITr2BoxVolume); the rest goes through
	/// CarbonAudioHost.
	/// Every thing has an ID unique across kinds and never reused: the listener's is LISTENER_ID, the rest count up.
	class LabScene
	{
	public:
		static constexpr int LISTENER_ID = 0;
		/// No thing has it.
		static constexpr int NO_ID = -1;

		LabListener listener;
		std::vector<LabEmitter> emitters;
		std::vector<LabRoom> rooms;
		std::vector<LabPortal> portals;
		std::vector<LabWall> walls;

		LabEmitter* AddEmitter( float x, float z, const std::string& eventName );
		void RemoveEmitter( int id );
		void Play( LabEmitter& emitter );
		void Stop( LabEmitter& emitter );

		/// reverbAuxBus: the new room's reverb, e.g. the first aux bus of the metadata; empty for none.
		LabRoom* AddRoom( float x, float z, float width, float depth, const std::string& reverbAuxBus );
		void RemoveRoom( int id );
		/// After editing a room's settings.
		void ApplySettings( LabRoom& room );
		/// After moving or resizing a room.
		void ApplyShape( LabRoom& room );

		LabPortal* AddPortal( float x, float z, float yawDegrees );
		/// Adds a portal across the room wall nearest the point, within reach metres, whichever way the wall runs, its
		/// front into the room; where the point is and facing up the screen when no wall is that near.
		LabPortal* AddPortalOnWall( float x, float z, float reach );
		void RemovePortal( int id );
		/// After editing a portal's settings.
		void ApplySettings( LabPortal& portal );
		/// After moving, turning or resizing a portal.
		void ApplyShape( LabPortal& portal );

		LabWall* AddWall( float x0, float z0, float x1, float z1 );
		void RemoveWall( int id );
		/// Whether the line from a to b crosses a wall.
		bool IsBlocked( float ax, float az, float bx, float bz ) const;

		/// Removes every emitter, room, portal and wall and puts the listener back at the origin.
		void Clear();

		LabEmitter* FindEmitter( int id );
		LabRoom* FindRoom( int id );
		LabPortal* FindPortal( int id );
		LabWall* FindWall( int id );
		const LabEmitter* FindEmitter( int id ) const;
		const LabRoom* FindRoom( int id ) const;
		const LabPortal* FindPortal( int id ) const;
		const LabWall* FindWall( int id ) const;
		/// LabKind::None once the thing is gone.
		LabKind KindOf( int id ) const;
		/// E.g. "Emitter 3" or the room's name.
		std::string NameOf( int id ) const;
		/// Moves the thing by the offset on the ground plane.
		void Move( int id, float dx, float dz );
		/// Removes an emitter, room, portal or wall; the listener stays.
		void Remove( int id );
		/// The lab room with the given Wwise room ID, or null.
		const LabRoom* FindRoomByWwiseID( uint64_t roomID ) const;

		/// Every frame: re-sends the listener and every emitter (room assignment follows position reports, as in the
		/// game), then ticks CarbonAudio.
		void Update();

	private:
		int m_nextID = 1;
	};

	/// The right-handed unit-box-to-world matrix ITr2BoxVolume takes (row vectors: scale, yaw, translation).
	Matrix RoomTransform( const LabRoom& room );
	Matrix PortalTransform( const LabPortal& portal );
}
