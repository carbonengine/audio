// Copyright © 2026 CCP ehf.

#include "LabWidgets.h"

#include <imgui_stdlib.h>

namespace AudioLab
{
	bool EventCombo( const SoundBankFolder& folder, const char* label, std::string& eventName, bool allowNone )
	{
		bool changed = false;
		if( ImGui::BeginCombo( label, eventName.empty() ? "(none)" : eventName.c_str() ) )
		{
			if( allowNone && ImGui::Selectable( "(none)", eventName.empty() ) )
			{
				eventName.clear();
				changed = true;
			}
			for( const std::string& name : folder.eventNames )
			{
				if( ImGui::Selectable( name.c_str(), name == eventName ) )
				{
					eventName = name;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	void HelpMarker( const char* text )
	{
		ImGui::SameLine();
		ImGui::TextDisabled( "(?)" );
		ImGui::SetItemTooltip( "%s", text );
	}

	std::string AssignedRoomText( const LabScene& scene, uint64_t gameObjectID )
	{
		uint64_t roomID = 0;
		if( !CarbonAudioHost::GetAssignedRoom( gameObjectID, roomID ) )
		{
			return "Wwise containment (no override)";
		}
		return WwiseRoomText( scene, roomID );
	}

	std::string WwiseRoomText( const LabScene& scene, uint64_t roomID )
	{
		if( roomID == CarbonAudioHost::kOutdoorRoomID )
		{
			return "Outdoors";
		}
		const LabRoom* room = scene.FindRoomByWwiseID( roomID );
		return room ? room->settings.name : "Room " + std::to_string( roomID );
	}

	bool InputString( const char* label, std::string& value )
	{
		// ImGui keeps the text being edited itself; the copy only matters on the frame the edit ends.
		std::string edited = value;
		ImGui::InputText( label, &edited );
		if( ImGui::IsItemDeactivatedAfterEdit() )
		{
			value = edited;
			return true;
		}
		return false;
	}

	bool DragPosition( const char* label, float& x, float& z )
	{
		float position[2] = { x, z };
		if( !ImGui::DragFloat2( label, position, 0.1f, 0.0f, 0.0f, "%.1f m" ) )
		{
			return false;
		}
		x = position[0];
		z = position[1];
		return true;
	}

	void AlignRight( float width )
	{
		ImGui::SameLine();
		const float available = ImGui::GetContentRegionAvail().x;
		if( available > width )
		{
			ImGui::SetCursorPosX( ImGui::GetCursorPosX() + available - width );
		}
	}

	float ButtonWidth( const char* label )
	{
		return ImGui::CalcTextSize( label, nullptr, true ).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	}

	float CheckboxWidth( const char* label )
	{
		return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize( label, nullptr, true ).x;
	}
}
