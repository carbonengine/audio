////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#if defined( _WIN32 )
	#if defined( CarbonAudio_EXPORTS )
		#define CARBON_AUDIO_HOST_API __declspec( dllexport )
	#else
		#define CARBON_AUDIO_HOST_API __declspec( dllimport )
	#endif
#else
	#define CARBON_AUDIO_HOST_API __attribute__( ( visibility( "default" ) ) )
#endif

struct IRoot;

/**
 * @brief Runs CarbonAudio from a native C++ host, without Python.
 *
 * The host boots Blue itself before Start(), the way exefile does before it starts Python: load Blue,
 * BlueModuleStartup(), BlueInitializePaths() and BlueSetSearchPaths(). Python is never initialised.
 * Then it calls Start() once, places the listener and every emitter each frame and calls Tick() once per frame;
 * room assignment follows those position reports, as it does in the game where trinity sends them every frame.
 *
 * Rooms, portals and emitters are Blue objects the host creates through BeClasses ("AudRoom", "AudPortal",
 * "AudEmitter") and places through ITr2BoxVolume and ITr2AudEmitter, as trinity does. This API only adds what Blue cannot do from C++.
 * Everything runs on the host's main thread.
 */
namespace CarbonAudioHost
{
	/// Wwise's outdoor room, i.e. no room.
	constexpr uint64_t kOutdoorRoomID = ~uint64_t( 0 );

	/// Right-handed, the same space trinity uses.
	struct Vec3
	{
		float x;
		float y;
		float z;
	};

	/// One event of the metadata (SoundPrioritizationMetadata.json).
	struct EventInfo
	{
		std::wstring name;
		uint32_t eventID = 0;
		double maxAttenuationRadius = 0.0;
		bool isLoop = false;
		bool is2D = false;
		bool isVital = false;
		/// SoundBank file names, e.g. L"AudioLab.bnk".
		std::vector<std::wstring> soundBanks;
		std::vector<std::wstring> stoppedBy;
	};

	struct StaticData
	{
		std::vector<EventInfo> events;
		/// SoundBank file name and whether it is essential (loaded from the essential folder instead of the root).
		std::vector<std::pair<std::wstring, bool>> soundBanks;
		/// Media ID as text and whether it is essential.
		std::vector<std::pair<std::wstring, bool>> sources;
	};

	/// Wwise Spatial Audio settings; the first eleven are only read when Wwise Spatial Audio starts.
	struct SpatialAudioSettings
	{
		float movementThreshold = 100.0f;
		int numberOfPrimaryRays = 35;
		int maxReflectionOrder = 0;
		int maxDiffractionOrder = 4;
		int maxEmitterRoomAuxSends = 0;
		int diffractionOnReflectionsOrder = 0;
		float maxPathLength = 1000.0f;
		float cpuLimitPercentage = 20.0f;
		int loadBalancingSpread = 1;
		bool enableDiffractionAndTransmission = true;
		bool calcEmitterVirtualPosition = true;
		/// Applied to geometry sent after the change.
		float transmissionLoss = 0.7f;
		bool enableDiffraction = true;
		bool enableDiffractionOnBoundaryEdges = true;
	};

	struct Settings
	{
		/// Blue path (e.g. L"soundbanks:/") or absolute path of the SoundBank folder.
		std::wstring soundBankPath = L"res:/Audio";
		std::wstring language = L"en";
		std::string applicationName = "CarbonAudio";
		bool roomsEnabled = true;
		bool geometryEnabled = false;
		SpatialAudioSettings spatialAudio;
		/// Sound prioritization culling. Off keeps every emitter awake.
		bool audioCulling = false;
		/// Start on the spatial audio (3D audio) device rather than the stereo one.
		bool spatialAudioEnabled = true;
		/// Wwise audio device sharesets for the spatial audio and stereo outputs.
		std::wstring spatialAudioDeviceName = L"System";
		std::wstring stereoAudioDeviceName = L"System_Stereo";
		/// Folders below the SoundBank path for essential media and loose media.
		std::wstring essentialPath = L"Essential_Media";
		std::wstring audioSrcPath = L"Media";
	};

	/// Registers CarbonAudio's classes, loads the static data, starts Wwise with the given SoundBanks (file names;
	/// Init.bnk is implicit) and creates the listener. False, with an error logged, when Blue is not booted or Wwise
	/// does not start.
	CARBON_AUDIO_HOST_API bool Start( const StaticData& staticData, const Settings& settings, const std::vector<std::wstring>& soundBanks );
	/// Once per frame, after placing the listener and the emitters: culling, rooms, rendering.
	CARBON_AUDIO_HOST_API void Tick();
	CARBON_AUDIO_HOST_API void Stop();
	CARBON_AUDIO_HOST_API bool IsRunning();

	CARBON_AUDIO_HOST_API void SetListenerPlacement( const Vec3& front, const Vec3& top, const Vec3& position );
	CARBON_AUDIO_HOST_API uint64_t GetListenerID();

	CARBON_AUDIO_HOST_API void SetRoomsEnabled( bool enabled );
	CARBON_AUDIO_HOST_API bool GetRoomsEnabled();

	/// The authored attributes of an AudRoom.
	struct RoomSettings
	{
		std::string name;
		/// Wwise aux bus name; empty for no reverb.
		std::string reverbAuxBus;
		float reverbLevel = 1.0f;
		float transmissionLoss = 1.0f;
		float priority = 100.0f;
		float auxSendLevelToSelf = 0.0f;
		bool keepRegistered = false;
		/// Event played on the room game object; empty for none.
		std::wstring roomToneEvent;
		bool enabled = true;
	};

	struct RoomStatus
	{
		uint64_t roomID = 0;
		bool inWwise = false;
		bool usableShape = false;
		bool roomTonePosted = false;
		bool roomTonePending = false;
	};

	/// room is an AudRoom (e.g. the ITr2BoxVolume from BeClasses). False for anything else.
	CARBON_AUDIO_HOST_API bool GetRoomSettings( IRoot* room, RoomSettings& out );
	/// Applies like an attribute edit from Graphite: the room is re-sent to Wwise.
	CARBON_AUDIO_HOST_API bool SetRoomSettings( IRoot* room, const RoomSettings& settings );
	CARBON_AUDIO_HOST_API bool GetRoomStatus( IRoot* room, RoomStatus& out );

	/// The authored attributes of an AudPortal.
	struct PortalSettings
	{
		std::string name;
		/// Whether sound passes through, e.g. an open door.
		bool open = true;
		float adjacentRoomBleed = 1.0f;
		float obstruction = 0.0f;
		float occlusion = 0.0f;
		/// Fades obstruction and occlusion over the portal's depth.
		bool obstructionOcclusionTransition = false;
		bool enabled = true;
	};

	struct PortalStatus
	{
		uint64_t portalID = 0;
		bool inWwise = false;
		bool usableShape = false;
		/// The rooms on the portal's front (its local -z side) and back in Wwise; kOutdoorRoomID for outdoors.
		uint64_t frontRoomID = kOutdoorRoomID;
		uint64_t backRoomID = kOutdoorRoomID;
		/// Obstruction and occlusion as last sent to Wwise.
		bool obstructionOcclusionSent = false;
		float sentObstruction = 0.0f;
		float sentOcclusion = 0.0f;
	};

	/// portal is an AudPortal (e.g. the ITr2BoxVolume from BeClasses). False for anything else.
	CARBON_AUDIO_HOST_API bool GetPortalSettings( IRoot* portal, PortalSettings& out );
	/// Applies like an attribute edit from Graphite: the portal is sent again on the next Tick().
	CARBON_AUDIO_HOST_API bool SetPortalSettings( IRoot* portal, const PortalSettings& settings );
	CARBON_AUDIO_HOST_API bool GetPortalStatus( IRoot* portal, PortalStatus& out );

	/// A way Wwise found for sound to reach the listener, around geometry or through portals.
	struct DiffractionPath
	{
		/// Where the path bends, from the listener's end to the emitter's.
		std::vector<Vec3> nodes;
		/// The portal at each node; 0 where the path bends around geometry.
		std::vector<uint64_t> portals;
		Vec3 emitter = {};
		/// [0, 1]
		float diffraction = 0.0f;
		/// [0, 1]
		float transmissionLoss = 0.0f;
		float length = 0.0f;
	};

	/// The paths Wwise found from the game object to the listener in its last update; empty when it found none.
	/// A path without nodes is the direct one.
	CARBON_AUDIO_HOST_API std::vector<DiffractionPath> GetDiffractionPaths( uint64_t gameObjectID );

	/// emitter is an AudEmitter (e.g. the ITr2AudEmitter from BeClasses). 0 for anything else.
	CARBON_AUDIO_HOST_API uint64_t GetGameObjectID( IRoot* emitter );
	CARBON_AUDIO_HOST_API bool StopAll( IRoot* emitter );
	/// Room CarbonAudio placed the game object in (kOutdoorRoomID after it left all rooms). False when CarbonAudio
	/// has not placed it, i.e. Wwise's own containment decides.
	CARBON_AUDIO_HOST_API bool GetAssignedRoom( uint64_t gameObjectID, uint64_t& roomID );

	/// How far a SoundBank has got, as CarbonAudio tracks it.
	enum class SoundBankStatus
	{
		Loading,
		Loaded,
		Unloading,
		NotLoaded,
	};

	/// Loads a SoundBank by file name, e.g. L"AudioLab.bnk", asynchronously; events posted meanwhile wait for it.
	CARBON_AUDIO_HOST_API void LoadBank( const std::wstring& name );
	CARBON_AUDIO_HOST_API void UnloadBank( const std::wstring& name );
	/// Unloads every SoundBank, Init.bnk included.
	CARBON_AUDIO_HOST_API void ClearBanks();
	/// SoundBanks that are loaded or still loading.
	CARBON_AUDIO_HOST_API std::vector<std::wstring> GetLoadedSoundBanks();
	CARBON_AUDIO_HOST_API SoundBankStatus GetSoundBankStatus( const std::wstring& name );
	/// Merges static data into what CarbonAudio knows, e.g. after generating SoundBanks again; known names are replaced.
	CARBON_AUDIO_HOST_API void UpdateStaticData( const StaticData& staticData );
	/// Disables and enables CarbonAudio with these SoundBanks, e.g. after generating them again. Emitters, rooms, the
	/// listener and the obstruction query stay and loops resume; one-shots stop and geometry has to be sent again.
	CARBON_AUDIO_HOST_API bool Restart( const std::vector<std::wstring>& soundBanks );

	CARBON_AUDIO_HOST_API bool SetGlobalRTPC( const std::wstring& rtpcName, float value );
	CARBON_AUDIO_HOST_API bool SetState( const std::wstring& stateGroup, const std::wstring& stateName );
	/// Stops every sound on every game object, the listener and the UI and music players included.
	CARBON_AUDIO_HOST_API void StopAll();

	struct PlayingEvent
	{
		uint32_t playingID = 0;
		std::wstring eventName;
	};

	/// Stops every instance of the event on the emitter; false when none was playing.
	CARBON_AUDIO_HOST_API bool StopEvent( IRoot* emitter, const std::wstring& eventName, uint32_t fadeOutMs = 1000 );
	CARBON_AUDIO_HOST_API bool StopSound( IRoot* emitter, uint32_t playingID, uint32_t fadeOutMs = 1000 );
	CARBON_AUDIO_HOST_API bool SeekOnEventPercent( IRoot* emitter, uint32_t playingID, float percent );
	CARBON_AUDIO_HOST_API bool SeekOnEventMs( IRoot* emitter, uint32_t playingID, uint32_t ms );
	CARBON_AUDIO_HOST_API std::vector<PlayingEvent> GetPlayingEvents( IRoot* emitter );
	/// Switch group and state pairs set on the emitter.
	CARBON_AUDIO_HOST_API std::vector<std::pair<std::wstring, std::wstring>> GetSwitches( IRoot* emitter );

	/// Wwise Spatial Audio geometry, sent by AudGeometry (ITr2AudGeometry).
	CARBON_AUDIO_HOST_API void SetGeometryEnabled( bool enabled );
	CARBON_AUDIO_HOST_API bool GetGeometryEnabled();
	/// Whether Wwise Spatial Audio has started; after that the first eleven settings no longer apply.
	CARBON_AUDIO_HOST_API bool IsSpatialAudioInitialized();
	CARBON_AUDIO_HOST_API void GetSpatialAudioSettings( SpatialAudioSettings& out );
	/// Stores every value; false when Wwise Spatial Audio has started and one of the first eleven changed.
	CARBON_AUDIO_HOST_API bool SetSpatialAudioSettings( const SpatialAudioSettings& settings );

	/// The last sightline result for one emitter.
	struct SightlineResult
	{
		uint64_t gameObjectID = 0;
		bool blocked = false;
	};

	CARBON_AUDIO_HOST_API void SetObstructionOcclusionEnabled( bool enabled );
	CARBON_AUDIO_HOST_API bool GetObstructionOcclusionEnabled();
	/// Units per second; 0 is instant.
	CARBON_AUDIO_HOST_API void SetObstructionOcclusionFadeRate( float rate );
	CARBON_AUDIO_HOST_API float GetObstructionOcclusionFadeRate();
	/// query implements IEveObstructionQuery and is held weakly, so the host keeps it alive; null turns sightlines off.
	CARBON_AUDIO_HOST_API bool SetObstructionQuery( IRoot* query );
	/// The current query, or null.
	CARBON_AUDIO_HOST_API IRoot* GetObstructionQuery();
	/// Blockage from 0 (clear) to 1 (blocked), as the game reports it without a query.
	CARBON_AUDIO_HOST_API bool SetEmitterLineOfSightBlockage( uint64_t gameObjectID, float blockage );
	CARBON_AUDIO_HOST_API float GetEmitterOcclusion( uint64_t gameObjectID );
	CARBON_AUDIO_HOST_API std::vector<SightlineResult> GetLastSightlineResults();
	CARBON_AUDIO_HOST_API void ClearObstructionOcclusion();

	/// Switches the main output to the spatial audio (3D audio) device, asynchronously.
	CARBON_AUDIO_HOST_API bool EnableSpatialAudio();
	/// Switches the main output to the stereo device, asynchronously.
	CARBON_AUDIO_HOST_API bool DisableSpatialAudio();
	/// Whether the spatial audio device is the main output.
	CARBON_AUDIO_HOST_API bool GetSpatialAudioEnabled();
	/// Whether the output endpoint supports spatial audio.
	CARBON_AUDIO_HOST_API bool SpatialAudioIsSupported();

	/// The UI player, created on first use; query it for ITr2AudEmitter to send events. Null before Start.
	CARBON_AUDIO_HOST_API IRoot* GetUIPlayer();
	/// The music player, created on first use; query it for ITr2AudEmitter to send events. Null before Start.
	CARBON_AUDIO_HOST_API IRoot* GetMusicPlayer();
	/// Posts a dialogue event on the UI player, whose play position can then be read; 0 when it did not play.
	CARBON_AUDIO_HOST_API uint32_t PostDialogueEvent( const std::wstring& eventName );
	/// Milliseconds played of a dialogue event; -1 when it is not playing.
	CARBON_AUDIO_HOST_API int32_t GetEventPlayPosition( uint32_t playingID );

	/// What the Wwise Profiler sees of this process.
	struct CommSettings
	{
		std::string appNetworkName;
		uint16_t discoveryBroadcastPort = 0;
		/// The port in use, also when Wwise picked it.
		uint16_t commandPort = 0;
	};

	/// Wwise's resource monitor summary for the last audio frame.
	struct ResourceMonitorSummary
	{
		float totalCPU = 0.0f;
		float pluginCPU = 0.0f;
		uint32_t physicalVoices = 0;
		uint32_t virtualVoices = 0;
		uint32_t totalVoices = 0;
		uint32_t activeEvents = 0;
	};

	/// The Wwise SDK version, e.g. "2025.1.5.9095".
	CARBON_AUDIO_HOST_API std::string GetWwiseVersion();
	/// Whether Wwise communication started, so the Wwise Profiler can connect; never in Release builds.
	CARBON_AUDIO_HOST_API bool GetWwiseCommunicationEnabled();
	CARBON_AUDIO_HOST_API bool GetCommSettings( CommSettings& out );
	/// Restarts Wwise communication with its current settings, which drops a connected Wwise Profiler.
	CARBON_AUDIO_HOST_API bool ResetComm();
	/// Writes WwiseCapture_<date>_<time>.prof to the working directory; false in Release builds.
	CARBON_AUDIO_HOST_API bool StartProfilerCapture();
	CARBON_AUDIO_HOST_API bool StopProfilerCapture();
	CARBON_AUDIO_HOST_API bool IsProfilerCapturing();
	/// Records the output to a WAV file, relative to the working directory.
	CARBON_AUDIO_HOST_API bool StartOutputCapture( const std::wstring& fileName );
	CARBON_AUDIO_HOST_API bool StopOutputCapture();
	CARBON_AUDIO_HOST_API bool IsOutputCapturing();
	/// Adds a marker at the current position of the output capture.
	CARBON_AUDIO_HOST_API bool AddOutputCaptureMarker( const std::string& text );
	/// False before the first audio frame and in Release builds.
	CARBON_AUDIO_HOST_API bool GetResourceMonitorSummary( ResourceMonitorSummary& out );

	/// Debug renderers show every emitter rather than only the selected ones.
	CARBON_AUDIO_HOST_API void SetDebugDisplayAllEmitters( bool enabled );
	CARBON_AUDIO_HOST_API bool GetDebugDisplayAllEmitters();
}
