// Copyright © 2026 CCP ehf.

#pragma once

#include <imgui.h>

namespace AudioLab
{
	/// A point in the lab's world: x to the right, y up, z down the screen.
	struct ScenePoint
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	/// Where the scene is on screen and how it is seen: from above, around (viewX, viewZ), or in 3D by a camera that
	/// looks at that point on the ground plane. From above, y is left out.
	struct SceneView
	{
		static constexpr float FIELD_OF_VIEW_DEGREES = 50.0f;
		static constexpr float MIN_PIXELS_PER_METRE = 1.0f;
		static constexpr float MAX_PIXELS_PER_METRE = 200.0f;
		static constexpr float MIN_DISTANCE = 2.0f;
		static constexpr float MAX_DISTANCE = 2000.0f;
		static constexpr float MIN_PITCH_DEGREES = 5.0f;

		ImDrawList* draw = nullptr;
		ImVec2 origin = ImVec2( 0.0f, 0.0f );
		ImVec2 size = ImVec2( 1.0f, 1.0f );
		float viewX = 0.0f;
		float viewZ = 0.0f;
		/// From above.
		float pixelsPerMetre = 1.0f;
		float uiScale = 1.0f;

		bool is3D = true;
		/// In 3D: the camera faces yawDegrees, clockwise from up the screen like the listener, looks down pitchDegrees,
		/// 90 straight down, and is distance metres from (viewX, 0, viewZ).
		float yawDegrees = 0.0f;
		float pitchDegrees = 50.0f;
		float distance = 50.0f;

		/// Switches between from above and 3D, keeping what is in the middle of the view about the same size.
		void Set3D( bool on );
		/// Centres the view on the area of the ground plane and fits it in.
		void Fit( float centreX, float centreZ, float width, float depth );
		/// Zooms in by steps of the mouse wheel, out when negative.
		void Zoom( float steps );
		/// In 3D: turns the camera around what it looks at.
		void Turn( float yawDegrees, float pitchDegrees );

		/// False when the point is behind the camera.
		bool ToScreen( const ScenePoint& point, ImVec2& screen ) const;
		/// The part of the segment in front of the camera; false when none of it is.
		bool ToScreen( const ScenePoint& a, const ScenePoint& b, ImVec2& screenA, ImVec2& screenB ) const;
		/// Where the screen point is on the ground plane. In 3D, false near and above the horizon, where it gives a far
		/// point on the ground in that direction.
		bool ToWorld( const ImVec2& screen, float& x, float& z ) const;
		/// The line of sight through the screen point; from above, straight down.
		void Ray( const ImVec2& screen, ScenePoint& from, ScenePoint& direction ) const;
		/// Pixels a metre takes at the point: in 3D, fewer the further it is.
		float PixelsPerMetreAt( const ScenePoint& point ) const;

		/// The part of the line in front of the camera.
		void Line( const ScenePoint& a, const ScenePoint& b, ImU32 colour, float thickness ) const;
		/// A flat convex polygon of up to 8 points, the part in front of the camera. 0 for either colour leaves out the
		/// fill or the outline.
		void Polygon( const ScenePoint* points, int count, ImU32 fill, ImU32 outline, float thickness ) const;
		/// A level circle around the point.
		void Circle( const ScenePoint& centre, float radius, ImU32 colour, float thickness ) const;
	};
}
