// Copyright © 2026 CCP ehf.

// The Audio Lab: places rooms, emitters and the listener on a 2D canvas and runs CarbonAudio natively, so changes
// can be tried with the Wwise Profiler without launching the game.
//
//   audiolab[_<flavour>] [--soundbanks <folder>] [--bank <name.bnk>]... [--page <page>] [--check all|<page>|<page>/<check>]
//
// --page opens a page at start; an empty one is the first page. --check runs pages' checks without a window and exits 0
// when they pass. Arguments it does not know exit 2.

#include "BlueBoot.h"
#include "LabApp.h"
#include "LabUi.h"
#include "LabWindow.h"
#include "Platform.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace
{
	const char* const USAGE = "audiolab [--soundbanks <folder>] [--bank <name.bnk>]... [--page <page>] [--check all|<page>|<page>/<check>]";

	struct CommandLine
	{
		AudioLab::LabOptions lab;
		/// What --check runs: "all", "<page>" or "<page>/<check>".
		std::string checks;
		/// The page to open at start; empty for the first page.
		std::string page;
		/// What is wrong with the arguments; empty when nothing is.
		std::string error;
	};

	CommandLine ParseCommandLine( const std::vector<std::string>& arguments )
	{
		CommandLine commandLine;
		commandLine.lab.soundBankFolder = AudioLab::FromUtf8Path( AUDIOLAB_DEFAULT_SOUNDBANKS );
		for( size_t i = 0; i < arguments.size(); ++i )
		{
			const std::string& argument = arguments[i];
			if( argument != "--check" && argument != "--page" && argument != "--soundbanks" && argument != "--bank" )
			{
				commandLine.error = "Unknown argument " + argument + ".";
				break;
			}
			// A run that asked for checks has no one to read a message box, even when its arguments are wrong.
			commandLine.lab.headless = commandLine.lab.headless || argument == "--check";
			if( i + 1 >= arguments.size() )
			{
				commandLine.error = argument + " needs a value.";
				break;
			}
			const std::string& value = arguments[++i];
			if( argument == "--check" )
			{
				commandLine.checks = value;
			}
			else if( argument == "--page" )
			{
				commandLine.page = value;
			}
			else if( argument == "--soundbanks" )
			{
				commandLine.lab.soundBankFolder = AudioLab::FromUtf8Path( value );
			}
			else
			{
				commandLine.lab.soundBanks.push_back( AudioLab::ToWide( value ) );
			}
		}
		return commandLine;
	}

	// Headless runs are under ctest, where a message box would hang the test.
	bool s_headless = false;

	[[noreturn]] void Fail( const std::string& message )
	{
		std::fprintf( stderr, "Audio Lab: %s\n", message.c_str() );
		if( !s_headless )
		{
			AudioLab::Platform::ShowError( "Audio Lab", message );
		}
		AudioLab::TerminateBlue( 1 );
	}

	int RunChecks( AudioLab::LabApp& app, const std::string& spec )
	{
		if( !app.GetChecks().Queue( spec ) )
		{
			std::fprintf( stderr, "No check matches %s.\n", spec.c_str() );
			return 1;
		}
		while( app.GetChecks().IsRunning() )
		{
			app.Update();
			std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
		}

		int failures = 0;
		const std::vector<AudioLab::CheckResult>& results = app.GetChecks().GetResults();
		for( const AudioLab::CheckResult& result : results )
		{
			std::printf( "%s %s/%s (%.1f s)%s%s\n", result.passed ? "PASS" : "FAIL", result.page.c_str(), result.check.c_str(), result.seconds, result.passed ? "" : ": ", result.message.c_str() );
			failures += result.passed ? 0 : 1;
		}
		std::printf( "%s (%d of %zu failed)\n", failures == 0 ? "CHECKS PASSED" : "CHECKS FAILED", failures, results.size() );
		return failures == 0 ? 0 : 1;
	}

	int RunWindow( AudioLab::LabApp& app )
	{
		AudioLab::LabWindow window;
		std::string error;
		if( !window.Create( "Audio Lab - CarbonAudio", 1500, 950, error ) )
		{
			Fail( error );
		}

		AudioLab::LabUi ui( app );
		while( window.BeginFrame() )
		{
			app.Update();
			ui.SetUiScale( window.GetUiScale() );
			ui.Draw();
			window.EndFrame();
		}
		window.Destroy();
		return 0;
	}
}

int main( int argc, char** argv )
{
	const CommandLine commandLine = ParseCommandLine( AudioLab::Platform::GetArguments( argc, argv ) );
	s_headless = commandLine.lab.headless;
	if( !commandLine.error.empty() )
	{
		// Before Blue boots, so the process can simply end.
		std::fprintf( stderr, "Audio Lab: %s\nUsage: %s\n", commandLine.error.c_str(), USAGE );
		if( !s_headless )
		{
			AudioLab::Platform::ShowError( "Audio Lab", commandLine.error + "\n\nUsage: " + USAGE );
		}
		return 2;
	}
	AudioLab::SetLogToConsole( commandLine.lab.headless );

	std::string error;
	if( !AudioLab::BootBlue( commandLine.lab.soundBankFolder, error ) )
	{
		Fail( error );
	}

	AudioLab::LabApp app;
	if( !app.Start( commandLine.lab, error ) )
	{
		Fail( error );
	}
	if( !commandLine.page.empty() )
	{
		const int page = app.FindPage( commandLine.page );
		if( page < 0 )
		{
			std::string pages;
			for( const auto& known : app.GetPages() )
			{
				pages += std::string( pages.empty() ? "" : ", " ) + known->GetID();
			}
			Fail( "There is no page " + commandLine.page + "; the pages are " + pages + "." );
		}
		app.OpenPage( page );
	}

	const int exitCode = !commandLine.checks.empty() ? RunChecks( app, commandLine.checks ) : RunWindow( app );

	app.Stop();
	AudioLab::TerminateBlue( exitCode );
}
