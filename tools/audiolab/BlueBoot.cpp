// Copyright © 2026 CCP ehf.

#include <BlueExposure.h>
#include <Blue.h>
#include <IBluePaths.h>
#include <CcpLog.h>
#include <ITr2AudEmitter.h>
#include <ITr2BoxVolume.h>

#include "BlueBoot.h"

#include "Platform.h"

#include <atomic>
#include <cstdio>
#include <deque>
#include <mutex>

// Every module that includes Blue names itself, for CCP logging and memory tracking.
const char* g_moduleName = "audiolab";

// Each module defines the IIDs of the interfaces it uses; _audio2 has its own.
BLUE_DEFINE_INTERFACE( ITr2AudEmitter );
BLUE_DEFINE_INTERFACE( ITr2BoxVolume );

namespace
{
	constexpr size_t MAX_LOG_LINES = 2000;

	std::mutex s_logMutex;
	std::deque<AudioLab::LogLine> s_log;
	std::atomic<uint64_t> s_logVersion{ 1 };
	bool s_logToConsole = false;

	void Append( AudioLab::LogLine line )
	{
		std::lock_guard<std::mutex> lock( s_logMutex );
		if( s_logToConsole )
		{
			std::printf( "[%s] %s\n", line.channel.c_str(), line.message.c_str() );
		}
		s_log.push_back( std::move( line ) );
		if( s_log.size() > MAX_LOG_LINES )
		{
			s_log.pop_front();
		}
		++s_logVersion;
	}

	// Registered as thread safe: SoundBank loads and Wwise monitor messages arrive on Wwise's threads.
	void OnLog( CcpLogChannel_t& channel, CCP::LogType type, unsigned long, const char* message )
	{
		Append( { static_cast<int>( type ), channel.object ? channel.object : "", message ? message : "" } );
	}
}

namespace AudioLab
{
	bool BootBlue( const std::filesystem::path& soundBankFolder, std::string& error )
	{
		CCP::RegisterLogEcho( &OnLog, CCP::LOGTYPE_INFO, true );

		// Registers Blue's own classes, which Blue paths are built from. No BlueSetCrashReporter: without a reporter
		// it would install a log hook that dereferences null. Never BeOS->Startup or PumpOS: both need Python.
		BlueModuleStartup();

		std::wstring exeFolder = ToWidePath( Platform::GetExecutablePath().parent_path() );
		exeFolder += static_cast<wchar_t>( std::filesystem::path::preferred_separator );
		if( !BlueInitializePaths( exeFolder ) )
		{
			error = "BlueInitializePaths failed.";
			return false;
		}

		if( !BlueSetSearchPaths( { L"soundbanks=" + ToWidePath( soundBankFolder ) } ) )
		{
			error = "BlueSetSearchPaths failed for the SoundBank folder.";
			return false;
		}
		return true;
	}

	void TerminateBlue( int exitCode )
	{
		std::fflush( stdout );
		BlueTerminate( exitCode );
		Platform::ExitProcessNow( exitCode );
	}

	std::vector<LogLine> GetLog( size_t maxLines )
	{
		std::lock_guard<std::mutex> lock( s_logMutex );
		const size_t first = s_log.size() > maxLines ? s_log.size() - maxLines : 0;
		return std::vector<LogLine>( s_log.begin() + first, s_log.end() );
	}

	uint64_t GetLogVersion()
	{
		return s_logVersion.load();
	}

	void AddLogLine( int severity, const std::string& channel, const std::string& message )
	{
		Append( { severity, channel, message } );
	}

	void ClearLog()
	{
		std::lock_guard<std::mutex> lock( s_logMutex );
		s_log.clear();
		++s_logVersion;
	}

	void SetLogToConsole( bool enabled )
	{
		std::lock_guard<std::mutex> lock( s_logMutex );
		s_logToConsole = enabled;
	}
}
