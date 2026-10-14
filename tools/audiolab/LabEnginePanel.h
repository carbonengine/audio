// Copyright © 2026 CCP ehf.

#pragma once

#include <cstdint>
#include <string>

namespace AudioLab
{
	class LabApp;
	class LabScene;
	struct SoundBankFolder;

	/// The sidebar's Engine tab: CarbonAudio's SoundBanks, Spatial Audio, occlusion, globals, players and output.
	class EnginePanel
	{
	public:
		explicit EnginePanel( LabApp& app );

		void Draw();

	private:
		void DrawSoundBanks();
		void DrawSpatialAudio();
		void DrawOcclusion();
		void DrawGlobals();
		void DrawPlayers();
		void DrawOutput();

		LabApp& m_app;
		LabScene& m_scene;
		const SoundBankFolder& m_folder;

		std::string m_globalRTPC;
		float m_globalRTPCValue = 0.0f;
		std::string m_stateGroup;
		std::string m_stateName;
		std::string m_playerEvent;
		uint32_t m_dialoguePlayingID = 0;
	};
}
