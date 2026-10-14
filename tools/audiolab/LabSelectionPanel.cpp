// Copyright © 2026 CCP ehf.

#include "LabSelectionPanel.h"

#include "LabApp.h"
#include "LabSceneCanvas.h"
#include "LabWidgets.h"
#include "Platform.h"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float MIN_ROOM_SIZE = 0.5f;
	constexpr float MIN_PORTAL_SIZE = 0.1f;
	/// Rows the object list shows before it scrolls.
	constexpr int OBJECT_LIST_ROWS = 8;
}

namespace AudioLab
{
	SelectionPanel::SelectionPanel( LabApp& app, Selection& selection, SceneCanvas& canvas ) :
		m_scene( app.GetScene() ),
		m_folder( app.GetFolder() ),
		m_selection( selection ),
		m_canvas( canvas )
	{
	}

	void SelectionPanel::Draw()
	{
		DrawObjectList();
		ImGui::Spacing();
		const int single = m_selection.GetIDs().size() == 1 ? m_selection.GetIDs().front() : LabScene::NO_ID;
		if( m_selection.GetIDs().size() > 1 )
		{
			DrawMultipleSettings();
		}
		else if( single == LabScene::LISTENER_ID )
		{
			DrawListenerSettings();
		}
		else if( LabRoom* room = m_scene.FindRoom( single ) )
		{
			DrawRoomSettings( *room );
		}
		else if( LabPortal* portal = m_scene.FindPortal( single ) )
		{
			DrawPortalSettings( *portal );
		}
		else if( LabEmitter* emitter = m_scene.FindEmitter( single ) )
		{
			DrawEmitterSettings( *emitter );
		}
		else if( LabWall* wall = m_scene.FindWall( single ) )
		{
			DrawWallSettings( *wall );
		}
		else
		{
			ImGui::PushTextWrapPos( 0.0f );
			ImGui::TextDisabled( "Click something in the scene, or pick it in the list. Drag a box in the scene to select "
								 "several; Ctrl-click adds or takes out one." );
			ImGui::PopTextWrapPos();
		}
	}

	void SelectionPanel::DrawMultipleSettings()
	{
		ImGui::SeparatorText( m_selection.Describe().c_str() );
		if( m_selection.Count( LabKind::Emitter ) > 0 )
		{
			if( ImGui::Button( "Play" ) )
			{
				m_selection.Play( true );
			}
			ImGui::SameLine();
			if( ImGui::Button( "Stop" ) )
			{
				m_selection.Play( false );
			}
			ImGui::SetItemTooltip( "Stops every sound on the selected emitters" );
			ImGui::SameLine();
		}
		if( m_selection.Count( LabKind::Emitter ) + m_selection.Count( LabKind::Room ) + m_selection.Count( LabKind::Portal ) + m_selection.Count( LabKind::Wall ) > 0 && ImGui::Button( "Remove" ) )
		{
			m_selection.RemoveFromScene();
		}
		ImGui::SetItemTooltip( "Removes the selected emitters, rooms, portals and walls (Delete)" );
		ImGui::PushTextWrapPos( 0.0f );
		ImGui::TextDisabled( "Drag any of them in the scene to move them together." );
		ImGui::PopTextWrapPos();
	}

	void SelectionPanel::DrawObjectList()
	{
		const int rows = 1 + static_cast<int>( m_scene.rooms.size() + m_scene.portals.size() + m_scene.emitters.size() + m_scene.walls.size() );
		const float height = ImGui::GetTextLineHeightWithSpacing() * static_cast<float>( std::min( rows, OBJECT_LIST_ROWS ) ) + ImGui::GetStyle().FramePadding.y * 2.0f;
		if( !ImGui::BeginListBox( "##Objects", ImVec2( -FLT_MIN, height ) ) )
		{
			return;
		}
		const auto item = [this]( const char* name, const char* detail, int id ) {
			const bool selected = m_selection.Contains( id );
			if( ImGui::Selectable( name, selected ) )
			{
				const ImGuiIO& io = ImGui::GetIO();
				io.KeyCtrl || io.KeyShift ? m_selection.Toggle( id ) : m_selection.Select( id );
			}
			// To the last one selected, which is the one just clicked in the scene.
			if( selected && m_selection.WantsScroll() && m_selection.GetIDs().back() == id )
			{
				ImGui::SetScrollHereY();
				m_selection.Scrolled();
			}
			if( detail != nullptr && detail[0] != '\0' )
			{
				ImGui::SameLine();
				ImGui::TextDisabled( "%s", detail );
			}
		};

		item( "Listener", nullptr, LabScene::LISTENER_ID );
		ImGui::PushID( "rooms" );
		for( const LabRoom& room : m_scene.rooms )
		{
			ImGui::PushID( room.id );
			item( room.settings.name.c_str(), "room", room.id );
			ImGui::PopID();
		}
		ImGui::PopID();
		ImGui::PushID( "portals" );
		for( const LabPortal& portal : m_scene.portals )
		{
			ImGui::PushID( portal.id );
			item( portal.settings.name.c_str(), "portal", portal.id );
			ImGui::PopID();
		}
		ImGui::PopID();
		ImGui::PushID( "emitters" );
		for( const LabEmitter& emitter : m_scene.emitters )
		{
			ImGui::PushID( emitter.id );
			item( emitter.name.c_str(), emitter.eventName.c_str(), emitter.id );
			ImGui::PopID();
		}
		ImGui::PopID();
		ImGui::PushID( "walls" );
		for( const LabWall& wall : m_scene.walls )
		{
			ImGui::PushID( wall.id );
			item( wall.name.c_str(), "wall", wall.id );
			ImGui::PopID();
		}
		ImGui::PopID();
		ImGui::EndListBox();
	}

	void SelectionPanel::DrawListenerSettings()
	{
		LabListener& listener = m_scene.listener;
		ImGui::SeparatorText( "Listener" );
		DragPosition( "Position", listener.x, listener.z );
		ImGui::SliderFloat( "Facing", &listener.yawDegrees, -180.0f, 180.0f, "%.0f deg" );
		ImGui::SliderFloat( "Walk speed", &m_canvas.WalkSpeed(), 1.0f, 200.0f, "%.0f m/s", ImGuiSliderFlags_Logarithmic );
		HelpMarker( "How fast W A S D walk the listener; Shift triples it." );
		ImGui::Text( "Room: %s", AssignedRoomText( m_scene, CarbonAudioHost::GetListenerID() ).c_str() );
	}

	void SelectionPanel::DrawRoomSettings( LabRoom& room )
	{
		CarbonAudioHost::RoomSettings& settings = room.settings;
		bool settingsChanged = false;
		bool shapeChanged = false;

		ImGui::SeparatorText( "Room" );
		settingsChanged |= InputString( "Name", settings.name );
		settingsChanged |= ImGui::Checkbox( "Enabled", &settings.enabled );
		if( ImGui::BeginCombo( "Reverb", settings.reverbAuxBus.empty() ? "(none)" : settings.reverbAuxBus.c_str() ) )
		{
			if( ImGui::Selectable( "(none)", settings.reverbAuxBus.empty() ) )
			{
				settings.reverbAuxBus.clear();
				settingsChanged = true;
			}
			for( const std::string& bus : m_folder.auxBusses )
			{
				if( ImGui::Selectable( bus.c_str(), bus == settings.reverbAuxBus ) )
				{
					settings.reverbAuxBus = bus;
					settingsChanged = true;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SetItemTooltip( "The room's reverb aux bus" );
		settingsChanged |= ImGui::SliderFloat( "Reverb level", &settings.reverbLevel, 0.0f, 1.0f );
		settingsChanged |= ImGui::SliderFloat( "Transmission loss", &settings.transmissionLoss, 0.0f, 1.0f );
		std::string tone = ToUtf8( settings.roomToneEvent );
		if( EventCombo( m_folder, "Room tone", tone, true ) )
		{
			settings.roomToneEvent = ToWide( tone );
			settingsChanged = true;
		}

		ImGui::SeparatorText( "Shape" );
		shapeChanged |= DragPosition( "Position", room.x, room.z );
		float size[3] = { room.width, room.depth, room.height };
		if( ImGui::DragFloat3( "Size", size, 0.1f, MIN_ROOM_SIZE, 1000.0f, "%.1f m", ImGuiSliderFlags_AlwaysClamp ) )
		{
			room.width = size[0];
			room.depth = size[1];
			room.height = size[2];
			shapeChanged = true;
		}
		ImGui::SetItemTooltip( "Width, depth and height" );
		shapeChanged |= ImGui::SliderFloat( "Yaw", &room.yawDegrees, -180.0f, 180.0f, "%.0f deg" );

		if( ImGui::TreeNode( "More settings" ) )
		{
			settingsChanged |= ImGui::DragFloat( "Priority", &settings.priority, 1.0f );
			settingsChanged |= ImGui::SliderFloat( "Aux send to self", &settings.auxSendLevelToSelf, 0.0f, 1.0f );
			settingsChanged |= ImGui::Checkbox( "Keep registered", &settings.keepRegistered );
			ImGui::TreePop();
		}

		if( settingsChanged )
		{
			m_scene.ApplySettings( room );
		}
		if( shapeChanged )
		{
			m_scene.ApplyShape( room );
		}

		ImGui::SeparatorText( "In CarbonAudio" );
		CarbonAudioHost::RoomStatus status;
		if( CarbonAudioHost::GetRoomStatus( room.object.p, status ) )
		{
			if( status.inWwise )
			{
				ImGui::Text( "In Wwise as room %llu", static_cast<unsigned long long>( status.roomID ) );
			}
			else
			{
				ImGui::TextColored( WARNING_COLOUR, "Not in Wwise" );
			}
			ImGui::Text( "Room tone: %s", status.roomTonePosted ? "playing" : status.roomTonePending ? "waiting for its SoundBank" : "none" );
		}

		ImGui::Spacing();
		if( ImGui::Button( "Remove room" ) )
		{
			m_selection.RemoveFromScene();
		}
	}

	void SelectionPanel::DrawPortalSettings( LabPortal& portal )
	{
		CarbonAudioHost::PortalSettings& settings = portal.settings;
		bool settingsChanged = false;
		bool shapeChanged = false;

		ImGui::SeparatorText( "Portal" );
		settingsChanged |= InputString( "Name", settings.name );
		settingsChanged |= ImGui::Checkbox( "Enabled", &settings.enabled );
		ImGui::SameLine();
		settingsChanged |= ImGui::Checkbox( "Open", &settings.open );
		ImGui::SetItemTooltip( "A closed portal stays in Wwise but lets no sound through, like a shut door" );
		settingsChanged |= ImGui::SliderFloat( "Occlusion", &settings.occlusion, 0.0f, 1.0f );
		ImGui::SetItemTooltip( "Muffles all sound through the portal, e.g. a door that is partly closed" );
		settingsChanged |= ImGui::SliderFloat( "Obstruction", &settings.obstruction, 0.0f, 1.0f );
		ImGui::SetItemTooltip( "Muffles the direct sound through the portal" );
		settingsChanged |= ImGui::Checkbox( "Fade over the depth", &settings.obstructionOcclusionTransition );
		ImGui::SetItemTooltip( "Fades obstruction and occlusion in over the portal's depth" );
		settingsChanged |= ImGui::SliderFloat( "Adjacent room bleed", &settings.adjacentRoomBleed, 0.0f, 4.0f );
		ImGui::SetItemTooltip( "How much of the reverb of the room on the other side comes through" );

		ImGui::SeparatorText( "Shape" );
		shapeChanged |= DragPosition( "Position", portal.x, portal.z );
		float size[3] = { portal.width, portal.depth, portal.height };
		if( ImGui::DragFloat3( "Size", size, 0.05f, MIN_PORTAL_SIZE, 100.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp ) )
		{
			portal.width = size[0];
			portal.depth = size[1];
			portal.height = size[2];
			shapeChanged = true;
		}
		ImGui::SetItemTooltip( "Width across the opening, depth through the wall, and height" );
		shapeChanged |= ImGui::SliderFloat( "Yaw", &portal.yawDegrees, -180.0f, 180.0f, "%.0f deg" );
		ImGui::SetItemTooltip( "At 0 the front room is up the screen" );

		if( settingsChanged )
		{
			m_scene.ApplySettings( portal );
		}
		if( shapeChanged )
		{
			m_scene.ApplyShape( portal );
		}

		ImGui::SeparatorText( "In CarbonAudio" );
		CarbonAudioHost::PortalStatus status;
		if( CarbonAudioHost::GetPortalStatus( portal.object.p, status ) )
		{
			if( status.inWwise )
			{
				ImGui::Text( "In Wwise as portal %llu", static_cast<unsigned long long>( status.portalID ) );
				ImGui::Text( "Front: %s", WwiseRoomText( m_scene, status.frontRoomID ).c_str() );
				ImGui::Text( "Back: %s", WwiseRoomText( m_scene, status.backRoomID ).c_str() );
			}
			else
			{
				ImGui::TextColored( WARNING_COLOUR, "Not in Wwise" );
				ImGui::PushTextWrapPos( 0.0f );
				ImGui::TextDisabled( "A portal needs a different room, or outdoors, on each side: place it across a room's wall." );
				ImGui::PopTextWrapPos();
			}
		}

		ImGui::Spacing();
		if( ImGui::Button( "Remove portal" ) )
		{
			m_selection.RemoveFromScene();
		}
	}

	void SelectionPanel::DrawEmitterSettings( LabEmitter& emitter )
	{
		ImGui::SeparatorText( emitter.name.c_str() );
		ImGui::SetItemTooltip( "Game object %llu", static_cast<unsigned long long>( emitter.gameObjectID ) );
		DragPosition( "Position", emitter.x, emitter.z );
		EventCombo( m_folder, "Event", emitter.eventName, false );
		if( ImGui::Button( "Play" ) )
		{
			m_scene.Play( emitter );
		}
		ImGui::SameLine();
		if( ImGui::Button( "Stop" ) )
		{
			m_scene.Stop( emitter );
		}
		ImGui::SetItemTooltip( "Stops every sound on the emitter" );

		if( emitter.object )
		{
			ImGui::SeparatorText( "In CarbonAudio" );
			ImGui::Text( "Room: %s", AssignedRoomText( m_scene, emitter.gameObjectID ).c_str() );
			ImGui::Text( "Occlusion %.2f", CarbonAudioHost::GetEmitterOcclusion( emitter.gameObjectID ) );

			const std::vector<CarbonAudioHost::PlayingEvent> playing = CarbonAudioHost::GetPlayingEvents( emitter.object.p );
			if( playing.empty() )
			{
				ImGui::TextDisabled( emitter.lastPlayingID == 0 ? "Playing nothing (its SoundBank was not loaded yet)" : "Playing nothing" );
			}
			for( const CarbonAudioHost::PlayingEvent& event : playing )
			{
				ImGui::PushID( static_cast<int>( event.playingID ) );
				if( ImGui::SmallButton( "Stop" ) )
				{
					CarbonAudioHost::StopSound( emitter.object.p, event.playingID );
				}
				ImGui::SameLine();
				ImGui::Text( "Playing %s", ToUtf8( event.eventName ).c_str() );
				ImGui::SetItemTooltip( "Playing ID %u", event.playingID );
				ImGui::PopID();
			}

			ImGui::SeparatorText( "Game object" );
			if( ImGui::SliderFloat( "Scaling factor", &emitter.scalingFactor, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic ) )
			{
				emitter.object->SetAttenuationScalingFactor( emitter.scalingFactor );
			}
			HelpMarker( "Scales the attenuation, and with it how far the emitter reaches." );
			if( ImGui::Checkbox( "Muted", &emitter.muted ) )
			{
				emitter.muted ? emitter.object->Mute() : emitter.object->Unmute();
			}

			if( ImGui::TreeNode( "RTPC and switch" ) )
			{
				ImGui::InputText( "RTPC", &m_emitterRTPC );
				ImGui::SliderFloat( "Value", &m_emitterRTPCValue, 0.0f, 100.0f );
				ImGui::BeginDisabled( m_emitterRTPC.empty() );
				if( ImGui::Button( "Set RTPC" ) )
				{
					emitter.object->SetRTPC( ToWide( m_emitterRTPC ), m_emitterRTPCValue );
				}
				ImGui::EndDisabled();
				ImGui::InputText( "Switch group", &m_switchGroup );
				ImGui::InputText( "Switch state", &m_switchState );
				ImGui::BeginDisabled( m_switchGroup.empty() );
				if( ImGui::Button( "Set switch" ) )
				{
					emitter.object->SetSwitch( ToWide( m_switchGroup ), ToWide( m_switchState ) );
				}
				ImGui::EndDisabled();
				for( const auto& [group, state] : CarbonAudioHost::GetSwitches( emitter.object.p ) )
				{
					ImGui::BulletText( "%s = %s", ToUtf8( group ).c_str(), ToUtf8( state ).c_str() );
				}
				ImGui::TreePop();
			}
		}

		ImGui::Spacing();
		if( ImGui::Button( "Remove emitter" ) )
		{
			m_selection.RemoveFromScene();
		}
	}

	void SelectionPanel::DrawWallSettings( LabWall& wall )
	{
		ImGui::SeparatorText( wall.name.c_str() );
		DragPosition( "Start", wall.x0, wall.z0 );
		DragPosition( "End", wall.x1, wall.z1 );
		ImGui::Text( "%.1f m long", std::hypot( wall.x1 - wall.x0, wall.z1 - wall.z0 ) );
		ImGui::PushTextWrapPos( 0.0f );
		if( CarbonAudioHost::GetObstructionOcclusionEnabled() )
		{
			ImGui::TextDisabled( "It occludes the emitters whose sightline to the listener it crosses." );
		}
		else
		{
			ImGui::TextColored( WARNING_COLOUR, "Occlusion is off, so it blocks nothing; the toolbar turns it on." );
		}
		ImGui::PopTextWrapPos();

		ImGui::Spacing();
		if( ImGui::Button( "Remove wall" ) )
		{
			m_selection.RemoveFromScene();
		}
	}

}
