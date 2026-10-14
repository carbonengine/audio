// Copyright © 2026 CCP ehf.

#pragma once

#include "LabPage.h"
#include "LabSelection.h"

#include <imgui.h>

#include <cstdint>
#include <string>

namespace AudioLab
{
	class LabApp;
	struct SoundBankFolder;

	/// The scene, from above or in 3D: its tool bar, the view, the drawing, and the mouse and keys that select, move,
	/// place and draw things.
	class SceneCanvas
	{
	public:
		SceneCanvas( LabApp& app, Selection& selection );

		/// Scale for the sizes the canvas draws itself, from LabWindow::GetUiScale().
		void SetUiScale( float scale ) { m_uiScale = scale; }
		/// After a page opened: ends what the mouse was doing and frames the new scene.
		void Reset();
		/// Every frame, wherever the focus is: WASD walks the listener relative to its facing, Q/E turn it; ignored
		/// while typing into a field.
		void HandleListenerKeys();
		/// The tools and the view's controls, above the scene.
		void DrawBar();
		/// The scene, filling the rest of the window.
		void Draw();
		/// How fast W A S D walk the listener, in metres per second; Shift triples it.
		float& WalkSpeed() { return m_walkSpeed; }

	private:
		enum class Tool
		{
			Select,
			AddRoom,
			AddEmitter,
			AddWall,
			AddPortal,
		};

		/// What the left mouse button is doing in the scene, from press to release.
		enum class Gesture
		{
			None,
			/// Pressed on a room that is not selected, or on nothing: a click selects, a drag becomes BoxSelect.
			Click,
			BoxSelect,
			/// Moving the selection.
			Move,
			DrawRoom,
			DrawWall,
		};

		void HandleInput();
		void DrawMenu();
		void DrawTooltip( int hovered );
		void DrawScaleBar( ImDrawList* draw );
		/// The ID of what is under the screen point: the listener, then an emitter, then a portal, then a wall, then the
		/// smallest room; LabScene::NO_ID for nothing.
		int Pick( const ImVec2& screen ) const;
		/// Selects the emitters inside the box on screen and the rooms, portals and walls whose outlines on the ground
		/// plane are wholly inside it; the listener is left out.
		void SelectInBox( const ImVec2& a, const ImVec2& b, bool add );
		void DeleteSelection();
		/// Fits the view around the listener, the emitters, the rooms, the portals and the walls.
		void FrameAll();
		ImU32 RoomColour( const LabRoom& room, int alpha ) const;
		/// Colour of the room CarbonAudio placed the game object in; grey for outdoors or Wwise's own containment.
		ImU32 AssignedColour( uint64_t gameObjectID, int alpha ) const;

		LabApp& m_app;
		LabScene& m_scene;
		const SoundBankFolder& m_folder;
		Selection& m_selection;
		float m_uiScale = 1.0f;

		SceneView m_view;
		bool m_focused = false;
		bool m_frameAll = true;
		bool m_followListener = false;
		/// The listener's facing last frame, for turning the 3D view with it.
		float m_lastListenerYaw = 0.0f;
		Tool m_tool = Tool::Select;
		std::string m_newEmitterEvent;
		float m_walkSpeed = 15.0f;
		/// Where the context menu opened.
		float m_menuX = 0.0f;
		float m_menuZ = 0.0f;

		Gesture m_gesture = Gesture::None;
		/// What the left button went down on, and where, on the ground plane and on screen.
		int m_pressed = LabScene::NO_ID;
		float m_pressX = 0.0f;
		float m_pressZ = 0.0f;
		ImVec2 m_pressScreen = ImVec2( 0.0f, 0.0f );
		/// Where the mouse was on the ground plane last frame, for moving the selection with it.
		float m_lastMouseX = 0.0f;
		float m_lastMouseZ = 0.0f;
	};
}
