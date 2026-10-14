// Copyright © 2026 CCP ehf.

#pragma once

#include "LabApp.h"
#include "LabEnginePanel.h"
#include "LabLogPanel.h"
#include "LabSceneCanvas.h"
#include "LabSelection.h"
#include "LabSelectionPanel.h"
#include "LabSplash.h"

#include <imgui.h>

#include <string>

namespace AudioLab
{
	/// The lab's user interface: a toolbar and a status bar around the scene, a sidebar with the open page, the
	/// selection and the engine's settings, and a log under the scene, with the Splash over it all at first. LabUi is
	/// the frame and the page tab; the scene is SceneCanvas, the other tabs SelectionPanel and EnginePanel, the log
	/// LogPanel.
	class LabUi
	{
	public:
		explicit LabUi( LabApp& app );

		/// Scale for the sizes the lab draws itself, from LabWindow::GetUiScale().
		void SetUiScale( float scale );

		/// Between ImGui::NewFrame() and ImGui::Render().
		void Draw();

	private:
		enum class Tab
		{
			None,
			Page,
			Selection,
			Engine,
		};

		void DrawToolbar();
		void DrawStatusBar();
		void DrawWwisePopup();
		void DrawHelpPopup();
		void DrawMain();
		void DrawSidebar();
		/// The open page's help, controls and checks.
		void DrawPageTab();
		void DrawChecks();
		bool ToggleButton( const char* label, bool on, const char* tooltip );
		bool LightButton( const char* label, ImU32 light, const char* tooltip );
		void PlayAll();

		LabApp& m_app;
		LabScene& m_scene;
		const SoundBankFolder& m_folder;
		float m_uiScale = 1.0f;

		Selection m_selection;
		SceneCanvas m_canvas;
		SelectionPanel m_selectionPanel;
		EnginePanel m_enginePanel;
		LogPanel m_log;
		Splash m_splash;

		int m_pageGeneration = -1;
		/// The sidebar tab to bring to the front on the next frame.
		Tab m_showTab = Tab::Page;
		/// 0 until the first frame sizes it.
		float m_sidebarWidth = 0.0f;
		float m_logHeight = 0.0f;
		std::string m_outputCaptureFile = "AudioLabCapture.wav";
		std::string m_outputCaptureMarker = "marker";
	};
}
