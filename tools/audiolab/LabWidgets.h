// Copyright © 2026 CCP ehf.

#pragma once

#include "LabScene.h"
#include "SoundBankFolder.h"

#include <imgui.h>

#include <cstdint>
#include <string>

namespace AudioLab
{
	/// A combo of the folder's events; true when the user picked one.
	bool EventCombo( const SoundBankFolder& folder, const char* label, std::string& eventName, bool allowNone );
	/// A "(?)" after the last item with the text as its tooltip.
	void HelpMarker( const char* text );
	/// The room CarbonAudio placed the game object in, by the scene's name for it, or how Wwise places it.
	std::string AssignedRoomText( const LabScene& scene, uint64_t gameObjectID );
	/// A room CarbonAudio sent to Wwise, by the scene's name for it, or "Outdoors".
	std::string WwiseRoomText( const LabScene& scene, uint64_t roomID );
	/// A text field that changes the value only when the edit ends, e.g. for names whose change is sent on; true then.
	bool InputString( const char* label, std::string& value );
	/// A position on the ground plane; true when it was changed.
	bool DragPosition( const char* label, float& x, float& z );
	/// On the same line, moves the cursor so that the next items, width wide in all, end at the right edge.
	void AlignRight( float width );
	/// The width ImGui::Button gives a label.
	float ButtonWidth( const char* label );
	/// The width ImGui::Checkbox gives a label.
	float CheckboxWidth( const char* label );

	/// A middle dot between the parts of a line, in UTF-8.
	constexpr const char* SEPARATOR = " \xC2\xB7 ";
	/// A multiplication sign, in UTF-8.
	constexpr const char* TIMES = "\xC3\x97";

	const ImVec4 WARNING_COLOUR( 1.0f, 0.8f, 0.35f, 1.0f );
	const ImVec4 PASS_COLOUR( 0.45f, 0.85f, 0.55f, 1.0f );
	const ImVec4 FAIL_COLOUR( 1.0f, 0.45f, 0.45f, 1.0f );
}
