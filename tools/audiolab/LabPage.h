// Copyright © 2026 CCP ehf.

#pragma once

#include "LabChecks.h"

#include <imgui.h>

#include <memory>
#include <vector>

namespace AudioLab
{
	class LabApp;

	/// Where the scene is on screen and which part of the ground plane it shows, around (viewX, viewZ).
	struct SceneView
	{
		ImDrawList* draw = nullptr;
		ImVec2 origin = ImVec2( 0.0f, 0.0f );
		ImVec2 size = ImVec2( 1.0f, 1.0f );
		float viewX = 0.0f;
		float viewZ = 0.0f;
		float pixelsPerMetre = 1.0f;
		float uiScale = 1.0f;

		ImVec2 ToScreen( float x, float z ) const
		{
			return ImVec2( origin.x + size.x * 0.5f + ( x - viewX ) * pixelsPerMetre, origin.y + size.y * 0.5f + ( z - viewZ ) * pixelsPerMetre );
		}

		void ToWorld( const ImVec2& screen, float& x, float& z ) const
		{
			x = viewX + ( screen.x - origin.x - size.x * 0.5f ) / pixelsPerMetre;
			z = viewZ + ( screen.y - origin.y - size.y * 0.5f ) / pixelsPerMetre;
		}
	};

	/// A page of the lab: one carbon-audio feature with its own scene, controls, overlay and checks.
	class LabPage
	{
	public:
		virtual ~LabPage() = default;

		/// Short and lowercase; `--check <id>` runs the page's checks.
		virtual const char* GetID() const = 0;
		virtual const char* GetName() const = 0;
		/// What the feature does and what to listen and look for.
		virtual const char* GetHelp() const = 0;

		/// Sets up the page's feature on a cleared scene with the lab's default switches, for its demo and its checks.
		virtual void Enter( LabApp& ) {}
		/// After Enter, when the page opens for the user rather than for a check: a scene that shows the feature.
		virtual void BuildDemo( LabApp& ) {}
		/// Puts back what Enter changed beyond the scene and the switches the lab resets.
		virtual void Leave( LabApp& ) {}
		/// Every frame, before the scene is sent.
		virtual void Update( LabApp& ) {}
		/// The page's own controls.
		virtual void DrawControls( LabApp& ) {}
		/// Drawn over the scene.
		virtual void DrawOverlay( LabApp&, const SceneView& ) {}
		virtual std::vector<LabCheck> GetChecks() { return {}; }
	};

	/// Every page, the Sandbox first.
	std::vector<std::unique_ptr<LabPage>> CreatePages();
}
