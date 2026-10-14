// Copyright © 2026 CCP ehf.

#include "LabApp.h"

#include "BlueBoot.h"
#include "Platform.h"

namespace
{
	constexpr std::chrono::milliseconds POLL_INTERVAL( 500 );
	/// Wwise writes a generation over several seconds; wait until the folder stops changing.
	constexpr std::chrono::milliseconds SETTLE_TIME( 1000 );
}

namespace AudioLab
{
	bool LabApp::Start( const LabOptions& options, std::string& error )
	{
		m_options = options;
		if( !LoadSoundBankFolder( options.soundBankFolder, m_folder, error ) )
		{
			return false;
		}
		m_soundBanks = options.soundBanks.empty() ? m_folder.soundBanks : options.soundBanks;
		m_loadedFingerprint = GetSoundBankFolderFingerprint( options.soundBankFolder );

		// The name the Wwise Profiler sees this process under.
		const std::string applicationName = "Audio Lab " + std::to_string( Platform::GetProcessId() );
		CarbonAudioHost::Settings settings;
		settings.soundBankPath = L"soundbanks:/";
		settings.language = L"English(US)";
		settings.applicationName = applicationName;
		settings.roomsEnabled = true;
		settings.audioCulling = false;
		if( !CarbonAudioHost::Start( m_folder.staticData, settings, m_soundBanks ) )
		{
			error = "CarbonAudio did not start; see the log.";
			return false;
		}
		m_defaultEvent = PickLoopEvent( m_folder );
		m_defaultFadeRate = CarbonAudioHost::GetObstructionOcclusionFadeRate();

		if( !options.headless )
		{
			WwiseLink::Options wwise;
			wwise.appName = applicationName;
			wwise.labProject = FromUtf8Path( AUDIOLAB_WWISE_PROJECT );
			// Release builds have no Wwise communication for the Profiler to connect to.
			wwise.autoAttachProfiler = CarbonAudioHost::GetWwiseCommunicationEnabled();
			m_wwise.Start( wwise );
		}

		RegisterLabClasses();
		m_sightlines = CreateSightlineQuery( m_scene );
		if( !m_sightlines )
		{
			error = "The lab's sightline query could not be created.";
			return false;
		}

		m_pages = CreatePages();
		m_checks.SetPages( m_pages );
		OpenPage( 0 );
		return true;
	}

	void LabApp::Update()
	{
		HandleWwiseNotices();
		WatchSoundBankFolder();
		RunChecks();
		GetPage().Update( *this );
		m_scene.Update();
	}

	int LabApp::FindPage( const std::string& id ) const
	{
		for( size_t i = 0; i < m_pages.size(); ++i )
		{
			if( id == m_pages[i]->GetID() )
			{
				return static_cast<int>( i );
			}
		}
		return -1;
	}

	void LabApp::OpenPage( int index )
	{
		if( index < 0 || index >= static_cast<int>( m_pages.size() ) )
		{
			return;
		}
		if( m_checks.IsRunning() )
		{
			m_checks.Stop( "interrupted: another page was opened" );
		}
		m_inCheckRun = false;
		EnterPage( index, true );
	}

	void LabApp::RunChecks()
	{
		if( m_checks.IsRunning() && !m_inCheckRun )
		{
			m_inCheckRun = true;
			m_pageBeforeChecks = m_page;
		}
		if( m_checks.StartNext() )
		{
			EnterPage( m_checks.GetRunningPage(), false );
		}
		m_checks.Step( *this );
		// The last check leaves its scene and switches behind; the user gets their page back, freshly built. Headless
		// runs end with the checks.
		if( m_inCheckRun && !m_checks.IsRunning() )
		{
			m_inCheckRun = false;
			if( !m_options.headless )
			{
				EnterPage( m_pageBeforeChecks, true );
			}
		}
	}

	void LabApp::EnterPage( int index, bool demo )
	{
		if( m_pageEntered )
		{
			GetPage().Leave( *this );
		}
		m_scene.Clear();
		ResetSwitches();
		m_page = index;
		GetPage().Enter( *this );
		if( demo )
		{
			GetPage().BuildDemo( *this );
		}
		m_pageEntered = true;
		++m_pageGeneration;
	}

	void LabApp::ResetSwitches()
	{
		CarbonAudioHost::StopAll();
		CarbonAudioHost::SetObstructionOcclusionEnabled( false );
		CarbonAudioHost::SetObstructionOcclusionFadeRate( m_defaultFadeRate );
		CarbonAudioHost::SetObstructionQuery( m_sightlines.p );
		CarbonAudioHost::ClearObstructionOcclusion();
		CarbonAudioHost::SetGeometryEnabled( false );
		CarbonAudioHost::SetRoomsEnabled( true );
	}

	void LabApp::HandleWwiseNotices()
	{
		for( const WwiseNotice& notice : m_wwise.Drain() )
		{
			AddLogLine( notice.severity, notice.channel, notice.message );
			if( notice.kind == WwiseNotice::Kind::GenerationDone )
			{
				CheckSoundBankFolderSoon();
			}
		}
	}

	void LabApp::WatchSoundBankFolder()
	{
		// A reload restarts CarbonAudio under the running check; it waits for the checks to finish.
		const auto now = std::chrono::steady_clock::now();
		if( now < m_nextPoll || m_checks.IsRunning() )
		{
			return;
		}
		m_nextPoll = now + POLL_INTERVAL;

		const uint64_t fingerprint = GetSoundBankFolderFingerprint( m_options.soundBankFolder );
		if( fingerprint == 0 || fingerprint == m_loadedFingerprint )
		{
			m_pendingFingerprint = 0;
			return;
		}
		if( fingerprint != m_pendingFingerprint )
		{
			m_pendingFingerprint = fingerprint;
			m_pendingSince = now;
			return;
		}
		if( now - m_pendingSince < SETTLE_TIME )
		{
			return;
		}

		std::string error;
		if( !ReloadSoundBanks( error ) )
		{
			AddLogLine( LOG_ERROR, "AudioLab", error );
			m_loadedFingerprint = fingerprint;
		}
	}

	bool LabApp::ReloadSoundBanks( std::string& error )
	{
		SoundBankFolder folder;
		if( !LoadSoundBankFolder( m_options.soundBankFolder, folder, error ) )
		{
			return false;
		}
		m_folder = std::move( folder );
		m_soundBanks = m_options.soundBanks.empty() ? m_folder.soundBanks : m_options.soundBanks;
		m_loadedFingerprint = GetSoundBankFolderFingerprint( m_options.soundBankFolder );
		m_pendingFingerprint = 0;

		CarbonAudioHost::UpdateStaticData( m_folder.staticData );
		if( !CarbonAudioHost::Restart( m_soundBanks ) )
		{
			error = "CarbonAudio did not restart on the new SoundBanks; see the log.";
			return false;
		}
		if( m_defaultEvent.empty() )
		{
			m_defaultEvent = PickLoopEvent( m_folder );
		}
		AddLogLine( LOG_NOTICE, "AudioLab", "Reloaded the SoundBanks from " + ToUtf8Path( m_options.soundBankFolder ) + "." );
		return true;
	}

	void LabApp::CheckSoundBankFolderSoon()
	{
		m_nextPoll = std::chrono::steady_clock::now();
	}

	void LabApp::Stop()
	{
		m_wwise.Shutdown();
		CarbonAudioHost::Stop();
	}
}
