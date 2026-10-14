// Copyright © 2026 CCP ehf.

#include "LabLogPanel.h"

#include "LabWidgets.h"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace
{
	constexpr size_t LOG_LINES = 2000;
}

namespace AudioLab
{
	void LogPanel::Refresh()
	{
		const uint64_t version = GetLogVersion();
		if( version == m_version )
		{
			return;
		}
		m_version = version;
		m_lines = GetLog( LOG_LINES );
		m_warnings = 0;
		m_errors = 0;
		for( const LogLine& line : m_lines )
		{
			m_warnings += line.severity == LOG_WARNING ? 1 : 0;
			m_errors += line.severity >= LOG_ERROR ? 1 : 0;
		}
	}

	void LogPanel::Draw()
	{
		ImGui::SetNextItemWidth( ImGui::GetFontSize() * 14.0f );
		ImGui::InputTextWithHint( "##Filter", "Filter", &m_filter );
		ImGui::SameLine();
		const char* levels[] = { "Everything", "Notices and up", "Warnings and up", "Errors" };
		ImGui::SetNextItemWidth( ImGui::GetFontSize() * 10.0f );
		ImGui::Combo( "##Level", &m_minSeverity, levels, 4 );
		ImGui::SameLine();
		ImGui::Checkbox( "Follow", &m_autoScroll );
		ImGui::SameLine();
		if( ImGui::Button( "Clear" ) )
		{
			ClearLog();
		}
		AlignRight( ButtonWidth( "Hide" ) );
		if( ImGui::Button( "Hide" ) )
		{
			m_open = false;
		}

		ImGui::BeginChild( "lines", ImVec2( 0.0f, 0.0f ), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar );
		// A line repeated back to back shows once, with how many times it came.
		for( size_t i = 0; i < m_lines.size(); )
		{
			const LogLine& line = m_lines[i];
			size_t repeats = 1;
			while( i + repeats < m_lines.size() && m_lines[i + repeats].severity == line.severity && m_lines[i + repeats].message == line.message && m_lines[i + repeats].channel == line.channel )
			{
				++repeats;
			}
			i += repeats;

			if( line.severity < m_minSeverity )
			{
				continue;
			}
			if( !m_filter.empty() && line.message.find( m_filter ) == std::string::npos && line.channel.find( m_filter ) == std::string::npos )
			{
				continue;
			}
			const ImVec4 colour = line.severity >= LOG_ERROR ? FAIL_COLOUR : line.severity == LOG_WARNING ? WARNING_COLOUR : ImVec4( 0.75f, 0.75f, 0.78f, 1.0f );
			if( repeats > 1 )
			{
				ImGui::TextColored( colour, "[%s] %s  %s%zu", line.channel.c_str(), line.message.c_str(), TIMES, repeats );
			}
			else
			{
				ImGui::TextColored( colour, "[%s] %s", line.channel.c_str(), line.message.c_str() );
			}
		}
		if( m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 4.0f )
		{
			ImGui::SetScrollHereY( 1.0f );
		}
		ImGui::EndChild();
	}
}
