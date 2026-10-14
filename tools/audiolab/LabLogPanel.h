// Copyright © 2026 CCP ehf.

#pragma once

#include "BlueBoot.h"

#include <cstdint>
#include <string>
#include <vector>

namespace AudioLab
{
	/// The log under the scene: CcpCore's and Wwise Authoring's lines, with a filter and a lowest severity.
	class LogPanel
	{
	public:
		/// Once a frame, before the status bar and the panel read the lines; copies them only when the log changed.
		void Refresh();
		void Draw();

		bool IsOpen() const { return m_open; }
		void SetOpen( bool open ) { m_open = open; }
		int GetWarningCount() const { return m_warnings; }
		int GetErrorCount() const { return m_errors; }

	private:
		std::vector<LogLine> m_lines;
		/// GetLogVersion() when m_lines was copied.
		uint64_t m_version = 0;
		int m_warnings = 0;
		int m_errors = 0;
		bool m_open = false;
		std::string m_filter;
		int m_minSeverity = LOG_WARNING;
		bool m_autoScroll = true;
	};
}
