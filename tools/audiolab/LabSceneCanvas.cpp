// Copyright © 2026 CCP ehf.

#include "LabSceneCanvas.h"

#include "LabApp.h"
#include "LabWidgets.h"
#include "Platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <limits>
#include <map>

namespace
{
	constexpr float DEGREES_TO_RADIANS = 3.14159265358979f / 180.0f;
	constexpr float PICK_RADIUS_PIXELS = 10.0f;
	constexpr float MIN_ROOM_SIZE = 0.5f;
	constexpr float MIN_WALL_LENGTH = 0.5f;
	/// A new portal goes on a room's wall when it is placed this near it: so many metres, or pixels when zoomed out.
	constexpr float PORTAL_SNAP_METRES = 1.5f;
	constexpr float PORTAL_SNAP_PIXELS = 20.0f;
	/// Metres; FrameAll never zooms in closer than this.
	constexpr float MIN_FRAMED_SIZE = 30.0f;
	/// In 3D, how tall walls are drawn; they block sightlines at any height.
	constexpr float WALL_HEIGHT = 3.0f;
	/// How far dragging turns the 3D view.
	constexpr float TURN_DEGREES_PER_PIXEL = 0.3f;

	const ImU32 ROOM_PALETTE[] = {
		IM_COL32( 86, 180, 233, 255 ),
		IM_COL32( 230, 159, 0, 255 ),
		IM_COL32( 0, 158, 115, 255 ),
		IM_COL32( 204, 121, 167, 255 ),
		IM_COL32( 240, 228, 66, 255 ),
		IM_COL32( 213, 94, 0, 255 ),
		IM_COL32( 0, 114, 178, 255 ),
	};
	const ImU32 HINT_COLOUR = IM_COL32( 140, 140, 150, 255 );
	const ImU32 WALL_COLOUR = IM_COL32( 200, 170, 120, 255 );
	const ImU32 SIGHTLINE_CLEAR = IM_COL32( 90, 200, 120, 70 );
	const ImU32 SIGHTLINE_BLOCKED = IM_COL32( 230, 90, 80, 160 );
	const ImU32 PORTAL_COLOUR = IM_COL32( 120, 230, 220, 255 );
	const ImU32 PORTAL_NOT_IN_WWISE_COLOUR = IM_COL32( 230, 90, 80, 255 );

	ImU32 WithAlpha( ImU32 colour, int alpha )
	{
		return ( colour & ~IM_COL32_A_MASK ) | ( static_cast<ImU32>( alpha ) << IM_COL32_A_SHIFT );
	}

	// A box's corners on the ground plane, in the same axes as RoomTransform and PortalTransform.
	void BoxCorners( float boxX, float boxZ, float width, float depth, float yawDegrees, float outX[4], float outZ[4] )
	{
		const float yaw = yawDegrees * DEGREES_TO_RADIANS;
		const float ux = std::cos( yaw ) * width * 0.5f;
		const float uz = -std::sin( yaw ) * width * 0.5f;
		const float vx = std::sin( yaw ) * depth * 0.5f;
		const float vz = std::cos( yaw ) * depth * 0.5f;
		const float signsU[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
		const float signsV[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
		for( int i = 0; i < 4; ++i )
		{
			outX[i] = boxX + signsU[i] * ux + signsV[i] * vx;
			outZ[i] = boxZ + signsU[i] * uz + signsV[i] * vz;
		}
	}

	/// Whether the line of sight goes through a box, centred on the ground plane like rooms and portals; from above,
	/// whether the point is inside its outline.
	bool RayHitsBox( const AudioLab::ScenePoint& from, const AudioLab::ScenePoint& direction, float boxX, float boxZ, float width, float height, float depth, float yawDegrees )
	{
		// Along the box's own axes: across, up and through.
		const float yaw = yawDegrees * DEGREES_TO_RADIANS;
		const float c = std::cos( yaw ), s = std::sin( yaw );
		const float dx = from.x - boxX, dz = from.z - boxZ;
		const float start[3] = { dx * c - dz * s, from.y, dx * s + dz * c };
		const float step[3] = { direction.x * c - direction.z * s, direction.y, direction.x * s + direction.z * c };
		const float half[3] = { width * 0.5f, height * 0.5f, depth * 0.5f };
		float enter = 0.0f;
		float leave = std::numeric_limits<float>::max();
		for( int axis = 0; axis < 3; ++axis )
		{
			if( std::fabs( step[axis] ) < 1e-6f )
			{
				if( std::fabs( start[axis] ) > half[axis] )
				{
					return false;
				}
				continue;
			}
			const float t0 = ( -half[axis] - start[axis] ) / step[axis];
			const float t1 = ( half[axis] - start[axis] ) / step[axis];
			enter = std::max( enter, std::min( t0, t1 ) );
			leave = std::min( leave, std::max( t0, t1 ) );
			if( enter > leave )
			{
				return false;
			}
		}
		return true;
	}

	/// In 3D, the edges of a box centred on the ground plane, like rooms and portals.
	void DrawBoxEdges( const AudioLab::SceneView& view, float boxX, float boxZ, float width, float height, float depth, float yawDegrees, ImU32 colour, float thickness )
	{
		float cx[4], cz[4];
		BoxCorners( boxX, boxZ, width, depth, yawDegrees, cx, cz );
		const float bottom = -height * 0.5f;
		const float top = height * 0.5f;
		for( int i = 0; i < 4; ++i )
		{
			const int next = ( i + 1 ) % 4;
			view.Line( { cx[i], bottom, cz[i] }, { cx[next], bottom, cz[next] }, colour, thickness );
			view.Line( { cx[i], top, cz[i] }, { cx[next], top, cz[next] }, colour, thickness );
			view.Line( { cx[i], bottom, cz[i] }, { cx[i], top, cz[i] }, colour, thickness );
		}
	}

	void RoomCorners( const AudioLab::LabRoom& room, float outX[4], float outZ[4] )
	{
		BoxCorners( room.x, room.z, room.width, room.depth, room.yawDegrees, outX, outZ );
	}

	void PortalCorners( const AudioLab::LabPortal& portal, float outX[4], float outZ[4] )
	{
		BoxCorners( portal.x, portal.z, portal.width, portal.depth, portal.yawDegrees, outX, outZ );
	}

	/// The four corners on the ground plane.
	void GroundCorners( const float cx[4], const float cz[4], AudioLab::ScenePoint out[4] )
	{
		for( int i = 0; i < 4; ++i )
		{
			out[i] = { cx[i], 0.0f, cz[i] };
		}
	}

	float DistanceToSegment( float x, float z, float x0, float z0, float x1, float z1 )
	{
		const float dx = x1 - x0;
		const float dz = z1 - z0;
		const float lengthSq = dx * dx + dz * dz;
		const float t = lengthSq > 0.0f ? std::clamp( ( ( x - x0 ) * dx + ( z - z0 ) * dz ) / lengthSq, 0.0f, 1.0f ) : 0.0f;
		return std::hypot( x - ( x0 + t * dx ), z - ( z0 + t * dz ) );
	}

	/// Where a label went on screen, so later labels can stay clear of it.
	struct LabelBox
	{
		ImVec2 min;
		ImVec2 max;
	};

	bool Overlaps( const LabelBox& a, const LabelBox& b )
	{
		return a.min.x < b.max.x && b.min.x < a.max.x && a.min.y < b.max.y && b.min.y < a.max.y;
	}
}

namespace AudioLab
{
	SceneCanvas::SceneCanvas( LabApp& app, Selection& selection ) :
		m_app( app ),
		m_scene( app.GetScene() ),
		m_folder( app.GetFolder() ),
		m_selection( selection ),
		m_newEmitterEvent( app.GetDefaultEvent() )
	{
		m_view.pixelsPerMetre = 12.0f;
	}

	void SceneCanvas::Reset()
	{
		m_gesture = Gesture::None;
		m_frameAll = true;
	}

	ImU32 SceneCanvas::RoomColour( const LabRoom& room, int alpha ) const
	{
		const size_t index = static_cast<size_t>( &room - m_scene.rooms.data() );
		return WithAlpha( ROOM_PALETTE[index % std::size( ROOM_PALETTE )], alpha );
	}

	ImU32 SceneCanvas::AssignedColour( uint64_t gameObjectID, int alpha ) const
	{
		uint64_t roomID = 0;
		if( CarbonAudioHost::GetAssignedRoom( gameObjectID, roomID ) )
		{
			if( const LabRoom* room = m_scene.FindRoomByWwiseID( roomID ) )
			{
				return RoomColour( *room, alpha );
			}
		}
		return IM_COL32( 200, 200, 200, alpha );
	}

	void SceneCanvas::DrawBar()
	{
		ImGui::BeginDisabled( m_app.GetChecks().IsRunning() );
		const auto toolButton = [this]( const char* label, Tool tool, const char* tooltip ) {
			const bool active = m_tool == tool;
			if( active )
			{
				ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetStyleColorVec4( ImGuiCol_ButtonActive ) );
			}
			if( ImGui::Button( label ) )
			{
				m_tool = tool;
			}
			if( active )
			{
				ImGui::PopStyleColor();
			}
			ImGui::SetItemTooltip( "%s", tooltip );
		};
		toolButton( "Select", Tool::Select, "Select and move things (1)" );
		ImGui::SameLine();
		toolButton( "Room", Tool::AddRoom, "Draw a Spatial Audio room (2)" );
		ImGui::SameLine();
		toolButton( "Emitter", Tool::AddEmitter, "Place an emitter (3)" );
		ImGui::SameLine();
		toolButton( "Wall", Tool::AddWall, "Draw a wall that blocks sightlines, for occlusion (4)" );
		ImGui::SameLine();
		toolButton( "Portal", Tool::AddPortal, "Place a Spatial Audio portal, e.g. a door between two rooms (5)" );
		if( m_tool == Tool::AddEmitter )
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth( ImGui::GetFontSize() * 13.0f );
			EventCombo( m_folder, "##NewEmitterEvent", m_newEmitterEvent, true );
			ImGui::SetItemTooltip( "The event new emitters play" );
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled( "Click in the scene to place one." );
		}
		else if( m_tool == Tool::AddRoom )
		{
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled( "Drag in the scene to draw one." );
		}
		else if( m_tool == Tool::AddPortal )
		{
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled( "Click on a room's wall to place one across it." );
		}
		else if( m_tool == Tool::AddWall )
		{
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			if( CarbonAudioHost::GetObstructionOcclusionEnabled() )
			{
				ImGui::TextDisabled( "Drag in the scene to draw one." );
			}
			else
			{
				ImGui::TextColored( WARNING_COLOUR, "Drag to draw one. Occlusion is off, so walls block nothing." );
			}
		}
		ImGui::EndDisabled();

		// Right: the view.
		AlignRight( CheckboxWidth( "3D" ) + ButtonWidth( "Frame all" ) + CheckboxWidth( "Follow listener" ) + ImGui::GetStyle().ItemSpacing.x * 2.0f );
		bool is3D = m_view.is3D;
		if( ImGui::Checkbox( "3D", &is3D ) )
		{
			m_view.Set3D( is3D );
		}
		ImGui::SetItemTooltip( "The scene in 3D: right-drag turns the view, Shift-right-drag pans. Off: from above, in 2D" );
		ImGui::SameLine();
		if( ImGui::Button( "Frame all" ) )
		{
			m_frameAll = true;
		}
		ImGui::SetItemTooltip( "Fit the view around everything (F)" );
		ImGui::SameLine();
		if( ImGui::Checkbox( "Follow listener", &m_followListener ) && m_followListener )
		{
			m_view.yawDegrees = m_scene.listener.yawDegrees;
		}
		ImGui::SetItemTooltip( "Keep the listener in the middle of the view; in 3D, look the way it faces" );
	}

	void SceneCanvas::Draw()
	{
		m_focused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );
		m_view.origin = ImGui::GetCursorScreenPos();
		m_view.size = ImGui::GetContentRegionAvail();
		m_view.size.x = std::max( m_view.size.x, 1.0f );
		m_view.size.y = std::max( m_view.size.y, 1.0f );
		if( m_frameAll )
		{
			m_frameAll = false;
			FrameAll();
		}
		if( m_followListener )
		{
			m_view.viewX = m_scene.listener.x;
			m_view.viewZ = m_scene.listener.z;
			// In 3D the view turns with the listener, so its left and right are the listener's.
			m_view.yawDegrees = std::remainder( m_view.yawDegrees + m_scene.listener.yawDegrees - m_lastListenerYaw, 360.0f );
		}
		m_lastListenerYaw = m_scene.listener.yawDegrees;

		ImDrawList* draw = ImGui::GetWindowDrawList();
		m_view.draw = draw;
		m_view.uiScale = m_uiScale;

		ImGui::InvisibleButton( "canvasInput", m_view.size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
		const bool canvasHovered = ImGui::IsItemHovered();
		HandleInput();

		const ImVec2 mouse = ImGui::GetIO().MousePos;
		float mouseX, mouseZ;
		m_view.ToWorld( mouse, mouseX, mouseZ );
		const int hovered = canvasHovered ? Pick( mouse ) : LabScene::NO_ID;

		const ImVec2 canvasEnd( m_view.origin.x + m_view.size.x, m_view.origin.y + m_view.size.y );
		draw->PushClipRect( m_view.origin, canvasEnd, true );
		draw->AddRectFilled( m_view.origin, canvasEnd, IM_COL32( 24, 26, 30, 255 ) );

		// Grid: a line per metre when zoomed in, a stronger one every 10 m. In 3D it reaches a few times as far as the
		// camera is, with a line every 10 or 100 m further out.
		float left, top, right, bottom;
		int step = 1;
		if( m_view.is3D )
		{
			const float reach = std::clamp( m_view.distance * 3.0f, 30.0f, 3000.0f );
			while( reach / static_cast<float>( step ) > 100.0f )
			{
				step *= 10;
			}
			left = m_view.viewX - reach;
			right = m_view.viewX + reach;
			top = m_view.viewZ - reach;
			bottom = m_view.viewZ + reach;
		}
		else
		{
			m_view.ToWorld( m_view.origin, left, top );
			m_view.ToWorld( canvasEnd, right, bottom );
			step = m_view.pixelsPerMetre >= 8.0f ? 1 : 10;
		}
		for( int gx = static_cast<int>( std::floor( left ) ); gx <= static_cast<int>( std::ceil( right ) ); ++gx )
		{
			if( gx % step != 0 )
			{
				continue;
			}
			const ImU32 colour = gx == 0 ? IM_COL32( 90, 90, 100, 255 ) : gx % 10 == 0 ? IM_COL32( 55, 58, 66, 255 ) : IM_COL32( 36, 38, 44, 255 );
			m_view.Line( { static_cast<float>( gx ), 0.0f, top }, { static_cast<float>( gx ), 0.0f, bottom }, colour, 1.0f );
		}
		for( int gz = static_cast<int>( std::floor( top ) ); gz <= static_cast<int>( std::ceil( bottom ) ); ++gz )
		{
			if( gz % step != 0 )
			{
				continue;
			}
			const ImU32 colour = gz == 0 ? IM_COL32( 90, 90, 100, 255 ) : gz % 10 == 0 ? IM_COL32( 55, 58, 66, 255 ) : IM_COL32( 36, 38, 44, 255 );
			m_view.Line( { left, 0.0f, static_cast<float>( gz ) }, { right, 0.0f, static_cast<float>( gz ) }, colour, 1.0f );
		}

		// Labels skip spots another label already took; room names and the selected emitter's name always show. In 3D,
		// none for what is behind the camera.
		std::vector<LabelBox> labels;
		const auto label = [this, &labels, draw]( const ScenePoint& at, const ImVec2& offset, ImU32 colour, const char* text, bool always ) {
			ImVec2 position;
			if( !m_view.ToScreen( at, position ) )
			{
				return;
			}
			position = ImVec2( position.x + offset.x, position.y + offset.y );
			const ImVec2 size = ImGui::CalcTextSize( text );
			const LabelBox box = { position, ImVec2( position.x + size.x, position.y + size.y ) };
			if( !always && std::any_of( labels.begin(), labels.end(), [&box]( const LabelBox& other ) { return Overlaps( box, other ); } ) )
			{
				return;
			}
			labels.push_back( box );
			draw->AddText( position, colour, text );
		};

		// Rooms and portals show their outlines on the ground plane, where the listener and the emitters are; in 3D also
		// the boxes around them.
		for( const LabRoom& room : m_scene.rooms )
		{
			float cx[4], cz[4];
			RoomCorners( room, cx, cz );
			ScenePoint corners[4];
			GroundCorners( cx, cz, corners );
			const bool selected = m_selection.Contains( room.id );
			CarbonAudioHost::RoomStatus status;
			CarbonAudioHost::GetRoomStatus( room.object.p, status );
			m_view.Polygon( corners, 4, RoomColour( room, status.inWwise ? 40 : 15 ), selected ? IM_COL32( 255, 255, 255, 255 ) : RoomColour( room, status.inWwise ? 220 : 90 ), ( selected ? 3.0f : 1.5f ) * m_uiScale );
			if( m_view.is3D )
			{
				DrawBoxEdges( m_view, room.x, room.z, room.width, room.height, room.depth, room.yawDegrees, selected ? IM_COL32( 255, 255, 255, 160 ) : RoomColour( room, status.inWwise ? 110 : 45 ), 1.0f * m_uiScale );
			}
			const std::string name = room.settings.name + ( status.inWwise ? "" : " (not in Wwise)" );
			label( { room.x, 0.0f, room.z }, ImVec2( 0.0f, 0.0f ), RoomColour( room, 255 ), name.c_str(), true );
		}

		// Portals: an arrow to the front room, lines to the rooms Wwise has on each side, crossed when closed.
		for( const LabPortal& portal : m_scene.portals )
		{
			float cx[4], cz[4];
			PortalCorners( portal, cx, cz );
			ScenePoint corners[4];
			GroundCorners( cx, cz, corners );
			const bool selected = m_selection.Contains( portal.id );
			CarbonAudioHost::PortalStatus status;
			CarbonAudioHost::GetPortalStatus( portal.object.p, status );
			const ImU32 colour = status.inWwise ? PORTAL_COLOUR : PORTAL_NOT_IN_WWISE_COLOUR;
			m_view.Polygon( corners, 4, WithAlpha( colour, 50 ), selected ? IM_COL32( 255, 255, 255, 255 ) : colour, ( selected ? 3.0f : 1.5f ) * m_uiScale );
			if( m_view.is3D )
			{
				DrawBoxEdges( m_view, portal.x, portal.z, portal.width, portal.height, portal.depth, portal.yawDegrees, selected ? IM_COL32( 255, 255, 255, 160 ) : WithAlpha( colour, 140 ), 1.0f * m_uiScale );
			}
			if( !portal.settings.open )
			{
				m_view.Line( corners[0], corners[2], colour, 1.5f * m_uiScale );
				m_view.Line( corners[1], corners[3], colour, 1.5f * m_uiScale );
			}

			const float yaw = portal.yawDegrees * DEGREES_TO_RADIANS;
			const float fx = -std::sin( yaw ), fz = -std::cos( yaw );
			const float half = portal.depth * 0.5f;
			const ScenePoint centre = { portal.x, 0.0f, portal.z };
			const float head = std::max( 0.3f, 8.0f * m_uiScale / m_view.PixelsPerMetreAt( centre ) );
			const ScenePoint front = { portal.x + fx * half, 0.0f, portal.z + fz * half };
			const ScenePoint back = { portal.x - fx * half, 0.0f, portal.z - fz * half };
			const ScenePoint arrowHead[3] = {
				{ portal.x + fx * ( half + head ), 0.0f, portal.z + fz * ( half + head ) },
				{ portal.x + fx * half - fz * head * 0.6f, 0.0f, portal.z + fz * half + fx * head * 0.6f },
				{ portal.x + fx * half + fz * head * 0.6f, 0.0f, portal.z + fz * half - fx * head * 0.6f },
			};
			m_view.Line( centre, front, colour, 2.0f * m_uiScale );
			m_view.Polygon( arrowHead, 3, colour, 0, 0.0f );

			if( status.inWwise )
			{
				if( const LabRoom* room = m_scene.FindRoomByWwiseID( status.frontRoomID ) )
				{
					m_view.Line( front, { room->x, 0.0f, room->z }, RoomColour( *room, 140 ), 1.0f * m_uiScale );
				}
				if( const LabRoom* room = m_scene.FindRoomByWwiseID( status.backRoomID ) )
				{
					m_view.Line( back, { room->x, 0.0f, room->z }, RoomColour( *room, 140 ), 1.0f * m_uiScale );
				}
			}
			const std::string name = portal.settings.name + ( status.inWwise ? "" : " (not in Wwise)" );
			label( centre, ImVec2( 8.0f * m_uiScale, 4.0f * m_uiScale ), colour, name.c_str(), selected );
		}

		for( const LabWall& wall : m_scene.walls )
		{
			const bool selected = m_selection.Contains( wall.id );
			const ImU32 colour = selected ? IM_COL32( 255, 255, 255, 255 ) : WALL_COLOUR;
			if( m_view.is3D )
			{
				const float half = WALL_HEIGHT * 0.5f;
				const ScenePoint panel[4] = { { wall.x0, -half, wall.z0 }, { wall.x1, -half, wall.z1 }, { wall.x1, half, wall.z1 }, { wall.x0, half, wall.z0 } };
				m_view.Polygon( panel, 4, WithAlpha( WALL_COLOUR, 60 ), WithAlpha( colour, 180 ), 1.0f * m_uiScale );
			}
			m_view.Line( { wall.x0, 0.0f, wall.z0 }, { wall.x1, 0.0f, wall.z1 }, colour, ( selected ? 5.0f : 4.0f ) * m_uiScale );
		}

		// The sightlines CarbonAudio checked in its last pass: red where a wall blocks them.
		const ScenePoint listenerAt = { m_scene.listener.x, 0.0f, m_scene.listener.z };
		if( CarbonAudioHost::GetObstructionOcclusionEnabled() )
		{
			std::map<uint64_t, bool> blocked;
			for( const CarbonAudioHost::SightlineResult& result : CarbonAudioHost::GetLastSightlineResults() )
			{
				blocked[result.gameObjectID] = result.blocked;
			}
			for( const LabEmitter& emitter : m_scene.emitters )
			{
				const auto sightline = blocked.find( emitter.gameObjectID );
				if( sightline != blocked.end() )
				{
					m_view.Line( listenerAt, { emitter.x, 0.0f, emitter.z }, sightline->second ? SIGHTLINE_BLOCKED : SIGHTLINE_CLEAR, 1.5f * m_uiScale );
				}
			}
		}

		const float emitterSize = 7.0f * m_uiScale;
		const ImVec2 nameOffset( 9.0f * m_uiScale, -7.0f * m_uiScale );
		const ImU32 nameColour = IM_COL32( 220, 220, 220, 255 );
		for( const LabEmitter& emitter : m_scene.emitters )
		{
			const ScenePoint at = { emitter.x, 0.0f, emitter.z };
			const float radius = GetEventRadius( m_folder, emitter.eventName );
			if( radius > 0.0f )
			{
				m_view.Circle( at, radius, IM_COL32( 120, 120, 140, 70 ), 1.0f );
			}
			ImVec2 centre;
			if( !m_view.ToScreen( at, centre ) )
			{
				continue;
			}
			const bool selected = m_selection.Contains( emitter.id );
			draw->AddCircleFilled( centre, emitterSize, AssignedColour( emitter.gameObjectID, 255 ) );
			draw->AddCircle( centre, emitterSize + 1.0f * m_uiScale, selected ? IM_COL32( 255, 255, 255, 255 ) : IM_COL32( 20, 20, 20, 255 ), 0, ( selected ? 3.0f : 1.5f ) * m_uiScale );
		}
		// One selected emitter's name always shows; several get the first spots.
		for( const int id : m_selection.GetIDs() )
		{
			if( const LabEmitter* emitter = m_scene.FindEmitter( id ) )
			{
				label( { emitter->x, 0.0f, emitter->z }, nameOffset, nameColour, emitter->name.c_str(), m_selection.GetIDs().size() == 1 );
			}
		}
		if( const LabEmitter* emitter = m_scene.FindEmitter( hovered ) )
		{
			label( { emitter->x, 0.0f, emitter->z }, nameOffset, nameColour, emitter->name.c_str(), false );
		}
		for( const LabEmitter& emitter : m_scene.emitters )
		{
			label( { emitter.x, 0.0f, emitter.z }, nameOffset, nameColour, emitter.name.c_str(), false );
		}

		{
			// The listener: a triangle pointing where it faces.
			const LabListener& listener = m_scene.listener;
			const float yaw = listener.yawDegrees * DEGREES_TO_RADIANS;
			const float fx = std::sin( yaw ), fz = -std::cos( yaw );
			const float sx = -fz, sz = fx;
			const float size = 0.9f * std::max( 1.0f, 14.0f * m_uiScale / m_view.PixelsPerMetreAt( listenerAt ) );
			const ScenePoint triangle[3] = {
				{ listener.x + fx * size, 0.0f, listener.z + fz * size },
				{ listener.x - fx * size * 0.6f + sx * size * 0.6f, 0.0f, listener.z - fz * size * 0.6f + sz * size * 0.6f },
				{ listener.x - fx * size * 0.6f - sx * size * 0.6f, 0.0f, listener.z - fz * size * 0.6f - sz * size * 0.6f },
			};
			const bool selected = m_selection.Contains( LabScene::LISTENER_ID );
			m_view.Polygon( triangle, 3, AssignedColour( CarbonAudioHost::GetListenerID(), 255 ), selected ? IM_COL32( 255, 255, 255, 255 ) : IM_COL32( 20, 20, 20, 255 ), ( selected ? 3.0f : 1.5f ) * m_uiScale );
		}

		m_app.GetPage().DrawOverlay( m_app, m_view );

		if( m_gesture == Gesture::DrawRoom )
		{
			const ScenePoint outline[4] = { { m_pressX, 0.0f, m_pressZ }, { mouseX, 0.0f, m_pressZ }, { mouseX, 0.0f, mouseZ }, { m_pressX, 0.0f, mouseZ } };
			m_view.Polygon( outline, 4, 0, IM_COL32( 255, 255, 255, 160 ), 1.5f * m_uiScale );
		}
		else if( m_gesture == Gesture::DrawWall )
		{
			m_view.Line( { m_pressX, 0.0f, m_pressZ }, { mouseX, 0.0f, mouseZ }, WithAlpha( WALL_COLOUR, 160 ), 4.0f * m_uiScale );
		}
		else if( m_gesture == Gesture::BoxSelect )
		{
			const ImVec2 min( std::min( mouse.x, m_pressScreen.x ), std::min( mouse.y, m_pressScreen.y ) );
			const ImVec2 max( std::max( mouse.x, m_pressScreen.x ), std::max( mouse.y, m_pressScreen.y ) );
			draw->AddRectFilled( min, max, IM_COL32( 90, 150, 240, 40 ) );
			draw->AddRect( min, max, IM_COL32( 90, 150, 240, 200 ), 0.0f, 0, 1.0f * m_uiScale );
		}

		if( m_scene.rooms.empty() && m_scene.portals.empty() && m_scene.emitters.empty() && m_scene.walls.empty() && !m_app.GetChecks().IsRunning() )
		{
			const char* const hints[] = {
				"Press 2 and drag to draw a room, or 3 and click to place an emitter.",
				"Press 4 and drag to draw a wall: with Occlusion on, it muffles what is behind it.",
				"W A S D walks the listener, Q and E turn it.",
				"Right-click for what can be done there; ? in the toolbar lists every shortcut.",
			};
			float y = m_view.origin.y + ImGui::GetTextLineHeightWithSpacing();
			for( const char* hint : hints )
			{
				const float width = ImGui::CalcTextSize( hint ).x;
				draw->AddText( ImVec2( m_view.origin.x + ( m_view.size.x - width ) * 0.5f, y ), HINT_COLOUR, hint );
				y += ImGui::GetTextLineHeightWithSpacing();
			}
		}

		// In 3D a metre is not the same length everywhere.
		if( !m_view.is3D )
		{
			DrawScaleBar( draw );
		}
		draw->PopClipRect();

		DrawMenu();
		if( hovered != LabScene::NO_ID && m_gesture != Gesture::Move && !ImGui::IsAnyMouseDown() && !ImGui::IsPopupOpen( "SceneMenu" ) )
		{
			DrawTooltip( hovered );
		}
	}

	void SceneCanvas::DrawScaleBar( ImDrawList* draw )
	{
		// The longest 1, 2 or 5 times a power of ten that is no longer than about 90 pixels.
		const float target = 90.0f * m_uiScale;
		float metres = std::pow( 10.0f, std::floor( std::log10( target / m_view.pixelsPerMetre ) ) );
		if( metres * 5.0f * m_view.pixelsPerMetre <= target )
		{
			metres *= 5.0f;
		}
		else if( metres * 2.0f * m_view.pixelsPerMetre <= target )
		{
			metres *= 2.0f;
		}

		const float margin = 12.0f * m_uiScale;
		const float tick = 4.0f * m_uiScale;
		const ImVec2 start( m_view.origin.x + margin, m_view.origin.y + m_view.size.y - margin );
		const ImVec2 end( start.x + metres * m_view.pixelsPerMetre, start.y );
		const float thickness = 1.5f * m_uiScale;
		draw->AddLine( start, end, HINT_COLOUR, thickness );
		draw->AddLine( start, ImVec2( start.x, start.y - tick ), HINT_COLOUR, thickness );
		draw->AddLine( end, ImVec2( end.x, end.y - tick ), HINT_COLOUR, thickness );
		char text[32];
		std::snprintf( text, sizeof( text ), "%g m", metres );
		draw->AddText( ImVec2( start.x, start.y - tick - ImGui::GetTextLineHeightWithSpacing() ), HINT_COLOUR, text );
	}

	void SceneCanvas::DrawMenu()
	{
		if( !ImGui::BeginPopup( "SceneMenu" ) )
		{
			return;
		}
		const int emitters = m_selection.Count( LabKind::Emitter );
		const int rooms = m_selection.Count( LabKind::Room );
		const int portals = m_selection.Count( LabKind::Portal );
		const int walls = m_selection.Count( LabKind::Wall );
		if( emitters + rooms + portals + walls > 0 )
		{
			ImGui::TextDisabled( "%s", m_selection.Describe().c_str() );
			if( emitters > 0 )
			{
				if( ImGui::MenuItem( "Play" ) )
				{
					m_selection.Play( true );
				}
				if( ImGui::MenuItem( "Stop" ) )
				{
					m_selection.Play( false );
				}
				ImGui::Separator();
			}
			if( ImGui::MenuItem( "Remove", "Del" ) )
			{
				DeleteSelection();
			}
		}
		else
		{
			if( ImGui::MenuItem( "Place an emitter here" ) )
			{
				if( LabEmitter* emitter = m_scene.AddEmitter( m_menuX, m_menuZ, m_newEmitterEvent ) )
				{
					m_scene.Play( *emitter );
					m_selection.Select( emitter->id );
				}
			}
			ImGui::SetItemTooltip( "Plays %s", m_newEmitterEvent.empty() ? "nothing" : m_newEmitterEvent.c_str() );
			if( ImGui::MenuItem( "Move the listener here" ) )
			{
				m_scene.listener.x = m_menuX;
				m_scene.listener.z = m_menuZ;
			}
		}
		ImGui::EndPopup();
	}

	void SceneCanvas::DrawTooltip( int hovered )
	{
		if( !ImGui::BeginTooltip() )
		{
			return;
		}
		if( hovered == LabScene::LISTENER_ID )
		{
			ImGui::TextUnformatted( "Listener" );
			ImGui::TextDisabled( "Room: %s", AssignedRoomText( m_scene, CarbonAudioHost::GetListenerID() ).c_str() );
			ImGui::TextDisabled( "W A S D walks it, Q and E turn it." );
		}
		else if( const LabEmitter* emitter = m_scene.FindEmitter( hovered ) )
		{
			ImGui::TextUnformatted( emitter->name.c_str() );
			ImGui::TextDisabled( "%s", emitter->eventName.empty() ? "No event" : emitter->eventName.c_str() );
			ImGui::TextDisabled( "Room: %s", AssignedRoomText( m_scene, emitter->gameObjectID ).c_str() );
		}
		else if( const LabRoom* room = m_scene.FindRoom( hovered ) )
		{
			CarbonAudioHost::RoomStatus status;
			CarbonAudioHost::GetRoomStatus( room->object.p, status );
			ImGui::TextUnformatted( room->settings.name.c_str() );
			ImGui::TextDisabled( "%.1f by %.1f m%s", room->width, room->depth, status.inWwise ? "" : ", not in Wwise" );
		}
		else if( const LabPortal* portal = m_scene.FindPortal( hovered ) )
		{
			CarbonAudioHost::PortalStatus status;
			CarbonAudioHost::GetPortalStatus( portal->object.p, status );
			ImGui::TextUnformatted( portal->settings.name.c_str() );
			ImGui::TextDisabled( "%.1f m wide, %.1f m deep, %s", portal->width, portal->depth, portal->settings.open ? "open" : "closed" );
			if( status.inWwise )
			{
				ImGui::TextDisabled( "Front: %s", WwiseRoomText( m_scene, status.frontRoomID ).c_str() );
				ImGui::TextDisabled( "Back: %s", WwiseRoomText( m_scene, status.backRoomID ).c_str() );
			}
			else
			{
				ImGui::TextDisabled( "Not in Wwise: it needs a different room, or outdoors, on each side." );
			}
		}
		else if( const LabWall* wall = m_scene.FindWall( hovered ) )
		{
			ImGui::TextUnformatted( wall->name.c_str() );
			ImGui::TextDisabled( "%.1f m long%s", std::hypot( wall->x1 - wall->x0, wall->z1 - wall->z0 ), CarbonAudioHost::GetObstructionOcclusionEnabled() ? "" : "; Occlusion is off, so it blocks nothing" );
		}
		ImGui::EndTooltip();
	}

	void SceneCanvas::HandleInput()
	{
		ImGuiIO& io = ImGui::GetIO();
		const bool hovered = ImGui::IsItemHovered();
		float mouseX, mouseZ;
		const bool mouseOnGround = m_view.ToWorld( io.MousePos, mouseX, mouseZ );

		// The right or middle mouse button pans, keeping the ground under the cursor; in 3D the right one turns the view
		// unless Shift is down. The wheel zooms around the cursor.
		if( ImGui::IsItemActive() && m_view.is3D && ImGui::IsMouseDragging( ImGuiMouseButton_Right ) && !io.KeyShift )
		{
			m_view.Turn( io.MouseDelta.x * TURN_DEGREES_PER_PIXEL, io.MouseDelta.y * TURN_DEGREES_PER_PIXEL );
		}
		else if( ImGui::IsItemActive() && ( ImGui::IsMouseDragging( ImGuiMouseButton_Right ) || ImGui::IsMouseDragging( ImGuiMouseButton_Middle ) ) )
		{
			float fromX, fromZ;
			if( m_view.ToWorld( ImVec2( io.MousePos.x - io.MouseDelta.x, io.MousePos.y - io.MouseDelta.y ), fromX, fromZ ) && mouseOnGround )
			{
				m_view.viewX += fromX - mouseX;
				m_view.viewZ += fromZ - mouseZ;
			}
		}
		if( hovered && io.MouseWheel != 0.0f )
		{
			m_view.Zoom( io.MouseWheel );
			float afterX, afterZ;
			if( m_view.ToWorld( io.MousePos, afterX, afterZ ) && mouseOnGround )
			{
				m_view.viewX += mouseX - afterX;
				m_view.viewZ += mouseZ - afterZ;
			}
		}

		// Running checks own the scene: it can be looked at, not changed.
		if( m_app.GetChecks().IsRunning() )
		{
			m_gesture = Gesture::None;
			return;
		}

		const bool additive = io.KeyCtrl || io.KeyShift;
		if( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
		{
			m_pressX = mouseX;
			m_pressZ = mouseZ;
			m_pressScreen = io.MousePos;
			if( m_tool == Tool::AddEmitter )
			{
				if( LabEmitter* emitter = m_scene.AddEmitter( mouseX, mouseZ, m_newEmitterEvent ) )
				{
					m_scene.Play( *emitter );
					m_selection.Select( emitter->id );
				}
			}
			else if( m_tool == Tool::AddRoom )
			{
				m_gesture = Gesture::DrawRoom;
			}
			else if( m_tool == Tool::AddWall )
			{
				m_gesture = Gesture::DrawWall;
			}
			else if( m_tool == Tool::AddPortal )
			{
				const float reach = std::max( PORTAL_SNAP_METRES, PORTAL_SNAP_PIXELS * m_uiScale / m_view.PixelsPerMetreAt( { mouseX, 0.0f, mouseZ } ) );
				if( LabPortal* portal = m_scene.AddPortalOnWall( mouseX, mouseZ, reach ) )
				{
					m_selection.Select( portal->id );
					m_tool = Tool::Select;
				}
			}
			else
			{
				// The listener, emitters and portals move as soon as they are pressed; a room only once it is selected, so a
				// drag that starts inside a room can still draw a selection box.
				m_pressed = Pick( io.MousePos );
				const LabKind kind = m_scene.KindOf( m_pressed );
				const bool grabbed = kind == LabKind::Listener || kind == LabKind::Emitter || kind == LabKind::Portal || ( kind == LabKind::Room && m_selection.Contains( m_pressed ) );
				if( grabbed && !additive )
				{
					if( !m_selection.Contains( m_pressed ) )
					{
						m_selection.Select( m_pressed );
					}
					// Everything selected moves with the mouse.
					m_gesture = Gesture::Move;
					m_lastMouseX = mouseX;
					m_lastMouseZ = mouseZ;
				}
				else
				{
					m_gesture = Gesture::Click;
				}
			}
		}

		if( m_gesture == Gesture::Click && ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
		{
			m_gesture = Gesture::BoxSelect;
		}
		if( m_gesture == Gesture::Move )
		{
			for( const int id : m_selection.GetIDs() )
			{
				m_scene.Move( id, mouseX - m_lastMouseX, mouseZ - m_lastMouseZ );
			}
			m_lastMouseX = mouseX;
			m_lastMouseZ = mouseZ;
		}

		const float dragThreshold = io.MouseDragThreshold * io.MouseDragThreshold;
		if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
		{
			switch( m_gesture )
			{
			case Gesture::Click:
				// With Ctrl or Shift it adds what was clicked or takes it out, otherwise only it is selected.
				additive ? m_selection.Toggle( m_pressed ) : m_selection.Select( m_pressed );
				break;
			case Gesture::BoxSelect:
				SelectInBox( m_pressScreen, io.MousePos, additive );
				break;
			case Gesture::Move:
				// A click on one of several selected things, without moving them, selects only it.
				if( m_selection.GetIDs().size() > 1 && io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] < dragThreshold )
				{
					m_selection.Select( m_pressed );
				}
				break;
			case Gesture::DrawRoom:
			{
				const float width = std::fabs( mouseX - m_pressX );
				const float depth = std::fabs( mouseZ - m_pressZ );
				if( width >= MIN_ROOM_SIZE && depth >= MIN_ROOM_SIZE )
				{
					// New rooms get a reverb straight away, so walking in and out of them is audible.
					const std::string reverb = m_folder.auxBusses.empty() ? std::string() : m_folder.auxBusses.front();
					if( LabRoom* room = m_scene.AddRoom( ( mouseX + m_pressX ) * 0.5f, ( mouseZ + m_pressZ ) * 0.5f, width, depth, reverb ) )
					{
						m_selection.Select( room->id );
						m_tool = Tool::Select;
					}
				}
				break;
			}
			case Gesture::DrawWall:
				// The Wall tool stays on, so several walls can be drawn one after another.
				if( std::hypot( mouseX - m_pressX, mouseZ - m_pressZ ) >= MIN_WALL_LENGTH )
				{
					if( LabWall* wall = m_scene.AddWall( m_pressX, m_pressZ, mouseX, mouseZ ) )
					{
						m_selection.Select( wall->id );
					}
				}
				break;
			case Gesture::None:
				break;
			}
			m_gesture = Gesture::None;
		}

		// A right-click that did not pan opens the menu for what is under the cursor: all of the selection when it is
		// part of it.
		if( hovered && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) && io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < dragThreshold )
		{
			const int picked = Pick( io.MousePos );
			if( picked != LabScene::LISTENER_ID )
			{
				if( !m_selection.Contains( picked ) )
				{
					m_selection.Clear();
					if( picked != LabScene::NO_ID )
					{
						m_selection.Add( picked );
					}
				}
				m_menuX = mouseX;
				m_menuZ = mouseZ;
				ImGui::OpenPopup( "SceneMenu" );
			}
		}

		if( ( m_focused || hovered ) && !io.WantTextInput )
		{
			if( ImGui::IsKeyPressed( ImGuiKey_Delete ) )
			{
				DeleteSelection();
			}
			if( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
			{
				m_selection.Clear();
				m_tool = Tool::Select;
				m_gesture = Gesture::None;
			}
			if( ImGui::IsKeyPressed( ImGuiKey_1 ) )
			{
				m_tool = Tool::Select;
			}
			if( ImGui::IsKeyPressed( ImGuiKey_2 ) )
			{
				m_tool = Tool::AddRoom;
			}
			if( ImGui::IsKeyPressed( ImGuiKey_3 ) )
			{
				m_tool = Tool::AddEmitter;
			}
			if( ImGui::IsKeyPressed( ImGuiKey_4 ) )
			{
				m_tool = Tool::AddWall;
			}
			if( ImGui::IsKeyPressed( ImGuiKey_5 ) )
			{
				m_tool = Tool::AddPortal;
			}
			if( ImGui::IsKeyPressed( ImGuiKey_F ) )
			{
				FrameAll();
			}
		}
	}

	void SceneCanvas::HandleListenerKeys()
	{
		ImGuiIO& io = ImGui::GetIO();
		if( io.WantTextInput || m_app.GetChecks().IsRunning() )
		{
			return;
		}

		LabListener& listener = m_scene.listener;
		if( ImGui::IsKeyDown( ImGuiKey_Q ) )
		{
			listener.yawDegrees -= 120.0f * io.DeltaTime;
		}
		if( ImGui::IsKeyDown( ImGuiKey_E ) )
		{
			listener.yawDegrees += 120.0f * io.DeltaTime;
		}
		listener.yawDegrees = std::remainder( listener.yawDegrees, 360.0f );

		float forward = 0.0f;
		float right = 0.0f;
		forward += ImGui::IsKeyDown( ImGuiKey_W ) ? 1.0f : 0.0f;
		forward -= ImGui::IsKeyDown( ImGuiKey_S ) ? 1.0f : 0.0f;
		right += ImGui::IsKeyDown( ImGuiKey_D ) ? 1.0f : 0.0f;
		right -= ImGui::IsKeyDown( ImGuiKey_A ) ? 1.0f : 0.0f;
		if( forward == 0.0f && right == 0.0f )
		{
			return;
		}

		// Facing is clockwise from up the screen; right is a quarter turn further.
		const float yaw = listener.yawDegrees * DEGREES_TO_RADIANS;
		const float facingX = std::sin( yaw ), facingZ = -std::cos( yaw );
		const float rightX = std::cos( yaw ), rightZ = std::sin( yaw );
		const float length = std::sqrt( forward * forward + right * right );
		const float step = m_walkSpeed * ( io.KeyShift ? 3.0f : 1.0f ) * io.DeltaTime / length;
		listener.x += ( facingX * forward + rightX * right ) * step;
		listener.z += ( facingZ * forward + rightZ * right ) * step;
	}

	int SceneCanvas::Pick( const ImVec2& screen ) const
	{
		const float pickRadius = PICK_RADIUS_PIXELS * m_uiScale;
		const auto isClose = [this, &screen, pickRadius]( float x, float z ) {
			ImVec2 at;
			return m_view.ToScreen( { x, 0.0f, z }, at ) && std::hypot( at.x - screen.x, at.y - screen.y ) <= pickRadius;
		};
		ScenePoint from, direction;
		m_view.Ray( screen, from, direction );

		if( isClose( m_scene.listener.x, m_scene.listener.z ) )
		{
			return LabScene::LISTENER_ID;
		}
		// The last emitter is drawn on top, so it is the one picked.
		for( auto emitter = m_scene.emitters.rbegin(); emitter != m_scene.emitters.rend(); ++emitter )
		{
			if( isClose( emitter->x, emitter->z ) )
			{
				return emitter->id;
			}
		}
		for( auto portal = m_scene.portals.rbegin(); portal != m_scene.portals.rend(); ++portal )
		{
			if( RayHitsBox( from, direction, portal->x, portal->z, portal->width, portal->height, portal->depth, portal->yawDegrees ) )
			{
				return portal->id;
			}
		}
		// Near its line on the ground plane, or in 3D anywhere on its panel.
		for( auto wall = m_scene.walls.rbegin(); wall != m_scene.walls.rend(); ++wall )
		{
			ImVec2 start, end;
			if( m_view.ToScreen( { wall->x0, 0.0f, wall->z0 }, { wall->x1, 0.0f, wall->z1 }, start, end ) && DistanceToSegment( screen.x, screen.y, start.x, start.y, end.x, end.y ) <= pickRadius )
			{
				return wall->id;
			}
			const float dx = wall->x1 - wall->x0;
			const float dz = wall->z1 - wall->z0;
			if( m_view.is3D && RayHitsBox( from, direction, ( wall->x0 + wall->x1 ) * 0.5f, ( wall->z0 + wall->z1 ) * 0.5f, std::hypot( dx, dz ), WALL_HEIGHT, 0.0f, std::atan2( -dz, dx ) / DEGREES_TO_RADIANS ) )
			{
				return wall->id;
			}
		}
		// The smallest room under the cursor, so nested rooms can be picked.
		const LabRoom* best = nullptr;
		for( const LabRoom& room : m_scene.rooms )
		{
			if( RayHitsBox( from, direction, room.x, room.z, room.width, room.height, room.depth, room.yawDegrees ) && ( best == nullptr || room.width * room.depth < best->width * best->depth ) )
			{
				best = &room;
			}
		}
		return best != nullptr ? best->id : LabScene::NO_ID;
	}

	void SceneCanvas::SelectInBox( const ImVec2& a, const ImVec2& b, bool add )
	{
		const ImVec2 min( std::min( a.x, b.x ), std::min( a.y, b.y ) );
		const ImVec2 max( std::max( a.x, b.x ), std::max( a.y, b.y ) );
		const auto inside = [this, min, max]( float x, float z ) {
			ImVec2 at;
			return m_view.ToScreen( { x, 0.0f, z }, at ) && at.x >= min.x && at.x <= max.x && at.y >= min.y && at.y <= max.y;
		};

		if( !add )
		{
			m_selection.Clear();
		}
		for( const LabEmitter& emitter : m_scene.emitters )
		{
			if( inside( emitter.x, emitter.z ) )
			{
				m_selection.Add( emitter.id );
			}
		}
		for( const LabRoom& room : m_scene.rooms )
		{
			float cx[4], cz[4];
			RoomCorners( room, cx, cz );
			if( inside( cx[0], cz[0] ) && inside( cx[1], cz[1] ) && inside( cx[2], cz[2] ) && inside( cx[3], cz[3] ) )
			{
				m_selection.Add( room.id );
			}
		}
		for( const LabPortal& portal : m_scene.portals )
		{
			float cx[4], cz[4];
			PortalCorners( portal, cx, cz );
			if( inside( cx[0], cz[0] ) && inside( cx[1], cz[1] ) && inside( cx[2], cz[2] ) && inside( cx[3], cz[3] ) )
			{
				m_selection.Add( portal.id );
			}
		}
		for( const LabWall& wall : m_scene.walls )
		{
			if( inside( wall.x0, wall.z0 ) && inside( wall.x1, wall.z1 ) )
			{
				m_selection.Add( wall.id );
			}
		}
		if( !m_selection.IsEmpty() )
		{
			m_selection.RequestShow();
		}
	}

	void SceneCanvas::DeleteSelection()
	{
		m_selection.RemoveFromScene();
		m_gesture = Gesture::None;
	}

	void SceneCanvas::FrameAll()
	{
		float minX = m_scene.listener.x;
		float maxX = minX;
		float minZ = m_scene.listener.z;
		float maxZ = minZ;
		const auto include = [&]( float x, float z ) {
			minX = std::min( minX, x );
			maxX = std::max( maxX, x );
			minZ = std::min( minZ, z );
			maxZ = std::max( maxZ, z );
		};
		for( const LabEmitter& emitter : m_scene.emitters )
		{
			include( emitter.x, emitter.z );
		}
		for( const LabRoom& room : m_scene.rooms )
		{
			float cx[4], cz[4];
			RoomCorners( room, cx, cz );
			for( int i = 0; i < 4; ++i )
			{
				include( cx[i], cz[i] );
			}
		}
		for( const LabPortal& portal : m_scene.portals )
		{
			float cx[4], cz[4];
			PortalCorners( portal, cx, cz );
			for( int i = 0; i < 4; ++i )
			{
				include( cx[i], cz[i] );
			}
		}
		for( const LabWall& wall : m_scene.walls )
		{
			include( wall.x0, wall.z0 );
			include( wall.x1, wall.z1 );
		}
		m_view.Fit( ( minX + maxX ) * 0.5f, ( minZ + maxZ ) * 0.5f, std::max( maxX - minX, MIN_FRAMED_SIZE ), std::max( maxZ - minZ, MIN_FRAMED_SIZE ) );
	}
}
