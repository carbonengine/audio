// Copyright © 2026 CCP ehf.

#pragma once

#include "LabChecks.h"
#include "LabPage.h"
#include "LabScene.h"
#include "LabSightlines.h"
#include "SoundBankFolder.h"
#include "WwiseLink.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace AudioLab
{
	struct LabOptions
	{
		std::filesystem::path soundBankFolder;
		/// SoundBanks to load instead of every SoundBank of the folder.
		std::vector<std::wstring> soundBanks;
		/// No window and no link to Wwise Authoring.
		bool headless = false;
	};

	/// The lab's state: the SoundBank folder CarbonAudio runs on, the scene, the pages and their checks, and a watcher
	/// that reloads the SoundBanks when Wwise generates them again.
	class LabApp
	{
	public:
		/// Reads the SoundBank folder, starts CarbonAudio on it and opens the first page.
		bool Start( const LabOptions& options, std::string& error );
		/// Every frame: reloads changed SoundBanks, runs checks, updates the page, places the scene and ticks CarbonAudio.
		void Update();
		/// Reads the SoundBank folder again and restarts CarbonAudio on it; the scene stays.
		bool ReloadSoundBanks( std::string& error );
		/// Looks at the SoundBank folder now instead of at the next poll.
		void CheckSoundBankFolderSoon();
		void Stop();

		const SoundBankFolder& GetFolder() const { return m_folder; }
		/// The SoundBanks given to CarbonAudio, Init.bnk aside.
		const std::vector<std::wstring>& GetSoundBanks() const { return m_soundBanks; }
		LabScene& GetScene() { return m_scene; }
		const std::string& GetDefaultEvent() const { return m_defaultEvent; }
		/// The link to Wwise Authoring; not started in headless runs.
		WwiseLink& GetWwise() { return m_wwise; }

		const std::vector<std::unique_ptr<LabPage>>& GetPages() const { return m_pages; }
		LabPage& GetPage() { return *m_pages[m_page]; }
		int GetPageIndex() const { return m_page; }
		/// -1 when no page has the ID.
		int FindPage( const std::string& id ) const;
		/// Leaves the open page, clears the scene, puts the lab's default switches back, enters the page and builds its
		/// demo scene. Ends a running check run, which needs its own page and scene.
		void OpenPage( int index );
		/// Changes whenever a page opens, so the UI can drop its selection.
		int GetPageGeneration() const { return m_pageGeneration; }
		/// With a window, pages wait to build their demo scene until the UI calls this after its splash, so nothing
		/// plays under it.
		void ReleaseDemo();

		/// While checks run, nothing but the checks may change the page, the scene or the SoundBanks.
		CheckRunner& GetChecks() { return m_checks; }

	private:
		void WatchSoundBankFolder();
		void HandleWwiseNotices();
		/// demo: whether the page builds its demo scene; not for checks.
		void EnterPage( int index, bool demo );
		void ResetSwitches();
		/// Runs the checks; once a run ends, opens the page the user was on again.
		void RunChecks();

		LabOptions m_options;
		SoundBankFolder m_folder;
		std::vector<std::wstring> m_soundBanks;
		LabScene m_scene;
		std::string m_defaultEvent;
		WwiseLink m_wwise;
		/// The scene's walls answer CarbonAudio's sightline query, which CarbonAudio holds weakly. ResetSwitches gives it
		/// back to CarbonAudio after a check took it away.
		BluePtr<IEveObstructionQuery> m_sightlines;
		/// CarbonAudio's occlusion fade rate at start, which ResetSwitches puts back.
		float m_defaultFadeRate = 1.0f;

		std::vector<std::unique_ptr<LabPage>> m_pages;
		int m_page = 0;
		int m_pageGeneration = 0;
		bool m_pageEntered = false;
		/// Until ReleaseDemo, with a window; meanwhile the open page's demo scene may be waiting.
		bool m_demoHeld = false;
		bool m_demoPending = false;
		CheckRunner m_checks;
		/// Set while a check run goes on, with the page it started from.
		bool m_inCheckRun = false;
		int m_pageBeforeChecks = 0;

		uint64_t m_loadedFingerprint = 0;
		uint64_t m_pendingFingerprint = 0;
		std::chrono::steady_clock::time_point m_pendingSince;
		std::chrono::steady_clock::time_point m_nextPoll;
	};
}
