// Copyright © 2026 CCP ehf.

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace AudioLab
{
	std::wstring ToWide( const std::string& utf8 );
	std::string ToUtf8( const std::wstring& wide );
	/// Blue and CarbonAudio take paths as wide strings on every platform.
	std::wstring ToWidePath( const std::filesystem::path& path );
	std::filesystem::path FromUtf8Path( const std::string& utf8 );
	std::string ToUtf8Path( const std::filesystem::path& path );

	/// What the lab needs from the operating system, for Windows and macOS.
	namespace Platform
	{
		/// The command line arguments as UTF-8, without the executable.
		std::vector<std::string> GetArguments( int argc, char** argv );
		std::filesystem::path GetExecutablePath();
		/// A per-user folder for the lab's settings, created on first use.
		std::filesystem::path GetUserConfigFolder();
		/// A message box, for errors before or without a window.
		void ShowError( const std::string& title, const std::string& message );
		/// Ends the process without running destructors, as Blue expects.
		[[noreturn]] void ExitProcessNow( int exitCode );
		int GetProcessId();
	}
}
