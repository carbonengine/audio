// Copyright © 2026 CCP ehf.

#pragma once

#include <CarbonAudioHost.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace AudioLab
{
	/// A SoundBank folder as Wwise generates it, read into CarbonAudio's static data.
	struct SoundBankFolder
	{
		std::filesystem::path path;
		CarbonAudioHost::StaticData staticData;
		/// Every SoundBank but Init.bnk, which CarbonAudio always loads.
		std::vector<std::wstring> soundBanks;
		/// Event names, sorted.
		std::vector<std::string> eventNames;
		/// Aux busses for room reverbs, AudioLab_Hall first when there is one.
		std::vector<std::string> auxBusses;
		/// Wwise audio device sharesets.
		std::vector<std::string> audioDevices;
		std::vector<std::string> warnings;
	};

	/// Reads SoundbanksInfo.json and, when present, SoundPrioritizationMetadata.json for what Wwise does not export,
	/// such as stopped-by events.
	bool LoadSoundBankFolder( const std::filesystem::path& path, SoundBankFolder& out, std::string& error );
	/// Changes whenever Wwise writes the folder again; 0 without a SoundbanksInfo.json.
	uint64_t GetSoundBankFolderFingerprint( const std::filesystem::path& path );
	/// A looping 3D event for new emitters: the lab project's speech, Play_AudioLab_Speech, when the folder has it,
	/// otherwise the first one; empty when there is none.
	std::string PickLoopEvent( const SoundBankFolder& folder );
	/// Metres; 0 when the event has no attenuation.
	float GetEventRadius( const SoundBankFolder& folder, const std::string& eventName );
}
