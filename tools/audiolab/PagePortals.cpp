// Copyright © 2026 CCP ehf.

#include "LabApp.h"
#include "LabWidgets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

namespace AudioLab
{
	namespace
	{
		/// How far everything moves each frame while the origin moves: more than half a portal's depth, so a portal a
		/// frame behind its rooms finds the wrong room at one of its faces.
		constexpr float ORIGIN_STEP = 0.6f;
		/// How far the walk check moves the listener each frame.
		constexpr float WALK_STEP = 0.1f;

		/// The aux bus with the name when the SoundBanks have it, otherwise the first one, or none.
		std::string Bus( LabApp& app, const char* name )
		{
			const std::vector<std::string>& busses = app.GetFolder().auxBusses;
			if( std::find( busses.begin(), busses.end(), name ) != busses.end() )
			{
				return name;
			}
			return busses.empty() ? std::string() : busses.front();
		}

		/// A hall up the screen and a room below it that share the wall at z = -4, a door in that wall, and a window
		/// from the room to outdoors at z = 8. rooms[0] is the hall, rooms[1] the room, portals[0] the door and
		/// portals[1] the window.
		void AddHouse( LabApp& app )
		{
			LabScene& scene = app.GetScene();
			if( LabRoom* hall = scene.AddRoom( 0.0f, -10.0f, 16.0f, 12.0f, Bus( app, "AudioLab_Hall" ) ) )
			{
				hall->settings.name = "Hall";
				scene.ApplySettings( *hall );
			}
			if( LabRoom* room = scene.AddRoom( 0.0f, 2.0f, 12.0f, 12.0f, Bus( app, "AudioLab_Room" ) ) )
			{
				room->settings.name = "Room";
				scene.ApplySettings( *room );
			}
			if( LabPortal* door = scene.AddPortal( 0.0f, -4.0f, 0.0f ) )
			{
				door->settings.name = "Door";
				scene.ApplySettings( *door );
			}
			if( LabPortal* window = scene.AddPortal( 4.0f, 8.0f, 0.0f ) )
			{
				window->settings.name = "Window";
				scene.ApplySettings( *window );
			}
		}

		/// The listener in the room and a loop playing in the hall, placed so that neither sees the other through the
		/// door: Wwise reports no paths for an emitter in direct sight of the listener.
		void AddListenerAndEmitter( LabApp& app )
		{
			LabScene& scene = app.GetScene();
			scene.listener.x = -5.0f;
			scene.listener.z = 5.0f;
			if( LabEmitter* emitter = scene.AddEmitter( 7.0f, -10.0f, app.GetDefaultEvent() ) )
			{
				scene.Play( *emitter );
			}
		}

		CarbonAudioHost::PortalStatus PortalStatusOf( LabApp& app, size_t portal )
		{
			CarbonAudioHost::PortalStatus status;
			LabScene& scene = app.GetScene();
			if( portal < scene.portals.size() )
			{
				CarbonAudioHost::GetPortalStatus( scene.portals[portal].object.p, status );
			}
			return status;
		}

		/// The room's Wwise ID; 0 while it is not in Wwise.
		uint64_t RoomIDOf( LabApp& app, size_t room )
		{
			CarbonAudioHost::RoomStatus status;
			LabScene& scene = app.GetScene();
			if( room >= scene.rooms.size() || !CarbonAudioHost::GetRoomStatus( scene.rooms[room].object.p, status ) || !status.inWwise )
			{
				return 0;
			}
			return status.roomID;
		}

		std::string Describe( LabApp& app, const CarbonAudioHost::PortalStatus& status )
		{
			if( !status.inWwise )
			{
				return "not in Wwise";
			}
			return "front " + WwiseRoomText( app.GetScene(), status.frontRoomID ) + ", back " + WwiseRoomText( app.GetScene(), status.backRoomID );
		}

		/// Whether the portal connects the two rooms, front first.
		bool Connects( const CarbonAudioHost::PortalStatus& status, uint64_t front, uint64_t back )
		{
			return status.inWwise && front != 0 && back != 0 && status.frontRoomID == front && status.backRoomID == back;
		}

		/// Whether a path Wwise found from the first emitter to the listener goes through the portal.
		bool PathThrough( LabApp& app, uint64_t portalID, std::string& detail )
		{
			if( app.GetScene().emitters.empty() )
			{
				return false;
			}
			const std::vector<CarbonAudioHost::DiffractionPath> paths = CarbonAudioHost::GetDiffractionPaths( app.GetScene().emitters[0].gameObjectID );
			detail = std::to_string( paths.size() ) + " paths";
			for( const CarbonAudioHost::DiffractionPath& path : paths )
			{
				if( std::find( path.portals.begin(), path.portals.end(), portalID ) != path.portals.end() )
				{
					return true;
				}
			}
			return false;
		}

		void SetDoorOpen( LabApp& app, bool open )
		{
			LabPortal& door = app.GetScene().portals[0];
			door.settings.open = open;
			app.GetScene().ApplySettings( door );
		}

		class PortalsPage : public LabPage
		{
		public:
			const char* GetID() const override { return "portals"; }
			const char* GetName() const override { return "Portals"; }
			const char* GetHelp() const override
			{
				return "A portal is an opening, such as a door or a window, that lets sound through between two rooms, or a "
					   "room and outdoors. CarbonAudio connects each portal to the rooms at the centres of its front and back "
					   "faces: the arrow points at the front room, and the lines go to the rooms Wwise has. A portal with the "
					   "same room on both sides is not sent; place it across a room's wall. A closed portal (crossed) stays in "
					   "Wwise and lets nothing through; occlusion muffles everything through it, like a door left ajar.\n"
					   "Look for: the yellow paths from the hall's loop bending through the door to the listener, the loop "
					   "dropping to transmission when the door closes, and the reverb crossfading as you walk through the door "
					   "(W A S D). With the Wwise Profiler attached, the Game Object 3D Viewer shows the portals and the "
					   "openings they cut in the rooms. Move the origin to see the portals keep their rooms while every box "
					   "moves, as in the game.";
			}

			void Enter( LabApp& ) override
			{
				m_movingOrigin = false;
				m_walking = false;
			}

			void BuildDemo( LabApp& app ) override
			{
				AddHouse( app );
				AddListenerAndEmitter( app );
			}

			void Leave( LabApp& ) override
			{
				m_movingOrigin = false;
				m_walking = false;
			}

			void Update( LabApp& app ) override
			{
				LabScene& scene = app.GetScene();
				if( m_movingOrigin )
				{
					// Everything moves, as it does in the game whenever the player moves. On every other frame the
					// portals stay a frame behind the rooms, as when the audio tick catches trinity halfway through
					// sending the boxes.
					++m_frame;
					const bool portalsBehind = m_frame % 2 == 1;
					scene.listener.z += ORIGIN_STEP;
					for( LabEmitter& emitter : scene.emitters )
					{
						emitter.z += ORIGIN_STEP;
					}
					for( LabRoom& room : scene.rooms )
					{
						room.z += ORIGIN_STEP;
						scene.ApplyShape( room );
					}
					for( LabPortal& portal : scene.portals )
					{
						portal.z += portalsBehind ? 0.0f : 2.0f * ORIGIN_STEP;
						scene.ApplyShape( portal );
					}
				}
				if( m_walking )
				{
					scene.listener.z -= WALK_STEP;
					uint64_t roomID = 0;
					if( CarbonAudioHost::GetAssignedRoom( CarbonAudioHost::GetListenerID(), roomID ) && roomID != m_listenerRoom )
					{
						m_listenerRoom = roomID;
						++m_listenerRoomChanges;
					}
				}
			}

			void DrawControls( LabApp& app ) override
			{
				LabScene& scene = app.GetScene();
				if( ImGui::Checkbox( "Move the origin", &m_movingOrigin ) && !m_movingOrigin && m_frame % 2 == 1 )
				{
					// Catch the portals up with the rooms.
					for( LabPortal& portal : scene.portals )
					{
						portal.z += ORIGIN_STEP;
						scene.ApplyShape( portal );
					}
				}
				HelpMarker( "Moves everything every frame, with the portals a frame behind the rooms on every other frame." );
				for( LabPortal& portal : scene.portals )
				{
					ImGui::PushID( portal.id );
					if( ImGui::Checkbox( portal.settings.name.c_str(), &portal.settings.open ) )
					{
						scene.ApplySettings( portal );
					}
					ImGui::SetItemTooltip( "Open" );
					ImGui::SameLine();
					ImGui::TextDisabled( "%s", Describe( app, PortalStatusOf( app, static_cast<size_t>( &portal - scene.portals.data() ) ) ).c_str() );
					ImGui::PopID();
				}
			}

			void DrawOverlay( LabApp& app, const SceneView& view ) override
			{
				// The paths Wwise found from each emitter to the listener; a dot where one goes through a portal.
				LabScene& scene = app.GetScene();
				const ImU32 colour = IM_COL32( 240, 220, 90, 200 );
				const ImVec2 listener = view.ToScreen( scene.listener.x, scene.listener.z );
				for( const LabEmitter& emitter : scene.emitters )
				{
					for( const CarbonAudioHost::DiffractionPath& path : CarbonAudioHost::GetDiffractionPaths( emitter.gameObjectID ) )
					{
						if( path.nodes.empty() )
						{
							continue;
						}
						ImVec2 from = listener;
						for( size_t i = 0; i < path.nodes.size(); ++i )
						{
							const ImVec2 to = view.ToScreen( path.nodes[i].x, path.nodes[i].z );
							view.draw->AddLine( from, to, colour, 1.5f * view.uiScale );
							if( path.portals[i] != 0 )
							{
								view.draw->AddCircleFilled( to, 3.5f * view.uiScale, colour );
							}
							from = to;
						}
						view.draw->AddLine( from, view.ToScreen( emitter.x, emitter.z ), colour, 1.5f * view.uiScale );
						char text[32];
						std::snprintf( text, sizeof( text ), "%.0f%%", path.diffraction * 100.0f );
						view.draw->AddText( ImVec2( from.x + 4.0f * view.uiScale, from.y ), colour, text );
					}
				}
			}

			std::vector<LabCheck> GetChecks() override
			{
				std::vector<LabCheck> checks;

				checks.push_back( { "rooms", "The door connects the hall in front of it and the room behind it, and the window the room and outdoors.", []( CheckScript& script ) {
					script.Do( "a hall and a room with a door between them, and a window out of the room", []( LabApp& app ) { AddHouse( app ); } );
					script.Expect( "the door connects the hall and the room", []( LabApp& app, std::string& detail ) {
						const CarbonAudioHost::PortalStatus door = PortalStatusOf( app, 0 );
						detail = Describe( app, door );
						return Connects( door, RoomIDOf( app, 0 ), RoomIDOf( app, 1 ) );
					} );
					script.Expect( "the window connects the room and outdoors", []( LabApp& app, std::string& detail ) {
						const CarbonAudioHost::PortalStatus window = PortalStatusOf( app, 1 );
						detail = Describe( app, window );
						return window.inWwise && window.frontRoomID == RoomIDOf( app, 1 ) && window.backRoomID == CarbonAudioHost::kOutdoorRoomID;
					} );
				} } );

				checks.push_back( { "same-room", "A portal with the same room on both sides is not sent to Wwise.", []( CheckScript& script ) {
					script.Do( "a room with a portal in its middle", []( LabApp& app ) {
						app.GetScene().AddRoom( 0.0f, 0.0f, 12.0f, 12.0f, Bus( app, "AudioLab_Room" ) );
						app.GetScene().AddPortal( 0.0f, 0.0f, 0.0f );
					} );
					script.Expect( "the room is in Wwise", []( LabApp& app, std::string& ) { return RoomIDOf( app, 0 ) != 0; } );
					script.ExpectFor( "the portal stays out of Wwise", []( LabApp& app, std::string& detail ) {
						const CarbonAudioHost::PortalStatus portal = PortalStatusOf( app, 0 );
						detail = Describe( app, portal );
						return !portal.inWwise;
					}, 0.5 );
				} } );

				checks.push_back( { "on-wall", "A portal placed on any of a room's walls goes across that wall, with the room in front and outdoors behind.", []( CheckScript& script ) {
					script.Do( "a room, and a portal placed just off each of its four walls", []( LabApp& app ) {
						LabScene& scene = app.GetScene();
						scene.AddRoom( 0.0f, 0.0f, 12.0f, 8.0f, Bus( app, "AudioLab_Room" ) );
						// Right, left, top and bottom, as clicked with the Portal tool, outside and inside the walls.
						scene.AddPortalOnWall( 6.5f, 1.0f, 1.5f );
						scene.AddPortalOnWall( -5.5f, -1.0f, 1.5f );
						scene.AddPortalOnWall( 2.0f, -4.5f, 1.5f );
						scene.AddPortalOnWall( -2.0f, 3.6f, 1.5f );
					} );
					script.Expect( "each connects the room and outdoors", []( LabApp& app, std::string& detail ) {
						const uint64_t room = RoomIDOf( app, 0 );
						for( size_t portal = 0; portal < 4; ++portal )
						{
							const CarbonAudioHost::PortalStatus status = PortalStatusOf( app, portal );
							if( !status.inWwise || status.frontRoomID != room || status.backRoomID != CarbonAudioHost::kOutdoorRoomID )
							{
								detail = portal < app.GetScene().portals.size() ? app.GetScene().portals[portal].settings.name + ": " + Describe( app, status ) : "a portal is missing";
								return false;
							}
						}
						return room != 0;
					} );
				} } );

				checks.push_back( { "door-path", "Sound from the hall reaches the listener in the room through the open door, and not through the closed one.", []( CheckScript& script ) {
					script.Do( "the house, the listener in the room and a loop in the hall", []( LabApp& app ) {
						AddHouse( app );
						AddListenerAndEmitter( app );
					} );
					script.Expect( "a path goes through the door", []( LabApp& app, std::string& detail ) { return PathThrough( app, PortalStatusOf( app, 0 ).portalID, detail ); } );
					script.Do( "close the door", []( LabApp& app ) { SetDoorOpen( app, false ); } );
					script.Expect( "the door stays in Wwise", []( LabApp& app, std::string& detail ) {
						detail = Describe( app, PortalStatusOf( app, 0 ) );
						return PortalStatusOf( app, 0 ).inWwise;
					}, 0.0 );
					script.Expect( "no path goes through it", []( LabApp& app, std::string& detail ) { return !PathThrough( app, PortalStatusOf( app, 0 ).portalID, detail ); } );
					script.Do( "open the door again", []( LabApp& app ) { SetDoorOpen( app, true ); } );
					script.Expect( "a path goes through the door again", []( LabApp& app, std::string& detail ) { return PathThrough( app, PortalStatusOf( app, 0 ).portalID, detail ); } );
				} } );

				checks.push_back( { "occlusion", "The door's occlusion reaches Wwise, and again after the rooms switch is turned off and on.", []( CheckScript& script ) {
					script.Do( "the house, with the door half occluded", []( LabApp& app ) {
						AddHouse( app );
						LabPortal& door = app.GetScene().portals[0];
						door.settings.occlusion = 0.5f;
						app.GetScene().ApplySettings( door );
					} );
					const auto sent = []( LabApp& app, std::string& detail ) {
						const CarbonAudioHost::PortalStatus door = PortalStatusOf( app, 0 );
						char text[64];
						std::snprintf( text, sizeof( text ), "%s, occlusion %s %.2f", door.inWwise ? "in Wwise" : "not in Wwise", door.obstructionOcclusionSent ? "sent" : "not sent", door.sentOcclusion );
						detail = text;
						return door.inWwise && door.obstructionOcclusionSent && door.sentOcclusion == 0.5f;
					};
					script.Expect( "it is sent", sent );
					script.Do( "turn rooms off", []( LabApp& ) { CarbonAudioHost::SetRoomsEnabled( false ); } );
					script.Expect( "the door left Wwise", []( LabApp& app, std::string& ) { return !PortalStatusOf( app, 0 ).inWwise; } );
					script.Do( "turn rooms on", []( LabApp& ) { CarbonAudioHost::SetRoomsEnabled( true ); } );
					script.Expect( "it is sent again", sent );
				} } );

				checks.push_back( { "room-removed", "Removing the hall takes the door out of Wwise before the hall, and sends it again between the room and outdoors.", []( CheckScript& script ) {
					const auto hallID = std::make_shared<uint64_t>( 0 );
					script.Do( "the house", []( LabApp& app ) { AddHouse( app ); } );
					script.Expect( "the door connects the hall and the room", [hallID]( LabApp& app, std::string& detail ) {
						*hallID = RoomIDOf( app, 0 );
						detail = Describe( app, PortalStatusOf( app, 0 ) );
						return Connects( PortalStatusOf( app, 0 ), *hallID, RoomIDOf( app, 1 ) );
					} );
					const auto doorStayed = std::make_shared<bool>( true );
					script.Do( "remove the hall", [doorStayed]( LabApp& app ) {
						app.GetScene().RemoveRoom( app.GetScene().rooms[0].id );
						*doorStayed = PortalStatusOf( app, 0 ).inWwise;
					} );
					script.Expect( "the door left Wwise with the hall", [doorStayed]( LabApp&, std::string& ) { return !*doorStayed; }, 0.0 );
					script.Expect( "the door connects outdoors and the room", []( LabApp& app, std::string& detail ) {
						const CarbonAudioHost::PortalStatus door = PortalStatusOf( app, 0 );
						detail = Describe( app, door );
						return door.inWwise && door.frontRoomID == CarbonAudioHost::kOutdoorRoomID && door.backRoomID == RoomIDOf( app, 0 );
					} );
					script.ExpectFor( "it never names the removed hall", [hallID]( LabApp& app, std::string& detail ) {
						const CarbonAudioHost::PortalStatus door = PortalStatusOf( app, 0 );
						detail = Describe( app, door );
						return door.frontRoomID != *hallID && door.backRoomID != *hallID;
					}, 0.3 );
				} } );

				checks.push_back( { "moving-origin", "While everything moves every frame, with the portals a frame behind the rooms on every other frame, the door keeps its rooms.", [this]( CheckScript& script ) {
					script.Do( "the house", []( LabApp& app ) { AddHouse( app ); } );
					script.Expect( "the door connects the hall and the room", []( LabApp& app, std::string& detail ) {
						detail = Describe( app, PortalStatusOf( app, 0 ) );
						return Connects( PortalStatusOf( app, 0 ), RoomIDOf( app, 0 ), RoomIDOf( app, 1 ) );
					} );
					script.Do( "move the origin every frame", [this]( LabApp& ) {
						m_frame = 0;
						m_movingOrigin = true;
					} );
					script.ExpectFor( "the door keeps the hall in front and the room behind", []( LabApp& app, std::string& detail ) {
						detail = Describe( app, PortalStatusOf( app, 0 ) );
						return Connects( PortalStatusOf( app, 0 ), RoomIDOf( app, 0 ), RoomIDOf( app, 1 ) );
					}, 1.0 );
					script.Do( "stop", [this]( LabApp& ) { m_movingOrigin = false; } );
				} } );

				checks.push_back( { "walk-through", "Walking the listener through the door moves it from the room to the hall once.", [this]( CheckScript& script ) {
					script.Do( "the house, and the listener in the room in line with the door", []( LabApp& app ) {
						AddHouse( app );
						app.GetScene().listener.z = 4.0f;
					} );
					script.Expect( "the listener is in the room", []( LabApp& app, std::string& ) {
						uint64_t roomID = 0;
						return CarbonAudioHost::GetAssignedRoom( CarbonAudioHost::GetListenerID(), roomID ) && roomID == RoomIDOf( app, 1 ) && roomID != 0;
					} );
					script.Do( "walk up through the door", [this]( LabApp& app ) {
						m_listenerRoom = RoomIDOf( app, 1 );
						m_listenerRoomChanges = 0;
						m_walking = true;
					} );
					script.Expect( "the listener is in the hall", []( LabApp& app, std::string& ) {
						uint64_t roomID = 0;
						return app.GetScene().listener.z < -8.0f && CarbonAudioHost::GetAssignedRoom( CarbonAudioHost::GetListenerID(), roomID ) && roomID == RoomIDOf( app, 0 );
					}, 5.0 );
					script.Do( "stop", [this]( LabApp& ) { m_walking = false; } );
					script.Expect( "it changed room once", [this]( LabApp&, std::string& detail ) {
						detail = std::to_string( m_listenerRoomChanges ) + " changes";
						return m_listenerRoomChanges == 1;
					}, 0.0 );
				} } );

				return checks;
			}

		private:
			bool m_movingOrigin = false;
			int m_frame = 0;
			bool m_walking = false;
			uint64_t m_listenerRoom = 0;
			int m_listenerRoomChanges = 0;
		};
	}

	std::unique_ptr<LabPage> CreatePortalsPage()
	{
		return std::make_unique<PortalsPage>();
	}
}
