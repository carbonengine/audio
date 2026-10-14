// Copyright © 2026 CCP ehf.

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace AudioLab
{
	/// Log severities, as CCP::LogType numbers them.
	constexpr int LOG_INFO = 0;
	constexpr int LOG_NOTICE = 1;
	constexpr int LOG_WARNING = 2;
	constexpr int LOG_ERROR = 3;

	struct LogLine
	{
		/// One of the LOG_ severities.
		int severity;
		std::string channel;
		std::string message;
	};

	/// Boots Blue the way exefile does before it starts Python, without Python: Blue's classes, Blue paths
	/// rooted at the exe folder and the soundbanks:/ search path. Mirrors CcpCore's log into GetLog().
	bool BootBlue( const std::filesystem::path& soundBankFolder, std::string& error );
	/// Ends the process through Blue, as exefile does; no destructors run.
	[[noreturn]] void TerminateBlue( int exitCode );

	/// The most recent log lines, oldest first. Lines come from every thread.
	std::vector<LogLine> GetLog( size_t maxLines );
	/// Changes whenever a line is added or the log is cleared, so readers copy it only then.
	uint64_t GetLogVersion();
	/// Adds a line that did not come through CcpCore, e.g. from Wwise Authoring.
	void AddLogLine( int severity, const std::string& channel, const std::string& message );
	void ClearLog();
	/// Also print every log line to stdout, for headless runs.
	void SetLogToConsole( bool enabled );
}
