// Copyright © 2026 CCP ehf.

#include "LabSplash.h"

#include <CarbonAudioHost.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace
{
	constexpr float DURATION = 4.0f;
	/// The end of DURATION, fading out.
	constexpr float FADE = 0.5f;
	/// The most a frame moves it on, in seconds.
	constexpr float MAX_STEP = 1.0f / 30.0f;
	/// Noise and the meter change this many times a second, like a terminal redrawing.
	constexpr float TICKS_PER_SECOND = 24.0f;
	constexpr float TYPED_PER_SECOND = 30.0f;
	/// The boot log's parts report in from BOOT_START, BOOT_STEP apart, each BOOT_SPIN after it shows.
	constexpr float BOOT_START = 0.5f;
	constexpr float BOOT_STEP = 0.25f;
	constexpr float BOOT_SPIN = 0.3f;
	/// The logo settles column by column from LOGO_START over LOGO_SWEEP seconds, with noise NOISE_LEAD ahead of it.
	constexpr float LOGO_START = 0.8f;
	constexpr float LOGO_SWEEP = 1.2f;
	constexpr float NOISE_LEAD = 0.5f;
	constexpr float METER_START = 1.6f;
	constexpr float METER_SWEEP = 0.6f;
	constexpr float READY_AT = 2.6f;
	constexpr int LOGO_ROWS = 5;
	constexpr int METER_ROWS = 4;
	const char NOISE[] = "#$%&*+=?@01<>/\\|[]{}";

	const ImU32 BACKGROUND = IM_COL32( 8, 10, 12, 255 );
	const ImU32 GREEN = IM_COL32( 80, 250, 120, 255 );
	const ImU32 NOISE_GREEN = IM_COL32( 40, 140, 70, 255 );
	const ImU32 AMBER = IM_COL32( 240, 200, 80, 255 );
	const ImU32 TEXT = IM_COL32( 150, 160, 170, 255 );
	const ImU32 EMPTY = IM_COL32( 50, 56, 62, 255 );
	const ImU32 HINT = IM_COL32( 95, 104, 112, 255 );
	const ImU32 SCANLINE = IM_COL32( 0, 0, 0, 60 );

	// "AUDIO LAB" in figlet's standard letters.
	const char* const LETTER_A[LOGO_ROWS] = { R"(    _    )", R"(   / \   )", R"(  / _ \  )", R"( / ___ \ )", R"(/_/   \_\)" };
	const char* const LETTER_U[LOGO_ROWS] = { R"( _   _ )", R"(| | | |)", R"(| | | |)", R"(| |_| |)", R"( \___/ )" };
	const char* const LETTER_D[LOGO_ROWS] = { R"( ____  )", R"(|  _ \ )", R"(| | | |)", R"(| |_| |)", R"(|____/ )" };
	const char* const LETTER_I[LOGO_ROWS] = { R"( ___ )", R"(|_ _|)", R"( | | )", R"( | | )", R"(|___|)" };
	const char* const LETTER_O[LOGO_ROWS] = { R"(  ___  )", R"( / _ \ )", R"(| | | |)", R"(| |_| |)", R"( \___/ )" };
	const char* const LETTER_L[LOGO_ROWS] = { R"( _     )", R"(| |    )", R"(| |    )", R"(| |___ )", R"(|_____|)" };
	const char* const LETTER_B[LOGO_ROWS] = { R"( ____  )", R"(| __ ) )", R"(|  _ \ )", R"(| |_) |)", R"(|____/ )" };
	const char* const WORD_SPACE[LOGO_ROWS] = { "  ", "  ", "  ", "  ", "  " };

	std::array<std::string, LOGO_ROWS> MakeLogo()
	{
		const char* const* const letters[] = { LETTER_A, LETTER_U, LETTER_D, LETTER_I, LETTER_O, WORD_SPACE, LETTER_L, LETTER_A, LETTER_B };
		std::array<std::string, LOGO_ROWS> rows;
		for( const char* const* letter : letters )
		{
			for( int row = 0; row < LOGO_ROWS; ++row )
			{
				rows[row] += rows[row].empty() ? "" : " ";
				rows[row] += letter[row];
			}
		}
		return rows;
	}

	uint32_t Hash( uint32_t a, uint32_t b, uint32_t c )
	{
		uint32_t h = a * 0x9E3779B1u ^ ( b + 0x7F4A7C15u ) * 0x85EBCA77u ^ ( c + 0x165667B1u ) * 0xC2B2AE3Du;
		h ^= h >> 15;
		h *= 0x2C1B3C6Du;
		h ^= h >> 12;
		return h;
	}

	/// In [0, 1).
	float Random( uint32_t a, uint32_t b, uint32_t c )
	{
		return static_cast<float>( Hash( a, b, c ) & 0xFFFFFFu ) / 16777216.0f;
	}

	ImU32 Faded( ImU32 colour, float alpha )
	{
		const ImU32 a = static_cast<ImU32>( static_cast<float>( ( colour >> IM_COL32_A_SHIFT ) & 0xFF ) * alpha );
		return ( colour & ~IM_COL32_A_MASK ) | ( a << IM_COL32_A_SHIFT );
	}
}

namespace AudioLab
{
	void Splash::Draw( float uiScale )
	{
		if( m_over )
		{
			return;
		}
		m_time += std::min( ImGui::GetIO().DeltaTime, MAX_STEP );
		if( m_time >= DURATION )
		{
			m_over = true;
			return;
		}
		const float t = m_time;
		const uint32_t tick = static_cast<uint32_t>( t * TICKS_PER_SECOND );
		const float alpha = std::clamp( ( DURATION - t ) / FADE, 0.0f, 1.0f );

		// A window over everything, so the mouse does not reach the lab under it.
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos( viewport->Pos );
		ImGui::SetNextWindowSize( viewport->Size );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
		ImGui::Begin( "##Splash", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground );
		ImGui::PopStyleVar();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->PushClipRectFullScreen();
		draw->AddRectFilled( viewport->Pos, ImVec2( viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y ), Faded( BACKGROUND, alpha ) );

		// Everything is on a grid of the font's characters; the logo's are twice as big.
		ImFont* font = ImGui::GetFont();
		const float size = ImGui::GetFontSize();
		const float logoSize = size * 2.0f;
		const float charWidth = font->CalcTextSizeA( size, FLT_MAX, 0.0f, "#" ).x;
		const float line = std::round( size * 1.3f );
		static const std::array<std::string, LOGO_ROWS> logo = MakeLogo();
		const int logoColumns = static_cast<int>( logo[0].size() );
		const int meterColumns = logoColumns * 2;
		const float gap = line;
		const float height = 4.0f * line + gap + LOGO_ROWS * logoSize + gap + METER_ROWS * line + gap + line;
		const ImVec2 origin( std::floor( viewport->Pos.x + ( viewport->Size.x - logoColumns * charWidth * 2.0f ) * 0.5f ), std::floor( viewport->Pos.y + ( viewport->Size.y - height ) * 0.5f ) );
		const auto text = [&]( float column, float y, ImU32 colour, const char* begin, const char* end ) {
			draw->AddText( font, size, ImVec2( origin.x + column * charWidth, y ), Faded( colour, alpha ), begin, end );
		};
		const auto cursor = [&]( float column, float y ) {
			if( static_cast<int>( t * 4.0f ) % 2 == 0 )
			{
				const ImVec2 at( origin.x + column * charWidth, y );
				draw->AddRectFilled( at, ImVec2( at.x + charWidth, at.y + size ), Faded( GREEN, alpha ) );
			}
		};

		// The boot log: the command typing out, then each part reporting in.
		float y = origin.y;
		const char* const command = "$ ./audiolab";
		const size_t typed = std::min( std::strlen( command ), static_cast<size_t>( t * TYPED_PER_SECOND ) );
		text( 0.0f, y, TEXT, command, command + typed );
		if( t < BOOT_START )
		{
			cursor( static_cast<float>( typed ), y );
		}
		const std::string wwise = "wwise sdk " + CarbonAudioHost::GetWwiseVersion();
		const char* const parts[] = { "blue", "carbon-audio", wwise.c_str() };
		for( int i = 0; i < 3; ++i )
		{
			y += line;
			const float at = BOOT_START + BOOT_STEP * static_cast<float>( i );
			if( t < at )
			{
				continue;
			}
			// A spinner until it reports in.
			const char spinner[] = { "|/-\\"[tick % 4], '\0' };
			const bool ok = t >= at + BOOT_SPIN;
			text( 0.0f, y, TEXT, "[    ]", nullptr );
			text( 2.0f, y, ok ? GREEN : TEXT, ok ? "ok" : spinner, nullptr );
			text( 7.0f, y, TEXT, parts[i], nullptr );
		}

		// The logo, settling from noise column by column.
		y += line + gap;
		for( int row = 0; row < LOGO_ROWS; ++row )
		{
			for( int column = 0; column < logoColumns; ++column )
			{
				const char letter = logo[row][column];
				const float settle = LOGO_START + LOGO_SWEEP * static_cast<float>( column ) / logoColumns + 0.12f * Random( column, row, 0 );
				if( letter == ' ' || t < settle - NOISE_LEAD )
				{
					continue;
				}
				const bool settled = t >= settle;
				const char shown = settled ? letter : NOISE[Hash( column, row, tick ) % ( sizeof( NOISE ) - 1 )];
				draw->AddText( font, logoSize, ImVec2( origin.x + column * charWidth * 2.0f, y + row * logoSize ), Faded( settled ? GREEN : NOISE_GREEN, alpha ), &shown, &shown + 1 );
			}
		}

		// A level meter, louder at the low end and in bursts like speech; the top row is amber.
		y += LOGO_ROWS * logoSize + gap;
		const float burst = 0.6f + 0.4f * std::fabs( std::sin( static_cast<float>( tick ) * 0.45f ) );
		for( int column = 0; column < meterColumns; ++column )
		{
			const float across = static_cast<float>( column ) / meterColumns;
			if( t < METER_START + METER_SWEEP * across )
			{
				break;
			}
			const float wave = 0.5f + 0.5f * std::sin( column * 0.31f + tick * 0.9f ) * std::sin( column * 0.07f - tick * 0.37f );
			const float level = std::clamp( ( wave * ( 1.0f - 0.55f * across ) + 0.3f * ( Random( column, tick, 1 ) - 0.5f ) ) * burst, 0.05f, 1.0f ) * METER_ROWS;
			for( int row = 0; row < METER_ROWS; ++row )
			{
				const float filled = level - static_cast<float>( row );
				const char shown = filled >= 1.0f ? '#' : filled > 0.4f ? ':' : '.';
				const ImU32 colour = shown == '.' ? EMPTY : row == METER_ROWS - 1 ? AMBER : GREEN;
				text( static_cast<float>( column ), y + ( METER_ROWS - 1 - row ) * line, colour, &shown, &shown + 1 );
			}
		}

		y += METER_ROWS * line + gap;
		if( t >= READY_AT )
		{
			text( 0.0f, y, GREEN, "> ready", nullptr );
			cursor( 7.0f, y );
		}

		// How to skip it, at the bottom.
		const char* const skip = "press space to skip";
		const float skipWidth = font->CalcTextSizeA( size, FLT_MAX, 0.0f, skip ).x;
		draw->AddText( font, size, ImVec2( std::floor( viewport->Pos.x + ( viewport->Size.x - skipWidth ) * 0.5f ), viewport->Pos.y + viewport->Size.y - 2.0f * line ), Faded( HINT, alpha ), skip );

		// Scanlines, like an old screen.
		for( float scan = viewport->Pos.y; scan < viewport->Pos.y + viewport->Size.y; scan += 3.0f * uiScale )
		{
			draw->AddLine( ImVec2( viewport->Pos.x, scan ), ImVec2( viewport->Pos.x + viewport->Size.x, scan ), Faded( SCANLINE, alpha ), uiScale );
		}
		draw->PopClipRect();

		// Space ends it, held down or pressed, and so does any other key or mouse button.
		m_over = ImGui::IsKeyDown( ImGuiKey_Space );
		for( int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key )
		{
			m_over = m_over || ImGui::IsKeyPressed( static_cast<ImGuiKey>( key ), false );
		}
		ImGui::End();
	}
}
