// Copyright © 2026 CCP ehf.

#include "LabApp.h"
#include "Platform.h"

#include <memory>
#include <string>

namespace AudioLab
{
	namespace
	{
		bool AssignedTo( uint64_t gameObjectID, uint64_t roomID )
		{
			uint64_t assigned = 0;
			return CarbonAudioHost::GetAssignedRoom( gameObjectID, assigned ) && assigned == roomID;
		}

		bool PlaysOnPlayers( LabApp& app, std::string& detail )
		{
			if( app.GetDefaultEvent().empty() )
			{
				return true;
			}
			const bool ui = !CarbonAudioHost::GetPlayingEvents( CarbonAudioHost::GetUIPlayer() ).empty();
			const bool music = !CarbonAudioHost::GetPlayingEvents( CarbonAudioHost::GetMusicPlayer() ).empty();
			detail = std::string( "UI player " ) + ( ui ? "plays" : "silent" ) + ", music player " + ( music ? "plays" : "silent" );
			return ui && music;
		}

		/// Release builds have no resource monitor; like the status bar, this takes no Wwise communication for one.
		bool HasResourceStats( LabApp&, std::string& detail )
		{
			CarbonAudioHost::ResourceMonitorSummary summary;
			if( !CarbonAudioHost::GetWwiseCommunicationEnabled() || CarbonAudioHost::GetResourceMonitorSummary( summary ) )
			{
				return true;
			}
			detail = "no resource monitor summary from Wwise";
			return false;
		}

		class SandboxPage : public LabPage
		{
		public:
			const char* GetID() const override { return "sandbox"; }
			const char* GetName() const override { return "Sandbox"; }
			const char* GetHelp() const override
			{
				return "A free scene. Draw rooms and place emitters with the tools above the scene, walk the listener with "
					   "W A S D, and change what you select in the Selection tab and the rest in the Engine tab. Nothing is "
					   "set up for you.";
			}

			std::vector<LabCheck> GetChecks() override
			{
				std::vector<LabCheck> checks;

				checks.push_back( { "rooms", "A room around the listener and an emitter goes to Wwise and both are assigned to it, until it is removed.", []( CheckScript& script ) {
					// The Wwise room ID, once the room is in Wwise; the room itself is gone by the last step.
					const auto roomID = std::make_shared<uint64_t>( 0 );
					script.Do( "an emitter, and a room around it and the listener", []( LabApp& app ) {
						LabScene& scene = app.GetScene();
						scene.AddEmitter( 2.0f, 1.0f, app.GetDefaultEvent() );
						const std::vector<std::string>& busses = app.GetFolder().auxBusses;
						scene.AddRoom( 0.0f, 0.0f, 12.0f, 12.0f, busses.empty() ? std::string() : busses.front() );
					} );
					script.Expect( "both are created through BeClasses", []( LabApp& app, std::string& ) {
						return app.GetScene().emitters.size() == 1 && app.GetScene().rooms.size() == 1;
					}, 0.0 );
					script.Expect( "the room is in Wwise", [roomID]( LabApp& app, std::string& ) {
						CarbonAudioHost::RoomStatus status;
						if( !CarbonAudioHost::GetRoomStatus( app.GetScene().rooms[0].object.p, status ) || !status.inWwise )
						{
							return false;
						}
						*roomID = status.roomID;
						return true;
					} );
					script.Expect( "the listener and the emitter are assigned to it", [roomID]( LabApp& app, std::string& ) {
						return AssignedTo( CarbonAudioHost::GetListenerID(), *roomID ) && AssignedTo( app.GetScene().emitters[0].gameObjectID, *roomID );
					} );
					script.Do( "play the emitter's loop", []( LabApp& app ) { app.GetScene().Play( app.GetScene().emitters[0] ); } );
					script.Expect( "it plays", []( LabApp& app, std::string& ) {
						return app.GetDefaultEvent().empty() || !CarbonAudioHost::GetPlayingEvents( app.GetScene().emitters[0].object.p ).empty();
					} );
					script.Do( "remove the room", []( LabApp& app ) { app.GetScene().RemoveRoom( app.GetScene().rooms[0].id ); } );
					script.Expect( "the listener and the emitter left it", [roomID]( LabApp& app, std::string& ) {
						return !AssignedTo( CarbonAudioHost::GetListenerID(), *roomID ) && !AssignedTo( app.GetScene().emitters[0].gameObjectID, *roomID );
					} );
				} } );

				checks.push_back( { "restart", "The UI and music players play and Wwise's resource stats come in, also after the SoundBanks are reloaded.", []( CheckScript& script ) {
					const auto reloaded = std::make_shared<bool>( false );
					const auto error = std::make_shared<std::string>();
					script.Expect( "the resource stats come in", HasResourceStats );
					script.Do( "play the default loop on the UI and music players", []( LabApp& app ) {
						if( app.GetDefaultEvent().empty() )
						{
							return;
						}
						for( IRoot* player : { CarbonAudioHost::GetUIPlayer(), CarbonAudioHost::GetMusicPlayer() } )
						{
							BluePtr<ITr2AudEmitter> emitter = BlueCastPtr( player );
							emitter->SendEvent( ToWide( app.GetDefaultEvent() ) );
						}
					} );
					script.Expect( "both play", PlaysOnPlayers );
					script.Do( "reload the SoundBanks, which restarts CarbonAudio", [reloaded, error]( LabApp& app ) {
						*reloaded = app.ReloadSoundBanks( *error );
					} );
					script.Expect( "the reload succeeded", [reloaded, error]( LabApp&, std::string& detail ) {
						detail = *error;
						return *reloaded;
					}, 0.0 );
					script.Expect( "the resource stats come in again", HasResourceStats );
					script.Expect( "both play again", PlaysOnPlayers );
				} } );

				return checks;
			}
		};
	}

	std::unique_ptr<LabPage> CreateSandboxPage()
	{
		return std::make_unique<SandboxPage>();
	}
}
