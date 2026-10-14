// Copyright © 2026 CCP ehf.

#include "LabSceneView.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	using AudioLab::ScenePoint;
	using AudioLab::SceneView;

	constexpr float DEGREES_TO_RADIANS = 3.14159265358979f / 180.0f;
	/// In 3D, what is nearer the camera than this, in metres, is behind it.
	constexpr float NEAR_DISTANCE = 0.05f;
	/// From above, lines of sight start this high.
	constexpr float ABOVE_HEIGHT = 10000.0f;
	/// In 3D, the ground plane ends this many times the camera's distance away, for the screen points near and above
	/// the horizon.
	constexpr float GROUND_REACH = 10.0f;
	constexpr int MAX_POLYGON_POINTS = 8;
	constexpr int CIRCLE_SEGMENTS = 64;
	/// How much one step of the mouse wheel zooms.
	constexpr float ZOOM_STEP = 1.15f;

	ScenePoint operator+( const ScenePoint& a, const ScenePoint& b )
	{
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}

	ScenePoint operator-( const ScenePoint& a, const ScenePoint& b )
	{
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}

	ScenePoint operator*( const ScenePoint& a, float s )
	{
		return { a.x * s, a.y * s, a.z * s };
	}

	float Dot( const ScenePoint& a, const ScenePoint& b )
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	float HalfViewTangent()
	{
		return std::tan( SceneView::FIELD_OF_VIEW_DEGREES * 0.5f * DEGREES_TO_RADIANS );
	}

	/// The 3D camera: where it is, its axes in the world and where its middle is on screen.
	struct Camera
	{
		ScenePoint position;
		ScenePoint right;
		ScenePoint up;
		ScenePoint forward;
		/// Pixels a metre takes a metre in front of it.
		float focal = 1.0f;
		ImVec2 centre;
	};

	Camera GetCamera( const SceneView& view )
	{
		const float yaw = view.yawDegrees * DEGREES_TO_RADIANS;
		const float pitch = view.pitchDegrees * DEGREES_TO_RADIANS;
		const float sy = std::sin( yaw ), cy = std::cos( yaw );
		const float sp = std::sin( pitch ), cp = std::cos( pitch );

		Camera camera;
		camera.forward = { sy * cp, -sp, -cy * cp };
		camera.right = { cy, 0.0f, sy };
		camera.up = { sy * sp, cp, -cy * sp };
		camera.position = ScenePoint{ view.viewX, 0.0f, view.viewZ } - camera.forward * view.distance;
		camera.focal = view.size.y * 0.5f / HalfViewTangent();
		camera.centre = ImVec2( view.origin.x + view.size.x * 0.5f, view.origin.y + view.size.y * 0.5f );
		return camera;
	}

	/// The point along the camera's axes: right, up and how far in front of it.
	ScenePoint ToCamera( const Camera& camera, const ScenePoint& point )
	{
		const ScenePoint offset = point - camera.position;
		return { Dot( offset, camera.right ), Dot( offset, camera.up ), Dot( offset, camera.forward ) };
	}

	/// A point along the camera's axes, in front of it, on screen.
	ImVec2 Project( const Camera& camera, const ScenePoint& point )
	{
		return ImVec2( camera.centre.x + point.x * camera.focal / point.z, camera.centre.y - point.y * camera.focal / point.z );
	}

	/// Where the segment between two points along the camera's axes, one in front of it and one behind, comes out in
	/// front.
	ScenePoint AtNear( const ScenePoint& a, const ScenePoint& b )
	{
		const float t = ( NEAR_DISTANCE - a.z ) / ( b.z - a.z );
		return { a.x + ( b.x - a.x ) * t, a.y + ( b.y - a.y ) * t, NEAR_DISTANCE };
	}

	/// Twice the polygon's area, positive when it goes clockwise on screen.
	float SignedArea( const ImVec2* points, int count )
	{
		float area = 0.0f;
		for( int i = 0; i < count; ++i )
		{
			const ImVec2& a = points[i];
			const ImVec2& b = points[( i + 1 ) % count];
			area += a.x * b.y - b.x * a.y;
		}
		return area;
	}
}

namespace AudioLab
{
	void SceneView::Set3D( bool on )
	{
		if( on == is3D )
		{
			return;
		}
		// Seen straight down, the camera at distance shows as much of the ground across the view's height as
		// pixelsPerMetre does.
		if( on )
		{
			distance = std::clamp( size.y / ( 2.0f * HalfViewTangent() * pixelsPerMetre ), MIN_DISTANCE, MAX_DISTANCE );
		}
		else
		{
			pixelsPerMetre = std::clamp( size.y / ( 2.0f * HalfViewTangent() * distance ), MIN_PIXELS_PER_METRE, MAX_PIXELS_PER_METRE );
		}
		is3D = on;
	}

	void SceneView::Fit( float centreX, float centreZ, float width, float depth )
	{
		viewX = centreX;
		viewZ = centreZ;
		if( is3D )
		{
			// Far enough that the longer side of the area fits across the narrower side of the view.
			const float tangent = HalfViewTangent() * std::min( 1.0f, size.x / size.y );
			distance = std::clamp( 0.5f * std::max( width, depth ) / ( 0.85f * tangent ), MIN_DISTANCE, MAX_DISTANCE );
		}
		else
		{
			pixelsPerMetre = std::clamp( 0.85f * std::min( size.x / width, size.y / depth ), MIN_PIXELS_PER_METRE, MAX_PIXELS_PER_METRE );
		}
	}

	void SceneView::Zoom( float steps )
	{
		const float factor = std::pow( ZOOM_STEP, steps );
		if( is3D )
		{
			distance = std::clamp( distance / factor, MIN_DISTANCE, MAX_DISTANCE );
		}
		else
		{
			pixelsPerMetre = std::clamp( pixelsPerMetre * factor, MIN_PIXELS_PER_METRE, MAX_PIXELS_PER_METRE );
		}
	}

	void SceneView::Turn( float yaw, float pitch )
	{
		yawDegrees = std::remainder( yawDegrees + yaw, 360.0f );
		pitchDegrees = std::clamp( pitchDegrees + pitch, MIN_PITCH_DEGREES, 90.0f );
	}

	bool SceneView::ToScreen( const ScenePoint& point, ImVec2& screen ) const
	{
		if( !is3D )
		{
			screen = ImVec2( origin.x + size.x * 0.5f + ( point.x - viewX ) * pixelsPerMetre, origin.y + size.y * 0.5f + ( point.z - viewZ ) * pixelsPerMetre );
			return true;
		}
		const Camera camera = GetCamera( *this );
		const ScenePoint seen = ToCamera( camera, point );
		if( seen.z < NEAR_DISTANCE )
		{
			return false;
		}
		screen = Project( camera, seen );
		return true;
	}

	bool SceneView::ToScreen( const ScenePoint& a, const ScenePoint& b, ImVec2& screenA, ImVec2& screenB ) const
	{
		if( !is3D )
		{
			ToScreen( a, screenA );
			ToScreen( b, screenB );
			return true;
		}
		const Camera camera = GetCamera( *this );
		ScenePoint seenA = ToCamera( camera, a );
		ScenePoint seenB = ToCamera( camera, b );
		if( seenA.z < NEAR_DISTANCE && seenB.z < NEAR_DISTANCE )
		{
			return false;
		}
		if( seenA.z < NEAR_DISTANCE )
		{
			seenA = AtNear( seenA, seenB );
		}
		else if( seenB.z < NEAR_DISTANCE )
		{
			seenB = AtNear( seenA, seenB );
		}
		screenA = Project( camera, seenA );
		screenB = Project( camera, seenB );
		return true;
	}

	bool SceneView::ToWorld( const ImVec2& screen, float& x, float& z ) const
	{
		if( !is3D )
		{
			x = viewX + ( screen.x - origin.x - size.x * 0.5f ) / pixelsPerMetre;
			z = viewZ + ( screen.y - origin.y - size.y * 0.5f ) / pixelsPerMetre;
			return true;
		}
		ScenePoint from, direction;
		Ray( screen, from, direction );
		const float farthest = GROUND_REACH * distance / std::sqrt( Dot( direction, direction ) );
		const float ground = direction.y < 0.0f ? -from.y / direction.y : std::numeric_limits<float>::max();
		const float t = std::min( ground, farthest );
		x = from.x + direction.x * t;
		z = from.z + direction.z * t;
		return ground <= farthest;
	}

	void SceneView::Ray( const ImVec2& screen, ScenePoint& from, ScenePoint& direction ) const
	{
		if( !is3D )
		{
			float x, z;
			ToWorld( screen, x, z );
			from = { x, ABOVE_HEIGHT, z };
			direction = { 0.0f, -1.0f, 0.0f };
			return;
		}
		const Camera camera = GetCamera( *this );
		from = camera.position;
		direction = camera.forward + camera.right * ( ( screen.x - camera.centre.x ) / camera.focal ) + camera.up * ( ( camera.centre.y - screen.y ) / camera.focal );
	}

	float SceneView::PixelsPerMetreAt( const ScenePoint& point ) const
	{
		if( !is3D )
		{
			return pixelsPerMetre;
		}
		const Camera camera = GetCamera( *this );
		return camera.focal / std::max( ToCamera( camera, point ).z, NEAR_DISTANCE );
	}

	void SceneView::Line( const ScenePoint& a, const ScenePoint& b, ImU32 colour, float thickness ) const
	{
		ImVec2 screenA, screenB;
		if( ToScreen( a, b, screenA, screenB ) )
		{
			draw->AddLine( screenA, screenB, colour, thickness );
		}
	}

	void SceneView::Polygon( const ScenePoint* points, int count, ImU32 fill, ImU32 outline, float thickness ) const
	{
		count = std::min( count, MAX_POLYGON_POINTS );
		// Cutting off what is behind the camera adds at most one corner to a convex polygon.
		ImVec2 screen[MAX_POLYGON_POINTS + 1];
		int screenCount = 0;
		if( !is3D )
		{
			for( int i = 0; i < count; ++i )
			{
				ToScreen( points[i], screen[screenCount++] );
			}
		}
		else
		{
			const Camera camera = GetCamera( *this );
			for( int i = 0; i < count; ++i )
			{
				const ScenePoint a = ToCamera( camera, points[i] );
				const ScenePoint b = ToCamera( camera, points[( i + 1 ) % count] );
				if( a.z >= NEAR_DISTANCE )
				{
					screen[screenCount++] = Project( camera, a );
				}
				if( ( a.z >= NEAR_DISTANCE ) != ( b.z >= NEAR_DISTANCE ) )
				{
					screen[screenCount++] = Project( camera, AtNear( a, b ) );
				}
			}
		}
		if( screenCount < 3 )
		{
			return;
		}
		// ImGui fills shapes that go clockwise on screen.
		if( SignedArea( screen, screenCount ) < 0.0f )
		{
			std::reverse( screen, screen + screenCount );
		}
		if( fill != 0 )
		{
			draw->AddConvexPolyFilled( screen, screenCount, fill );
		}
		if( outline != 0 )
		{
			draw->AddPolyline( screen, screenCount, outline, ImDrawFlags_Closed, thickness );
		}
	}

	void SceneView::Circle( const ScenePoint& centre, float radius, ImU32 colour, float thickness ) const
	{
		if( !is3D )
		{
			ImVec2 screen;
			ToScreen( centre, screen );
			draw->AddCircle( screen, radius * pixelsPerMetre, colour, CIRCLE_SEGMENTS, thickness );
			return;
		}
		ScenePoint from = { centre.x + radius, centre.y, centre.z };
		for( int i = 1; i <= CIRCLE_SEGMENTS; ++i )
		{
			const float angle = 2.0f * 3.14159265358979f * static_cast<float>( i ) / CIRCLE_SEGMENTS;
			const ScenePoint to = { centre.x + radius * std::cos( angle ), centre.y, centre.z + radius * std::sin( angle ) };
			Line( from, to, colour, thickness );
			from = to;
		}
	}
}
