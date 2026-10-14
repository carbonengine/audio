// Copyright © 2026 CCP ehf.

#pragma once

#include "LabScene.h"
#include "LabSelection.h"

#include <string>

namespace AudioLab
{
	class LabApp;
	class SceneCanvas;
	struct SoundBankFolder;

	/// The sidebar's Selection tab: every thing in the scene in a list, and the settings of what is selected.
	class SelectionPanel
	{
	public:
		SelectionPanel( LabApp& app, Selection& selection, SceneCanvas& canvas );

		void Draw();

	private:
		void DrawObjectList();
		void DrawListenerSettings();
		void DrawRoomSettings( LabRoom& room );
		void DrawPortalSettings( LabPortal& portal );
		void DrawEmitterSettings( LabEmitter& emitter );
		void DrawWallSettings( LabWall& wall );
		void DrawMultipleSettings();

		LabScene& m_scene;
		const SoundBankFolder& m_folder;
		Selection& m_selection;
		/// For the listener's walk speed.
		SceneCanvas& m_canvas;

		std::string m_emitterRTPC;
		float m_emitterRTPCValue = 0.0f;
		std::string m_switchGroup;
		std::string m_switchState;
	};
}
