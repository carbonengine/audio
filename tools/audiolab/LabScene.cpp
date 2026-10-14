// Copyright © 2026 CCP ehf.

#include "LabScene.h"

#include "Platform.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	constexpr float DEGREES_TO_RADIANS = 3.14159265358979f / 180.0f;

	Vector3 MakeVector( float x, float y, float z )
	{
		Vector3 v;
		v.x = x;
		v.y = y;
		v.z = z;
		return v;
	}

	CarbonAudioHost::Vec3 MakeVec3( float x, float y, float z )
	{
		return CarbonAudioHost::Vec3{ x, y, z };
	}

	/// Which side of the line through a and b the point c is on: positive, negative, or 0 on the line.
	float Side( float ax, float az, float bx, float bz, float cx, float cz )
	{
		return ( bx - ax ) * ( cz - az ) - ( bz - az ) * ( cx - ax );
	}

	bool Opposite( float a, float b )
	{
		return ( a > 0.0f && b < 0.0f ) || ( a < 0.0f && b > 0.0f );
	}

	/// Whether the segments a-b and c-d cross; touching does not count.
	bool SegmentsCross( float ax, float az, float bx, float bz, float cx, float cz, float dx, float dz )
	{
		return Opposite( Side( cx, cz, dx, dz, ax, az ), Side( cx, cz, dx, dz, bx, bz ) ) && Opposite( Side( ax, az, bx, bz, cx, cz ), Side( ax, az, bx, bz, dx, dz ) );
	}
}

namespace AudioLab
{
	Matrix BoxTransform( float x, float z, float width, float height, float depth, float yawDegrees )
	{
		const float yaw = yawDegrees * DEGREES_TO_RADIANS;
		const float c = std::cos( yaw );
		const float s = std::sin( yaw );

		Matrix m = {};
		// Rows are the box axes in world space, scaled by the box size, then the centre.
		m._11 = width * c;   m._12 = 0.0f;    m._13 = width * -s;
		m._21 = 0.0f;        m._22 = height;  m._23 = 0.0f;
		m._31 = depth * s;   m._32 = 0.0f;    m._33 = depth * c;
		m._41 = x;           m._42 = 0.0f;    m._43 = z;
		m._44 = 1.0f;
		return m;
	}

	Matrix RoomTransform( const LabRoom& room )
	{
		return BoxTransform( room.x, room.z, room.width, room.height, room.depth, room.yawDegrees );
	}

	Matrix PortalTransform( const LabPortal& portal )
	{
		return BoxTransform( portal.x, portal.z, portal.width, portal.height, portal.depth, portal.yawDegrees );
	}

	LabEmitter* LabScene::AddEmitter( float x, float z, const std::string& eventName )
	{
		LabEmitter emitter;
		emitter.id = m_nextID++;
		emitter.name = "Emitter " + std::to_string( emitter.id );
		emitter.x = x;
		emitter.z = z;
		emitter.eventName = eventName;

		// As trinity's AudioGameObject does it.
		if( !BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &emitter.object.p ) ) || !emitter.object )
		{
			return nullptr;
		}
		emitter.object->Initialize( emitter.name, L"", MakeVector( x, 0.0f, z ) );
		emitter.gameObjectID = CarbonAudioHost::GetGameObjectID( emitter.object.p );

		emitters.push_back( std::move( emitter ) );
		return &emitters.back();
	}

	void LabScene::RemoveEmitter( int id )
	{
		// Releasing the last reference stops the emitter's sounds and unregisters it.
		emitters.erase( std::remove_if( emitters.begin(), emitters.end(), [id]( const LabEmitter& e ) { return e.id == id; } ), emitters.end() );
	}

	void LabScene::Play( LabEmitter& emitter )
	{
		if( emitter.object && !emitter.eventName.empty() )
		{
			// 0 while the event's SoundBank is still loading.
			emitter.lastPlayingID = emitter.object->SendEvent( ToWide( emitter.eventName ) );
		}
	}

	void LabScene::Stop( LabEmitter& emitter )
	{
		if( emitter.object )
		{
			CarbonAudioHost::StopAll( emitter.object.p );
			emitter.lastPlayingID = 0;
		}
	}

	LabRoom* LabScene::AddRoom( float x, float z, float width, float depth, const std::string& reverbAuxBus )
	{
		LabRoom room;
		room.id = m_nextID++;
		room.x = x;
		room.z = z;
		room.width = width;
		room.depth = depth;
		room.settings.name = "Room " + std::to_string( room.id );
		room.settings.reverbAuxBus = reverbAuxBus;

		if( !BeClasses->CreateInstanceFromName( "AudRoom", BlueInterfaceIID<ITr2BoxVolume>(), reinterpret_cast<void**>( &room.object.p ) ) || !room.object )
		{
			return nullptr;
		}

		rooms.push_back( std::move( room ) );
		LabRoom& added = rooms.back();
		CarbonAudioHost::SetRoomSettings( added.object.p, added.settings );
		ApplyShape( added );
		return &added;
	}

	void LabScene::RemoveRoom( int id )
	{
		const auto it = std::find_if( rooms.begin(), rooms.end(), [id]( const LabRoom& r ) { return r.id == id; } );
		if( it == rooms.end() )
		{
			return;
		}
		if( it->object )
		{
			it->object->Remove();
		}
		rooms.erase( it );
	}

	void LabScene::ApplySettings( LabRoom& room )
	{
		if( room.object )
		{
			CarbonAudioHost::SetRoomSettings( room.object.p, room.settings );
		}
	}

	void LabScene::ApplyShape( LabRoom& room )
	{
		if( room.object )
		{
			room.object->SetTransform( RoomTransform( room ) );
		}
	}

	LabPortal* LabScene::AddPortal( float x, float z, float yawDegrees )
	{
		LabPortal portal;
		portal.id = m_nextID++;
		portal.x = x;
		portal.z = z;
		portal.yawDegrees = yawDegrees;
		portal.settings.name = "Portal " + std::to_string( portal.id );

		if( !BeClasses->CreateInstanceFromName( "AudPortal", BlueInterfaceIID<ITr2BoxVolume>(), reinterpret_cast<void**>( &portal.object.p ) ) || !portal.object )
		{
			return nullptr;
		}

		portals.push_back( std::move( portal ) );
		LabPortal& added = portals.back();
		CarbonAudioHost::SetPortalSettings( added.object.p, added.settings );
		ApplyShape( added );
		return &added;
	}

	LabPortal* LabScene::AddPortalOnWall( float x, float z, float reach )
	{
		float portalX = x;
		float portalZ = z;
		float yawDegrees = 0.0f;
		float nearest = reach;
		for( const LabRoom& room : rooms )
		{
			// The room's corners, from the axes RoomTransform scales and turns.
			const Matrix box = RoomTransform( room );
			const float ux = box._11 * 0.5f, uz = box._13 * 0.5f, vx = box._31 * 0.5f, vz = box._33 * 0.5f;
			const float cornerX[4] = { room.x - ux - vx, room.x + ux - vx, room.x + ux + vx, room.x - ux + vx };
			const float cornerZ[4] = { room.z - uz - vz, room.z + uz - vz, room.z + uz + vz, room.z - uz + vz };
			for( int i = 0; i < 4; ++i )
			{
				const float x0 = cornerX[i], z0 = cornerZ[i];
				const float dx = cornerX[( i + 1 ) % 4] - x0, dz = cornerZ[( i + 1 ) % 4] - z0;
				const float lengthSq = dx * dx + dz * dz;
				if( lengthSq <= 0.0f )
				{
					continue;
				}
				const float t = std::clamp( ( ( x - x0 ) * dx + ( z - z0 ) * dz ) / lengthSq, 0.0f, 1.0f );
				const float onX = x0 + t * dx, onZ = z0 + t * dz;
				const float distance = std::hypot( x - onX, z - onZ );
				if( distance > nearest )
				{
					continue;
				}
				// Across the wall: the wall's normal, towards the room's centre, is the portal's front, which is
				// ( -sin yaw, -cos yaw ) as PortalTransform turns it.
				float nx = -dz, nz = dx;
				if( nx * ( room.x - onX ) + nz * ( room.z - onZ ) < 0.0f )
				{
					nx = -nx;
					nz = -nz;
				}
				portalX = onX;
				portalZ = onZ;
				yawDegrees = std::atan2( -nx, -nz ) / DEGREES_TO_RADIANS;
				nearest = distance;
			}
		}
		return AddPortal( portalX, portalZ, yawDegrees );
	}

	void LabScene::RemovePortal( int id )
	{
		const auto it = std::find_if( portals.begin(), portals.end(), [id]( const LabPortal& p ) { return p.id == id; } );
		if( it == portals.end() )
		{
			return;
		}
		if( it->object )
		{
			it->object->Remove();
		}
		portals.erase( it );
	}

	void LabScene::ApplySettings( LabPortal& portal )
	{
		if( portal.object )
		{
			CarbonAudioHost::SetPortalSettings( portal.object.p, portal.settings );
		}
	}

	void LabScene::ApplyShape( LabPortal& portal )
	{
		if( portal.object )
		{
			portal.object->SetTransform( PortalTransform( portal ) );
		}
	}

	LabWall* LabScene::AddWall( float x0, float z0, float x1, float z1 )
	{
		LabWall wall;
		wall.id = m_nextID++;
		wall.name = "Wall " + std::to_string( wall.id );
		wall.x0 = x0;
		wall.z0 = z0;
		wall.x1 = x1;
		wall.z1 = z1;
		walls.push_back( std::move( wall ) );
		return &walls.back();
	}

	void LabScene::RemoveWall( int id )
	{
		walls.erase( std::remove_if( walls.begin(), walls.end(), [id]( const LabWall& w ) { return w.id == id; } ), walls.end() );
	}

	bool LabScene::IsBlocked( float ax, float az, float bx, float bz ) const
	{
		return std::any_of( walls.begin(), walls.end(), [=]( const LabWall& w ) { return SegmentsCross( ax, az, bx, bz, w.x0, w.z0, w.x1, w.z1 ); } );
	}

	void LabScene::Clear()
	{
		emitters.clear();
		walls.clear();
		for( LabPortal& portal : portals )
		{
			if( portal.object )
			{
				portal.object->Remove();
			}
		}
		portals.clear();
		for( LabRoom& room : rooms )
		{
			if( room.object )
			{
				room.object->Remove();
			}
		}
		rooms.clear();
		listener = LabListener();
	}

	LabEmitter* LabScene::FindEmitter( int id )
	{
		return const_cast<LabEmitter*>( std::as_const( *this ).FindEmitter( id ) );
	}

	LabRoom* LabScene::FindRoom( int id )
	{
		return const_cast<LabRoom*>( std::as_const( *this ).FindRoom( id ) );
	}

	LabPortal* LabScene::FindPortal( int id )
	{
		return const_cast<LabPortal*>( std::as_const( *this ).FindPortal( id ) );
	}

	LabWall* LabScene::FindWall( int id )
	{
		return const_cast<LabWall*>( std::as_const( *this ).FindWall( id ) );
	}

	const LabEmitter* LabScene::FindEmitter( int id ) const
	{
		const auto it = std::find_if( emitters.begin(), emitters.end(), [id]( const LabEmitter& e ) { return e.id == id; } );
		return it != emitters.end() ? &*it : nullptr;
	}

	const LabRoom* LabScene::FindRoom( int id ) const
	{
		const auto it = std::find_if( rooms.begin(), rooms.end(), [id]( const LabRoom& r ) { return r.id == id; } );
		return it != rooms.end() ? &*it : nullptr;
	}

	const LabPortal* LabScene::FindPortal( int id ) const
	{
		const auto it = std::find_if( portals.begin(), portals.end(), [id]( const LabPortal& p ) { return p.id == id; } );
		return it != portals.end() ? &*it : nullptr;
	}

	const LabWall* LabScene::FindWall( int id ) const
	{
		const auto it = std::find_if( walls.begin(), walls.end(), [id]( const LabWall& w ) { return w.id == id; } );
		return it != walls.end() ? &*it : nullptr;
	}

	LabKind LabScene::KindOf( int id ) const
	{
		if( id == LISTENER_ID )
		{
			return LabKind::Listener;
		}
		if( FindEmitter( id ) != nullptr )
		{
			return LabKind::Emitter;
		}
		if( FindRoom( id ) != nullptr )
		{
			return LabKind::Room;
		}
		if( FindPortal( id ) != nullptr )
		{
			return LabKind::Portal;
		}
		if( FindWall( id ) != nullptr )
		{
			return LabKind::Wall;
		}
		return LabKind::None;
	}

	std::string LabScene::NameOf( int id ) const
	{
		if( id == LISTENER_ID )
		{
			return "Listener";
		}
		if( const LabEmitter* emitter = FindEmitter( id ) )
		{
			return emitter->name;
		}
		if( const LabRoom* room = FindRoom( id ) )
		{
			return room->settings.name;
		}
		if( const LabPortal* portal = FindPortal( id ) )
		{
			return portal->settings.name;
		}
		if( const LabWall* wall = FindWall( id ) )
		{
			return wall->name;
		}
		return std::string();
	}

	void LabScene::Move( int id, float dx, float dz )
	{
		if( id == LISTENER_ID )
		{
			listener.x += dx;
			listener.z += dz;
		}
		else if( LabEmitter* emitter = FindEmitter( id ) )
		{
			emitter->x += dx;
			emitter->z += dz;
		}
		else if( LabRoom* room = FindRoom( id ) )
		{
			room->x += dx;
			room->z += dz;
			ApplyShape( *room );
		}
		else if( LabPortal* portal = FindPortal( id ) )
		{
			portal->x += dx;
			portal->z += dz;
			ApplyShape( *portal );
		}
		else if( LabWall* wall = FindWall( id ) )
		{
			wall->x0 += dx;
			wall->z0 += dz;
			wall->x1 += dx;
			wall->z1 += dz;
		}
	}

	void LabScene::Remove( int id )
	{
		RemoveEmitter( id );
		RemoveRoom( id );
		RemovePortal( id );
		RemoveWall( id );
	}

	const LabRoom* LabScene::FindRoomByWwiseID( uint64_t roomID ) const
	{
		for( const LabRoom& room : rooms )
		{
			CarbonAudioHost::RoomStatus status;
			if( room.object && CarbonAudioHost::GetRoomStatus( room.object.p, status ) && status.roomID == roomID )
			{
				return &room;
			}
		}
		return nullptr;
	}

	void LabScene::Update()
	{
		// CarbonAudio's listener takes its front as the camera's back axis (see RH2LH::convertListener), so the facing
		// direction goes in negated.
		const float yaw = listener.yawDegrees * DEGREES_TO_RADIANS;
		const float facingX = std::sin( yaw );
		const float facingZ = -std::cos( yaw );
		CarbonAudioHost::SetListenerPlacement( MakeVec3( -facingX, 0.0f, -facingZ ), MakeVec3( 0.0f, 1.0f, 0.0f ), MakeVec3( listener.x, 0.0f, listener.z ) );

		for( LabEmitter& emitter : emitters )
		{
			if( emitter.object )
			{
				emitter.object->SetPosition( MakeVector( 0.0f, 0.0f, -1.0f ), MakeVector( 0.0f, 1.0f, 0.0f ), MakeVector( emitter.x, 0.0f, emitter.z ) );
			}
		}

		CarbonAudioHost::Tick();
	}
}
