// Copyright © 2026 CCP ehf.

#include "LabUi.h"

#include "LabWidgets.h"
#include "Platform.h"

#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
	const ImU32 LIGHT_ON = IM_COL32( 80, 200, 120, 255 );
	const ImU32 LIGHT_BUSY = IM_COL32( 230, 180, 60, 255 );
	const ImU32 LIGHT_OFF = IM_COL32( 110, 110, 120, 255 );
	const ImU32 LIGHT_RECORDING = IM_COL32( 230, 70, 70, 255 );
	const ImVec4 TOGGLE_ON( 0.16f, 0.36f, 0.24f, 1.0f );
	const ImVec4 TOGGLE_ON_HOVERED( 0.20f, 0.46f, 0.30f, 1.0f );

	void VerticalSeparator()
	{
		ImGui::SameLine();
		ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );
		ImGui::SameLine();
	}

	float LightButtonWidth( const char* label )
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		return style.FramePadding.x * 2.0f + ImGui::GetFontSize() * 0.6f + style.ItemInnerSpacing.x + ImGui::CalcTextSize( label ).x;
	}

	/// A button with a light before its label, hollow when it is off.
	bool ButtonWithLight( const char* label, ImU32 light, bool hollow, ImU32 textColour )
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float radius = ImGui::GetFontSize() * 0.3f;
		const ImVec2 size( LightButtonWidth( label ), ImGui::GetFrameHeight() );
		ImGui::PushID( label );
		const bool pressed = ImGui::Button( "##light", size );
		ImGui::PopID();

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 centre( min.x + style.FramePadding.x + radius, min.y + size.y * 0.5f );
		if( hollow )
		{
			draw->AddCircle( centre, radius - 0.5f, light, 0, std::max( 1.0f, radius * 0.35f ) );
		}
		else
		{
			draw->AddCircleFilled( centre, radius, light );
		}
		draw->AddText( ImVec2( centre.x + radius + style.ItemInnerSpacing.x, min.y + style.FramePadding.y ), textColour, label );
		return pressed;
	}

	/// ImGui's splitter at the cursor, between the panes before and after it along the axis: length long, thickness
	/// thick and easier to grab than it looks. A double-click sets after to 0 for the caller to size again.
	void Splitter( const char* id, ImGuiAxis axis, float length, float thickness, float grab, float& before, float& after, float minBefore, float minAfter )
	{
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const ImVec2 max = axis == ImGuiAxis_X ? ImVec2( min.x + thickness, min.y + length ) : ImVec2( min.x + length, min.y + thickness );
		ImGui::SplitterBehavior( ImRect( min, max ), ImGui::GetID( id ), axis, &before, &after, minBefore, minAfter, grab );
		if( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
		{
			after = 0.0f;
		}
	}
}

namespace AudioLab
{
	LabUi::LabUi( LabApp& app ) :
		m_app( app ),
		m_scene( app.GetScene() ),
		m_folder( app.GetFolder() ),
		m_selection( app.GetScene() ),
		m_canvas( app, m_selection ),
		m_selectionPanel( app, m_selection, m_canvas ),
		m_enginePanel( app )
	{
	}

	void LabUi::SetUiScale( float scale )
	{
		m_uiScale = scale;
		m_canvas.SetUiScale( scale );
	}

	void LabUi::Draw()
	{
		if( m_pageGeneration != m_app.GetPageGeneration() )
		{
			// The scene was rebuilt, so nothing the UI points at is there any more.
			m_pageGeneration = m_app.GetPageGeneration();
			m_selection.Clear();
			m_canvas.Reset();
			m_showTab = Tab::Page;
		}
		// Pages and checks remove emitters too.
		m_selection.Prune();
		m_log.Refresh();

		m_canvas.HandleListenerKeys();
		DrawToolbar();
		DrawStatusBar();
		DrawMain();
		m_splash.Draw( m_uiScale );
		if( m_splash.IsOver() )
		{
			m_app.ReleaseDemo();
		}
	}

	bool LabUi::ToggleButton( const char* label, bool on, const char* tooltip )
	{
		ImGui::PushStyleColor( ImGuiCol_Button, on ? TOGGLE_ON : ImGui::GetStyleColorVec4( ImGuiCol_FrameBg ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, on ? TOGGLE_ON_HOVERED : ImGui::GetStyleColorVec4( ImGuiCol_FrameBgHovered ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4( ImGuiCol_FrameBgActive ) );
		const bool pressed = ButtonWithLight( label, on ? LIGHT_ON : LIGHT_OFF, !on, ImGui::GetColorU32( on ? ImGuiCol_Text : ImGuiCol_TextDisabled ) );
		ImGui::PopStyleColor( 3 );
		ImGui::SetItemTooltip( "%s\n%s; click to turn it %s.", tooltip, on ? "On" : "Off", on ? "off" : "on" );
		return pressed;
	}

	bool LabUi::LightButton( const char* label, ImU32 light, const char* tooltip )
	{
		const bool pressed = ButtonWithLight( label, light, light == LIGHT_OFF, ImGui::GetColorU32( ImGuiCol_Text ) );
		ImGui::SetItemTooltip( "%s", tooltip );
		return pressed;
	}

	void LabUi::DrawToolbar()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const float height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;
		if( ImGui::BeginViewportSideBar( "##Toolbar", ImGui::GetMainViewport(), ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings ) )
		{
			// Running checks own the page, the scene and the switches.
			ImGui::BeginDisabled( m_app.GetChecks().IsRunning() );
			const auto& pages = m_app.GetPages();
			float nameWidth = 0.0f;
			for( const auto& page : pages )
			{
				nameWidth = std::max( nameWidth, ImGui::CalcTextSize( page->GetName() ).x );
			}
			ImGui::SetNextItemWidth( nameWidth + ImGui::GetFrameHeight() + style.FramePadding.x * 2.0f );
			if( ImGui::BeginCombo( "##Page", m_app.GetPage().GetName() ) )
			{
				for( int page = 0; page < static_cast<int>( pages.size() ); ++page )
				{
					if( ImGui::Selectable( pages[page]->GetName(), page == m_app.GetPageIndex() ) )
					{
						m_app.OpenPage( page );
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SetItemTooltip( "The page: one feature with its own scene, controls and checks. Opening one rebuilds the scene." );
			ImGui::SameLine();
			if( ImGui::Button( "Reset page" ) )
			{
				m_app.OpenPage( m_app.GetPageIndex() );
			}
			ImGui::SetItemTooltip( "Clears the scene and builds the page's scene again." );

			VerticalSeparator();
			if( ImGui::Button( "Play all" ) )
			{
				PlayAll();
			}
			ImGui::SetItemTooltip( "Send every emitter's event again" );
			ImGui::SameLine();
			if( ImGui::Button( "Stop all" ) )
			{
				CarbonAudioHost::StopAll();
			}
			ImGui::SetItemTooltip( "Stop every sound on every game object" );

			VerticalSeparator();
			const bool rooms = CarbonAudioHost::GetRoomsEnabled();
			if( ToggleButton( "Rooms", rooms, "Spatial Audio rooms (spatialAudioRoomsEnabled)." ) )
			{
				CarbonAudioHost::SetRoomsEnabled( !rooms );
			}
			ImGui::SameLine();
			const bool geometry = CarbonAudioHost::GetGeometryEnabled();
			if( ToggleButton( "Geometry", geometry, "Spatial Audio geometry (spatialAudioGeometryEnabled).\nWhile it is on, game-driven occlusion is suppressed." ) )
			{
				CarbonAudioHost::SetGeometryEnabled( !geometry );
			}
			ImGui::SameLine();
			const bool occlusion = CarbonAudioHost::GetObstructionOcclusionEnabled();
			if( ToggleButton( "Occlusion", occlusion, geometry ? "Game-driven occlusion: walls muffle the emitters behind them.\nSuppressed: Geometry is on." : "Game-driven occlusion: walls (the Wall tool, 4) muffle the emitters behind them." ) )
			{
				CarbonAudioHost::SetObstructionOcclusionEnabled( !occlusion );
			}
			ImGui::EndDisabled();

			// Right: Wwise, a capture that is recording, and help.
			const WwiseStatus wwise = m_app.GetWwise().GetStatus();
			const bool connected = wwise.connection == WwiseStatus::Connection::Connected;
			const char* wwiseTooltip = connected ? ( wwise.isLabProject ? "Connected to Wwise Authoring with the lab's project." : "Connected to Wwise Authoring, but the open project is not the lab's." )
									 : wwise.connection == WwiseStatus::Connection::Connecting ? "Looking for Wwise Authoring (WAAPI on port 8080)."
																							   : "Not connected to Wwise Authoring.";
			const ImU32 wwiseLight = connected ? ( wwise.isLabProject ? LIGHT_ON : LIGHT_BUSY ) : wwise.connection == WwiseStatus::Connection::Connecting ? LIGHT_BUSY : LIGHT_OFF;
			const std::string wwiseLabel = wwise.profilerAttached ? std::string( "Wwise" ) + SEPARATOR + "Profiler" : std::string( "Wwise" );
			const bool recording = CarbonAudioHost::IsProfilerCapturing() || CarbonAudioHost::IsOutputCapturing() || wwise.capturing;

			float rightWidth = LightButtonWidth( wwiseLabel.c_str() ) + style.ItemSpacing.x + ImGui::GetFrameHeight();
			if( recording )
			{
				rightWidth += LightButtonWidth( "Recording" ) + style.ItemSpacing.x;
			}
			AlignRight( rightWidth );
			if( recording )
			{
				if( LightButton( "Recording", LIGHT_RECORDING, "A capture is recording; click to see it." ) )
				{
					ImGui::OpenPopup( "Wwise" );
				}
				ImGui::SameLine();
			}
			if( LightButton( wwiseLabel.c_str(), wwiseLight, wwiseTooltip ) )
			{
				ImGui::OpenPopup( "Wwise" );
			}
			ImGui::SameLine();
			if( ImGui::Button( "?", ImVec2( ImGui::GetFrameHeight(), 0.0f ) ) )
			{
				ImGui::OpenPopup( "Help" );
			}
			ImGui::SetItemTooltip( "Shortcuts and versions" );

			DrawWwisePopup();
			DrawHelpPopup();
		}
		ImGui::End();
	}

	void LabUi::DrawWwisePopup()
	{
		if( !ImGui::BeginPopup( "Wwise" ) )
		{
			return;
		}
		WwiseLink& wwise = m_app.GetWwise();
		const WwiseStatus status = wwise.GetStatus();
		const bool connected = status.connection == WwiseStatus::Connection::Connected;
		const ImVec2 startStop( ImGui::GetFontSize() * 3.5f, 0.0f );
		ImGui::PushTextWrapPos( ImGui::GetFontSize() * 32.0f );

		ImGui::SeparatorText( "Wwise Authoring" );
		bool enabled = wwise.IsEnabled();
		if( ImGui::Checkbox( "Connect", &enabled ) )
		{
			wwise.SetEnabled( enabled );
		}
		ImGui::SameLine();
		bool autoAttach = wwise.GetAutoAttachProfiler();
		ImGui::BeginDisabled( !CarbonAudioHost::GetWwiseCommunicationEnabled() );
		if( ImGui::Checkbox( "Attach the Profiler automatically", &autoAttach ) )
		{
			wwise.SetAutoAttachProfiler( autoAttach );
		}
		ImGui::EndDisabled();
		if( connected )
		{
			ImGui::Text( "Wwise %s%s%s", status.version.c_str(), SEPARATOR, status.projectName.empty() ? "no project open" : status.projectName.c_str() );
			if( !status.projectName.empty() && !status.isLabProject )
			{
				ImGui::TextColored( WARNING_COLOUR, "Not the lab's project. Open %s", AUDIOLAB_WWISE_PROJECT );
			}
			ImGui::Text( "Profiler: %s", status.profilerAttached ? "connected to this process" : status.profilerTarget.empty() ? "not connected" : ( "connected to " + status.profilerTarget ).c_str() );
			if( ImGui::Button( status.profilerAttached ? "Detach the Profiler" : "Attach the Profiler" ) )
			{
				status.profilerAttached ? wwise.DetachProfiler() : wwise.AttachProfiler();
			}
			ImGui::SameLine();
			if( ImGui::Button( "Generate SoundBanks" ) )
			{
				wwise.GenerateSoundBanks();
			}
			ImGui::SetItemTooltip( "Same as F7 in Wwise. The lab reloads its SoundBanks once Wwise has written them." );
		}
		else if( status.connection == WwiseStatus::Connection::Connecting )
		{
			ImGui::TextDisabled( "Looking for Wwise Authoring on 127.0.0.1:8080. Start Wwise, open %s and enable WAAPI (Project > User Preferences > Enable Wwise Authoring API).", AUDIOLAB_WWISE_PROJECT );
		}
		else
		{
			ImGui::TextDisabled( "Not connected." );
		}
		if( !status.lastError.empty() )
		{
			ImGui::TextColored( FAIL_COLOUR, "Last error: %s", status.lastError.c_str() );
		}

		ImGui::SeparatorText( "Recording" );
		ImGui::BeginDisabled( !connected );
		if( ImGui::Button( status.capturing ? "Stop##Session" : "Start##Session", startStop ) )
		{
			status.capturing ? wwise.StopCapture() : wwise.StartCapture();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted( "Profiler session in Wwise Authoring" );

		const bool profiling = CarbonAudioHost::IsProfilerCapturing();
		ImGui::BeginDisabled( !CarbonAudioHost::GetWwiseCommunicationEnabled() );
		if( ImGui::Button( profiling ? "Stop##Prof" : "Start##Prof", startStop ) )
		{
			profiling ? CarbonAudioHost::StopProfilerCapture() : CarbonAudioHost::StartProfilerCapture();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted( "Profiler capture to a .prof file" );
		HelpMarker( "The sound engine writes WwiseCapture_<date>_<time>.prof to the working directory; open it in Wwise." );

		const bool recording = CarbonAudioHost::IsOutputCapturing();
		if( ImGui::Button( recording ? "Stop##Wav" : "Start##Wav", startStop ) )
		{
			recording ? CarbonAudioHost::StopOutputCapture() : CarbonAudioHost::StartOutputCapture( ToWide( m_outputCaptureFile ) );
		}
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted( "Output to" );
		ImGui::SameLine();
		ImGui::SetNextItemWidth( ImGui::GetFontSize() * 12.0f );
		ImGui::BeginDisabled( recording );
		ImGui::InputText( "##WavFile", &m_outputCaptureFile );
		ImGui::EndDisabled();
		if( recording )
		{
			ImGui::SetNextItemWidth( ImGui::GetFontSize() * 12.0f );
			ImGui::InputText( "##Marker", &m_outputCaptureMarker );
			ImGui::SameLine();
			if( ImGui::Button( "Add marker" ) )
			{
				CarbonAudioHost::AddOutputCaptureMarker( m_outputCaptureMarker );
			}
		}

		ImGui::SeparatorText( "This process" );
		CarbonAudioHost::CommSettings comm;
		if( CarbonAudioHost::GetCommSettings( comm ) )
		{
			ImGui::Text( "The Profiler sees it as %s, on ports %u and %u.", comm.appNetworkName.c_str(), comm.discoveryBroadcastPort, comm.commandPort );
			if( ImGui::Button( "Restart Wwise communication" ) )
			{
				CarbonAudioHost::ResetComm();
			}
			HelpMarker( "Drops a connected Wwise Profiler." );
		}
		else
		{
			ImGui::TextDisabled( "No Wwise communication (Release build)." );
		}
		ImGui::PopTextWrapPos();
		ImGui::EndPopup();
	}

	void LabUi::DrawHelpPopup()
	{
		if( !ImGui::BeginPopup( "Help" ) )
		{
			return;
		}
		ImGui::SeparatorText( "Scene" );
		if( ImGui::BeginTable( "keys", 2, ImGuiTableFlags_SizingFixedFit ) )
		{
			const char* const keys[][2] = {
				{ "W A S D", "Walk the listener; Shift walks faster" },
				{ "Q E", "Turn the listener" },
				{ "1 2 3 4 5", "Select, draw a room, place an emitter, draw a wall, place a portal" },
				{ "Drag", "Select everything in a box; drag a selected thing to move them all" },
				{ "Ctrl-click", "Add to the selection or take out of it; Shift works too" },
				{ "Right-drag", "Turn the view; Shift-right-drag pans, as right-drag does in 2D" },
				{ "Wheel", "Zoom" },
				{ "F", "Frame everything" },
				{ "Right-click", "What can be done there" },
				{ "Delete", "Remove the selection" },
				{ "Esc", "Clear the selection" },
			};
			for( const auto& key : keys )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted( key[0] );
				ImGui::TableNextColumn();
				ImGui::TextDisabled( "%s", key[1] );
			}
			ImGui::EndTable();
		}
		ImGui::SeparatorText( "Versions" );
		ImGui::Text( "Wwise SDK %s", CarbonAudioHost::GetWwiseVersion().c_str() );
		const WwiseStatus status = m_app.GetWwise().GetStatus();
		if( !status.version.empty() )
		{
			ImGui::Text( "Wwise Authoring %s", status.version.c_str() );
		}
		ImGui::EndPopup();
	}

	void LabUi::DrawStatusBar()
	{
		if( ImGui::BeginViewportSideBar( "##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar ) )
		{
			if( ImGui::BeginMenuBar() )
			{
				ImGui::TextUnformatted( CarbonAudioHost::IsRunning() ? "CarbonAudio running" : "CarbonAudio stopped" );

				ImGui::Separator();
				ImGui::Text( "SoundBanks %zu/%zu", CarbonAudioHost::GetLoadedSoundBanks().size(), m_app.GetSoundBanks().size() + 1 );
				ImGui::SetItemTooltip( "%s", ToUtf8Path( m_folder.path ).c_str() );
				if( !m_folder.warnings.empty() )
				{
					ImGui::SameLine();
					ImGui::TextColored( WARNING_COLOUR, "%zu %s", m_folder.warnings.size(), m_folder.warnings.size() == 1 ? "warning" : "warnings" );
					ImGui::SetItemTooltip( "%s\nEngine > SoundBanks has them all.", m_folder.warnings.front().c_str() );
				}

				CarbonAudioHost::ResourceMonitorSummary summary;
				if( CarbonAudioHost::GetResourceMonitorSummary( summary ) )
				{
					ImGui::Separator();
					ImGui::Text( "CPU %.1f%%%s%u voices (+%u virtual)%s%u events", summary.totalCPU, SEPARATOR, summary.physicalVoices, summary.virtualVoices, SEPARATOR, summary.activeEvents );
					ImGui::SetItemTooltip( "Plug-ins use %.2f%% of the CPU.", summary.pluginCPU );
				}

				// Right: the log, with how many warnings and errors it has.
				const int warnings = m_log.GetWarningCount();
				const int errors = m_log.GetErrorCount();
				std::string label = "Log";
				if( errors > 0 )
				{
					label += SEPARATOR + std::to_string( errors ) + ( errors == 1 ? " error" : " errors" );
				}
				if( warnings > 0 )
				{
					label += SEPARATOR + std::to_string( warnings ) + ( warnings == 1 ? " warning" : " warnings" );
				}
				// A menu bar's item is as wide as its label and the item spacing either side.
				AlignRight( ImGui::CalcTextSize( label.c_str() ).x + ImGui::GetStyle().ItemSpacing.x * 2.0f );
				ImGui::PushStyleColor( ImGuiCol_Text, errors > 0 ? FAIL_COLOUR : warnings > 0 ? WARNING_COLOUR : ImGui::GetStyleColorVec4( ImGuiCol_Text ) );
				if( ImGui::MenuItem( label.c_str(), nullptr, m_log.IsOpen() ) )
				{
					m_log.SetOpen( !m_log.IsOpen() );
				}
				ImGui::PopStyleColor();
				ImGui::SetItemTooltip( m_log.IsOpen() ? "Hide the log" : "Show the log" );
				ImGui::EndMenuBar();
			}
		}
		ImGui::End();
	}

	void LabUi::DrawMain()
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos( viewport->WorkPos );
		ImGui::SetNextWindowSize( viewport->WorkSize );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse;
		const bool visible = ImGui::Begin( "##Main", nullptr, flags );
		ImGui::PopStyleVar( 3 );
		if( !visible )
		{
			ImGui::End();
			return;
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 area = ImGui::GetContentRegionAvail();
		const float splitter = std::max( 1.0f, std::round( 2.0f * m_uiScale ) );
		const float grab = 3.0f * m_uiScale;
		const float minimum = ImGui::GetFontSize() * 16.0f;

		if( m_sidebarWidth <= 0.0f )
		{
			m_sidebarWidth = area.x * 0.32f;
		}
		const float narrowest = std::min( minimum, area.x * 0.5f );
		m_sidebarWidth = std::clamp( m_sidebarWidth, narrowest, std::max( narrowest, area.x - minimum - splitter ) );
		const float sceneWidth = std::max( 1.0f, area.x - m_sidebarWidth - splitter );

		// Left: the scene's bar, the scene and, when it is open, the log.
		ImGui::BeginChild( "##SceneColumn", ImVec2( sceneWidth, area.y ), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
		{
			const float barPadding = style.ItemSpacing.y;
			const float barHeight = ImGui::GetFrameHeight() + barPadding * 2.0f;
			ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( style.WindowPadding.x, barPadding ) );
			ImGui::BeginChild( "##SceneBar", ImVec2( 0.0f, barHeight ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
			ImGui::PopStyleVar();
			m_canvas.DrawBar();
			ImGui::EndChild();

			float sceneHeight = area.y - barHeight;
			if( m_log.IsOpen() )
			{
				if( m_logHeight <= 0.0f )
				{
					m_logHeight = area.y * 0.3f;
				}
				const float lowest = std::min( ImGui::GetFrameHeight() * 4.0f, sceneHeight * 0.5f );
				m_logHeight = std::clamp( m_logHeight, lowest, std::max( lowest, sceneHeight - splitter - ImGui::GetFrameHeight() * 4.0f ) );
				sceneHeight -= m_logHeight + splitter;
			}

			ImGui::SetCursorPos( ImVec2( 0.0f, barHeight ) );
			ImGui::BeginChild( "##Scene", ImVec2( 0.0f, std::max( 1.0f, sceneHeight ) ), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
			m_canvas.Draw();
			ImGui::EndChild();

			if( m_log.IsOpen() )
			{
				ImGui::SetCursorPos( ImVec2( 0.0f, barHeight + sceneHeight ) );
				float sceneRowHeight = sceneHeight;
				Splitter( "##LogSplitter", ImGuiAxis_Y, sceneWidth, splitter, grab, sceneRowHeight, m_logHeight, ImGui::GetFrameHeight() * 4.0f, ImGui::GetFrameHeight() * 4.0f );
				ImGui::SetCursorPos( ImVec2( 0.0f, barHeight + sceneHeight + splitter ) );
				ImGui::BeginChild( "##Log", ImVec2( 0.0f, m_logHeight ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar );
				m_log.Draw();
				ImGui::EndChild();
			}
		}
		ImGui::EndChild();

		// Right: the sidebar.
		ImGui::SetCursorPos( ImVec2( sceneWidth, 0.0f ) );
		float sceneColumnWidth = sceneWidth;
		Splitter( "##SidebarSplitter", ImGuiAxis_X, area.y, splitter, grab, sceneColumnWidth, m_sidebarWidth, minimum, narrowest );
		ImGui::SetCursorPos( ImVec2( sceneWidth + splitter, 0.0f ) );
		ImGui::BeginChild( "##Sidebar", ImVec2( m_sidebarWidth, area.y ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar );
		DrawSidebar();
		ImGui::EndChild();

		ImGui::End();
	}

	void LabUi::DrawSidebar()
	{
		if( !ImGui::BeginTabBar( "##SidebarTabs" ) )
		{
			return;
		}
		if( m_selection.TakeShowRequest() )
		{
			m_showTab = Tab::Selection;
		}
		const auto tab = [this]( const char* label, Tab tab ) {
			return ImGui::BeginTabItem( label, nullptr, m_showTab == tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None );
		};
		// Running checks own the scene and the engine; their tabs show it without changing it.
		const bool running = m_app.GetChecks().IsRunning();
		if( tab( "Page", Tab::Page ) )
		{
			ImGui::BeginChild( "##PageTab" );
			DrawPageTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if( tab( "Selection", Tab::Selection ) )
		{
			ImGui::BeginChild( "##SelectionTab" );
			ImGui::BeginDisabled( running );
			m_selectionPanel.Draw();
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if( tab( "Engine", Tab::Engine ) )
		{
			ImGui::BeginChild( "##EngineTab" );
			ImGui::BeginDisabled( running );
			m_enginePanel.Draw();
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		m_showTab = Tab::None;
		ImGui::EndTabBar();
	}

	void LabUi::DrawPageTab()
	{
		LabPage& page = m_app.GetPage();
		if( ImGui::TreeNodeEx( "About this page", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui::PushTextWrapPos( 0.0f );
			ImGui::TextDisabled( "%s", page.GetHelp() );
			ImGui::PopTextWrapPos();
			ImGui::TreePop();
		}

		ImGui::PushID( page.GetID() );
		ImGui::BeginDisabled( m_app.GetChecks().IsRunning() );
		page.DrawControls( m_app );
		ImGui::EndDisabled();
		ImGui::PopID();

		DrawChecks();
	}

	void LabUi::DrawChecks()
	{
		LabPage& page = m_app.GetPage();
		const std::vector<LabCheck>& checks = m_app.GetChecks().GetChecks( m_app.GetPageIndex() );
		const std::vector<CheckResult>& results = m_app.GetChecks().GetResults();
		const bool running = m_app.GetChecks().IsRunning();

		ImGui::SeparatorText( "Checks" );
		ImGui::BeginDisabled( running );
		if( !checks.empty() )
		{
			if( ImGui::Button( "Run all" ) )
			{
				m_app.GetChecks().Queue( page.GetID() );
			}
			ImGui::SetItemTooltip( "Runs this page's checks one after another, each on a freshly opened page." );
			ImGui::SameLine();
		}
		if( ImGui::Button( "Run every page's checks" ) )
		{
			m_app.GetChecks().Queue( "all" );
		}
		ImGui::EndDisabled();
		if( running )
		{
			ImGui::SameLine();
			if( ImGui::Button( "Stop" ) )
			{
				m_app.GetChecks().Stop( "stopped" );
			}
			ImGui::SetItemTooltip( "Fails the running check and drops the rest" );
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::TextColored( WARNING_COLOUR, "Running..." );
		}
		if( checks.empty() )
		{
			ImGui::TextDisabled( "This page has no checks of its own." );
		}

		ImGui::BeginDisabled( running );
		for( const LabCheck& check : checks )
		{
			const CheckResult* last = nullptr;
			for( const CheckResult& result : results )
			{
				if( result.page == page.GetID() && result.check == check.name )
				{
					last = &result;
				}
			}
			ImGui::PushID( check.name.c_str() );
			if( ImGui::SmallButton( "Run" ) )
			{
				m_app.GetChecks().Queue( std::string( page.GetID() ) + "/" + check.name );
			}
			ImGui::PopID();
			ImGui::SameLine();
			if( last != nullptr )
			{
				ImGui::TextColored( last->passed ? PASS_COLOUR : FAIL_COLOUR, last->passed ? "PASS" : "FAIL" );
				ImGui::SameLine();
			}
			ImGui::TextUnformatted( check.name.c_str() );
			ImGui::SetItemTooltip( "%s", check.description.c_str() );
			if( last != nullptr && !last->passed )
			{
				ImGui::Indent();
				ImGui::PushTextWrapPos( 0.0f );
				ImGui::TextColored( FAIL_COLOUR, "%s", last->message.c_str() );
				ImGui::PopTextWrapPos();
				ImGui::Unindent();
			}
		}
		ImGui::EndDisabled();

		if( results.empty() )
		{
			return;
		}
		// The totals, with the failures of other pages, from running every page's checks.
		const size_t passed = std::count_if( results.begin(), results.end(), []( const CheckResult& result ) { return result.passed; } );
		ImGui::Spacing();
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled( "%zu passed, %zu failed", passed, results.size() - passed );
		ImGui::SameLine();
		ImGui::BeginDisabled( running );
		if( ImGui::SmallButton( "Clear" ) )
		{
			m_app.GetChecks().ClearResults();
		}
		ImGui::EndDisabled();
		ImGui::PushTextWrapPos( 0.0f );
		for( const CheckResult& result : results )
		{
			if( !result.passed && result.page != page.GetID() )
			{
				ImGui::TextColored( FAIL_COLOUR, "%s/%s: %s", result.page.c_str(), result.check.c_str(), result.message.c_str() );
			}
		}
		ImGui::PopTextWrapPos();
	}

	void LabUi::PlayAll()
	{
		for( LabEmitter& emitter : m_scene.emitters )
		{
			m_scene.Play( emitter );
		}
	}
}
