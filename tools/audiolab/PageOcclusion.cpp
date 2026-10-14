// Copyright © 2026 CCP ehf.

#include "LabApp.h"
#include "LabWidgets.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace AudioLab
{
	namespace
	{
		float OcclusionOf( LabApp& app, size_t emitter )
		{
			return CarbonAudioHost::GetEmitterOcclusion( app.GetScene().emitters[emitter].gameObjectID );
		}

		/// Whether the last sightline pass checked the emitter, and what it found.
		bool Sightline( LabApp& app, size_t emitter, bool& blocked )
		{
			const uint64_t id = app.GetScene().emitters[emitter].gameObjectID;
			for( const CarbonAudioHost::SightlineResult& result : CarbonAudioHost::GetLastSightlineResults() )
			{
				if( result.gameObjectID == id )
				{
					blocked = result.blocked;
					return true;
				}
			}
			return false;
		}

		void AddPlaying( LabApp& app, float x, float z )
		{
			LabScene& scene = app.GetScene();
			if( LabEmitter* emitter = scene.AddEmitter( x, z, app.GetDefaultEvent() ) )
			{
				scene.Play( *emitter );
			}
		}

		/// Across the line from the listener at the origin to (0, -20).
		void AddWallAhead( LabApp& app )
		{
			app.GetScene().AddWall( -5.0f, -10.0f, 5.0f, -10.0f );
		}

		std::string Describe( float occlusion )
		{
			char text[32];
			std::snprintf( text, sizeof( text ), "occlusion %.2f", occlusion );
			return text;
		}

		class OcclusionPage : public LabPage
		{
		public:
			const char* GetID() const override { return "occlusion"; }
			const char* GetName() const override { return "Occlusion"; }
			const char* GetHelp() const override
			{
				return "Game-driven occlusion muffles an emitter whose sightline to the listener is blocked. In the game "
					   "destiny answers CarbonAudio's sightline query; here the walls do. CarbonAudio asks when an emitter "
					   "starts playing and applies the answer at once, then every 0.2 s while it plays, fading towards it "
					   "at the fade rate. With Spatial Audio geometry on, a blocked sightline gives no occlusion: "
					   "transmission handles walls there.\n"
					   "Look for: red sightlines through the wall, the emitter behind it getting muffled, and the occlusion "
					   "fading as you walk around the wall (W A S D). Draw more walls with the Wall tool (4); Engine > "
					   "Occlusion lists every emitter's occlusion.";
			}

			void Enter( LabApp& ) override
			{
				CarbonAudioHost::SetObstructionOcclusionEnabled( true );
			}

			void BuildDemo( LabApp& app ) override
			{
				// A wall, and an emitter behind it.
				AddWallAhead( app );
				AddPlaying( app, 0.0f, -20.0f );
			}

			void DrawControls( LabApp& ) override
			{
				if( !CarbonAudioHost::GetObstructionOcclusionEnabled() )
				{
					ImGui::TextColored( WARNING_COLOUR, "Occlusion is off; the toolbar turns it on." );
				}
			}

			std::vector<LabCheck> GetChecks() override
			{
				std::vector<LabCheck> checks;

				checks.push_back( { "blocked-at-start", "An emitter that starts playing behind a wall is fully occluded at once, without fading in.", []( CheckScript& script ) {
					script.Do( "a wall, and a loop starting behind it", []( LabApp& app ) {
						AddWallAhead( app );
						AddPlaying( app, 0.0f, -20.0f );
					} );
					// A fade at the default rate would take a second.
					script.Expect( "it is fully occluded", []( LabApp& app, std::string& detail ) {
						detail = Describe( OcclusionOf( app, 0 ) );
						return OcclusionOf( app, 0 ) >= 0.99f;
					}, 0.5 );
				} } );

				checks.push_back( { "clear", "An emitter whose sightline misses every wall is not occluded.", []( CheckScript& script ) {
					script.Do( "a wall, and a loop beside it", []( LabApp& app ) {
						AddWallAhead( app );
						AddPlaying( app, -20.0f, -20.0f );
					} );
					script.Expect( "its sightline is checked and clear", []( LabApp& app, std::string& ) {
						bool blocked = true;
						return Sightline( app, 0, blocked ) && !blocked;
					} );
					script.ExpectFor( "it is not occluded", []( LabApp& app, std::string& detail ) {
						detail = Describe( OcclusionOf( app, 0 ) );
						return OcclusionOf( app, 0 ) == 0.0f;
					}, 0.5 );
				} } );

				checks.push_back( { "fade-in", "A wall that comes between a playing emitter and the listener fades its occlusion in.", []( CheckScript& script ) {
					script.Do( "a loop in view", []( LabApp& app ) { AddPlaying( app, 0.0f, -20.0f ); } );
					script.Expect( "its sightline is checked and clear", []( LabApp& app, std::string& ) {
						bool blocked = true;
						return Sightline( app, 0, blocked ) && !blocked;
					} );
					script.Do( "a wall across the sightline", []( LabApp& app ) { AddWallAhead( app ); } );
					script.Expect( "the occlusion is part of the way in", []( LabApp& app, std::string& detail ) {
						const float occlusion = OcclusionOf( app, 0 );
						detail = Describe( occlusion );
						return occlusion > 0.05f && occlusion < 0.95f;
					}, 1.0 );
					script.Expect( "then all the way", []( LabApp& app, std::string& detail ) {
						detail = Describe( OcclusionOf( app, 0 ) );
						return OcclusionOf( app, 0 ) >= 0.99f;
					} );
				} } );

				checks.push_back( { "off", "Turning occlusion off fades every emitter back to clear.", []( CheckScript& script ) {
					script.Do( "a wall, and a loop behind it", []( LabApp& app ) {
						AddWallAhead( app );
						AddPlaying( app, 0.0f, -20.0f );
					} );
					script.Expect( "it is occluded", []( LabApp& app, std::string& detail ) {
						detail = Describe( OcclusionOf( app, 0 ) );
						return OcclusionOf( app, 0 ) >= 0.99f;
					} );
					script.Do( "turn occlusion off", []( LabApp& ) { CarbonAudioHost::SetObstructionOcclusionEnabled( false ); } );
					script.Expect( "it is clear again", []( LabApp& app, std::string& detail ) {
						detail = Describe( OcclusionOf( app, 0 ) );
						return OcclusionOf( app, 0 ) <= 0.01f;
					} );
				} } );

				checks.push_back( { "geometry", "With Spatial Audio geometry on, a blocked sightline gives no occlusion.", []( CheckScript& script ) {
					script.Do( "geometry on, a wall, and a loop behind it", []( LabApp& app ) {
						CarbonAudioHost::SetGeometryEnabled( true );
						AddWallAhead( app );
						AddPlaying( app, 0.0f, -20.0f );
					} );
					script.Expect( "its sightline is checked and blocked", []( LabApp& app, std::string& ) {
						bool blocked = false;
						return Sightline( app, 0, blocked ) && blocked;
					} );
					script.ExpectFor( "it is not occluded", []( LabApp& app, std::string& detail ) {
						detail = Describe( OcclusionOf( app, 0 ) );
						return OcclusionOf( app, 0 ) == 0.0f;
					}, 0.5 );
				} } );

				checks.push_back( { "game-blockage", "Without a sightline query, the blockage the game reports is the occlusion.", []( CheckScript& script ) {
					script.Do( "no sightline query, and a loop", []( LabApp& app ) {
						CarbonAudioHost::SetObstructionQuery( nullptr );
						AddPlaying( app, 0.0f, -20.0f );
					} );
					script.Do( "report it half blocked", []( LabApp& app ) {
						CarbonAudioHost::SetEmitterLineOfSightBlockage( app.GetScene().emitters[0].gameObjectID, 0.5f );
					} );
					script.Expect( "its occlusion is a half", []( LabApp& app, std::string& detail ) {
						const float occlusion = OcclusionOf( app, 0 );
						detail = Describe( occlusion );
						return occlusion > 0.49f && occlusion < 0.51f;
					} );
				} } );

				return checks;
			}
		};
	}

	std::unique_ptr<LabPage> CreateOcclusionPage()
	{
		return std::make_unique<OcclusionPage>();
	}
}
