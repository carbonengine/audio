// Copyright © 2026 CCP ehf.

#include "LabEnginePanel.h"

#include "BlueBoot.h"
#include "LabApp.h"
#include "LabWidgets.h"
#include "Platform.h"

#include <imgui_stdlib.h>

#include <map>

namespace
{
	const char* BankStatusText( CarbonAudioHost::SoundBankStatus status )
	{
		switch( status )
		{
		case CarbonAudioHost::SoundBankStatus::Loading:
			return "loading";
		case CarbonAudioHost::SoundBankStatus::Loaded:
			return "loaded";
		case CarbonAudioHost::SoundBankStatus::Unloading:
			return "unloading";
		default:
			return "not loaded";
		}
	}

	BluePtr<ITr2AudEmitter> AsEmitter( IRoot* object )
	{
		BluePtr<ITr2AudEmitter> emitter = BlueCastPtr( object );
		return emitter;
	}
}

namespace AudioLab
{
	EnginePanel::EnginePanel( LabApp& app ) :
		m_app( app ),
		m_scene( app.GetScene() ),
		m_folder( app.GetFolder() ),
		m_playerEvent( app.GetDefaultEvent() )
	{
	}

	void EnginePanel::Draw()
	{
		if( ImGui::CollapsingHeader( "SoundBanks" ) )
		{
			DrawSoundBanks();
		}
		if( ImGui::CollapsingHeader( "Spatial Audio" ) )
		{
			DrawSpatialAudio();
		}
		if( ImGui::CollapsingHeader( "Occlusion" ) )
		{
			DrawOcclusion();
		}
		if( ImGui::CollapsingHeader( "Global RTPCs and states" ) )
		{
			DrawGlobals();
		}
		if( ImGui::CollapsingHeader( "UI and music players" ) )
		{
			DrawPlayers();
		}
		if( ImGui::CollapsingHeader( "Output device" ) )
		{
			DrawOutput();
		}
	}

	void EnginePanel::DrawSoundBanks()
	{
		ImGui::PushTextWrapPos( 0.0f );
		ImGui::TextDisabled( "%s", ToUtf8Path( m_folder.path ).c_str() );
		ImGui::PopTextWrapPos();
		if( ImGui::Button( "Reload SoundBanks" ) )
		{
			std::string error;
			if( !m_app.ReloadSoundBanks( error ) )
			{
				AddLogLine( LOG_ERROR, "AudioLab", error );
			}
		}
		HelpMarker( "Reads SoundbanksInfo.json again and restarts CarbonAudio on it. Happens by itself when Wwise writes the folder." );
		ImGui::Text( "%zu events", m_folder.eventNames.size() );
		ImGui::PushTextWrapPos( 0.0f );
		for( const std::string& warning : m_folder.warnings )
		{
			ImGui::TextColored( WARNING_COLOUR, "%s", warning.c_str() );
		}
		ImGui::PopTextWrapPos();

		if( ImGui::BeginTable( "banks", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp ) )
		{
			ImGui::TableSetupColumn( "SoundBank" );
			ImGui::TableSetupColumn( "Status" );
			ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed );
			ImGui::TableHeadersRow();
			for( const auto& [bank, essential] : m_folder.staticData.soundBanks )
			{
				const CarbonAudioHost::SoundBankStatus status = CarbonAudioHost::GetSoundBankStatus( bank );
				const std::string name = ToUtf8( bank );
				ImGui::PushID( name.c_str() );
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted( name.c_str() );
				if( essential )
				{
					ImGui::SameLine();
					ImGui::TextDisabled( "essential" );
				}
				ImGui::TableNextColumn();
				ImGui::TextUnformatted( BankStatusText( status ) );
				ImGui::TableNextColumn();
				if( status == CarbonAudioHost::SoundBankStatus::NotLoaded )
				{
					if( ImGui::SmallButton( "Load" ) )
					{
						CarbonAudioHost::LoadBank( bank );
					}
				}
				else if( ImGui::SmallButton( "Unload" ) )
				{
					CarbonAudioHost::UnloadBank( bank );
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	void EnginePanel::DrawSpatialAudio()
	{
		if( CarbonAudioHost::GetGeometryEnabled() && CarbonAudioHost::GetObstructionOcclusionEnabled() )
		{
			ImGui::TextColored( WARNING_COLOUR, "Geometry is on, so game-driven occlusion is suppressed." );
		}
		CarbonAudioHost::SpatialAudioSettings settings;
		CarbonAudioHost::GetSpatialAudioSettings( settings );
		bool changed = false;
		changed |= ImGui::SliderFloat( "Transmission loss", &settings.transmissionLoss, 0.0f, 1.0f );
		changed |= ImGui::Checkbox( "Diffraction", &settings.enableDiffraction );
		changed |= ImGui::Checkbox( "Diffraction on boundary edges", &settings.enableDiffractionOnBoundaryEdges );
		ImGui::TextDisabled( "These apply the next time geometry is sent." );
		if( changed )
		{
			CarbonAudioHost::SetSpatialAudioSettings( settings );
		}
	}

	void EnginePanel::DrawOcclusion()
	{
		if( !CarbonAudioHost::GetObstructionOcclusionEnabled() )
		{
			ImGui::TextDisabled( "Occlusion is off; the toolbar turns it on." );
		}
		else if( CarbonAudioHost::GetGeometryEnabled() )
		{
			ImGui::TextColored( WARNING_COLOUR, "Spatial Audio geometry is on, so this is suppressed." );
		}
		float fadeRate = CarbonAudioHost::GetObstructionOcclusionFadeRate();
		if( ImGui::DragFloat( "Fade rate", &fadeRate, 0.05f, 0.0f, 100.0f, "%.2f per second" ) )
		{
			CarbonAudioHost::SetObstructionOcclusionFadeRate( fadeRate );
		}
		HelpMarker( "How fast occlusion moves while an emitter plays; one that just started gets its occlusion at once." );
		ImGui::PushTextWrapPos( 0.0f );
		ImGui::TextDisabled( "The scene's walls answer CarbonAudio's sightline query, as destiny does in the game. It checks "
							 "the emitters that play and are in range of the listener." );
		ImGui::PopTextWrapPos();

		std::map<uint64_t, bool> sightlines;
		for( const CarbonAudioHost::SightlineResult& result : CarbonAudioHost::GetLastSightlineResults() )
		{
			sightlines[result.gameObjectID] = result.blocked;
		}
		if( ImGui::BeginTable( "occlusion", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp ) )
		{
			ImGui::TableSetupColumn( "Emitter" );
			ImGui::TableSetupColumn( "Sightline" );
			ImGui::TableSetupColumn( "Occlusion" );
			ImGui::TableHeadersRow();
			for( const LabEmitter& emitter : m_scene.emitters )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted( emitter.name.c_str() );
				ImGui::TableNextColumn();
				const auto sightline = sightlines.find( emitter.gameObjectID );
				if( sightline == sightlines.end() )
				{
					ImGui::TextDisabled( "not checked" );
				}
				else
				{
					ImGui::TextColored( sightline->second ? FAIL_COLOUR : PASS_COLOUR, sightline->second ? "blocked" : "clear" );
				}
				ImGui::TableNextColumn();
				ImGui::Text( "%.2f", CarbonAudioHost::GetEmitterOcclusion( emitter.gameObjectID ) );
			}
			ImGui::EndTable();
		}
	}

	void EnginePanel::DrawGlobals()
	{
		ImGui::InputText( "RTPC##Global", &m_globalRTPC );
		if( ImGui::SliderFloat( "Value##Global", &m_globalRTPCValue, 0.0f, 100.0f ) && !m_globalRTPC.empty() )
		{
			CarbonAudioHost::SetGlobalRTPC( ToWide( m_globalRTPC ), m_globalRTPCValue );
		}
		ImGui::BeginDisabled( m_globalRTPC.empty() );
		if( ImGui::Button( "Set RTPC##Global" ) )
		{
			if( !CarbonAudioHost::SetGlobalRTPC( ToWide( m_globalRTPC ), m_globalRTPCValue ) )
			{
				AddLogLine( LOG_WARNING, "AudioLab", "SetGlobalRTPC failed for " + m_globalRTPC + "." );
			}
		}
		ImGui::EndDisabled();

		ImGui::Spacing();
		ImGui::InputText( "State group", &m_stateGroup );
		ImGui::InputText( "State", &m_stateName );
		ImGui::BeginDisabled( m_stateGroup.empty() );
		if( ImGui::Button( "Set state" ) )
		{
			if( !CarbonAudioHost::SetState( ToWide( m_stateGroup ), ToWide( m_stateName ) ) )
			{
				AddLogLine( LOG_WARNING, "AudioLab", "SetState failed for " + m_stateGroup + "." );
			}
		}
		ImGui::EndDisabled();
	}

	void EnginePanel::DrawPlayers()
	{
		EventCombo( m_folder, "Event##Players", m_playerEvent, false );

		BluePtr<ITr2AudEmitter> uiPlayer = AsEmitter( CarbonAudioHost::GetUIPlayer() );
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted( "UI player" );
		ImGui::SameLine();
		ImGui::BeginDisabled( !uiPlayer || m_playerEvent.empty() );
		if( ImGui::Button( "Post##UI" ) )
		{
			uiPlayer->SendEvent( ToWide( m_playerEvent ) );
		}
		ImGui::SameLine();
		if( ImGui::Button( "Post dialogue" ) )
		{
			m_dialoguePlayingID = CarbonAudioHost::PostDialogueEvent( ToWide( m_playerEvent ) );
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if( ImGui::Button( "Stop##UI" ) && uiPlayer )
		{
			CarbonAudioHost::StopAll( uiPlayer.p );
		}
		if( m_dialoguePlayingID != 0 )
		{
			const int32_t position = CarbonAudioHost::GetEventPlayPosition( m_dialoguePlayingID );
			ImGui::TextDisabled( "Dialogue %u: %s", m_dialoguePlayingID, position < 0 ? "finished" : ( std::to_string( position ) + " ms" ).c_str() );
		}

		BluePtr<ITr2AudEmitter> musicPlayer = AsEmitter( CarbonAudioHost::GetMusicPlayer() );
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted( "Music player" );
		ImGui::SameLine();
		ImGui::BeginDisabled( !musicPlayer || m_playerEvent.empty() );
		if( ImGui::Button( "Post##Music" ) )
		{
			musicPlayer->SendEvent( ToWide( m_playerEvent ) );
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if( ImGui::Button( "Stop##Music" ) && musicPlayer )
		{
			CarbonAudioHost::StopAll( musicPlayer.p );
		}
	}

	void EnginePanel::DrawOutput()
	{
		const bool spatial = CarbonAudioHost::GetSpatialAudioEnabled();
		ImGui::Text( "Main output: %s", spatial ? "spatial audio (3D) device" : "stereo device" );
		ImGui::BeginDisabled( !spatial && !CarbonAudioHost::SpatialAudioIsSupported() );
		if( ImGui::Button( spatial ? "Switch to the stereo device" : "Switch to the spatial audio device" ) )
		{
			spatial ? CarbonAudioHost::DisableSpatialAudio() : CarbonAudioHost::EnableSpatialAudio();
		}
		ImGui::EndDisabled();
		if( !CarbonAudioHost::SpatialAudioIsSupported() )
		{
			ImGui::TextDisabled( "The output endpoint does not support spatial audio." );
		}
		if( !m_folder.audioDevices.empty() )
		{
			ImGui::TextDisabled( "Audio devices in Init.bnk:" );
			for( const std::string& device : m_folder.audioDevices )
			{
				ImGui::BulletText( "%s", device.c_str() );
			}
		}
	}

}
