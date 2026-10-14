// Copyright © 2026 CCP ehf.

#pragma once

#include <memory>
#include <string>

namespace AudioLab
{
	/// The lab's window with Dear ImGui, on each platform's own window system and renderer:
	/// Win32 and Direct3D 11 on Windows, Cocoa and Metal on macOS.
	class LabWindow
	{
	public:
		LabWindow();
		~LabWindow();

		bool Create( const char* title, int width, int height, std::string& error );
		/// Handles window events and starts an ImGui frame; false once the window was asked to close.
		bool BeginFrame();
		void EndFrame();
		/// Saves the ImGui layout and closes the window; Blue ends the process without destructors.
		void Destroy();
		/// Scale for sizes the lab draws itself, so they match the ImGui style on high DPI screens.
		float GetUiScale() const;

		/// The platform's window state, defined next to its implementation.
		struct Impl;

	private:
		std::unique_ptr<Impl> m_impl;
	};

	/// Shared by both platforms: the ini file in the user's config folder, style and font for a scale.
	void SetUpImGui( std::string& iniPath );
	/// sizeScale scales the style; fontScale is the pixel density the font is rasterised for.
	void ApplyImGuiScale( float sizeScale, float fontScale );
}
