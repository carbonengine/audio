////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "CarbonAudioHost.h"

#include "AudGameObjResource.h"
#include "AudListener.h"
#include "AudManager.h"
#include "AudMusicPlayer.h"
#include "AudPortal.h"
#include "AudRoom.h"
#include "AudRoomManager.h"
#include "AudSettings.h"
#include "AudStaticDataRepository.h"
#include "AudUIPlayer.h"
#include "IEveObstructionQuery.h"

#include <mutex>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "CarbonAudioHost" );

// Reads and writes the AudRoom and AudPortal state CarbonAudioHost needs; both befriend it.
struct CarbonAudioHostAccess
{
	static void Read( const AudRoom& room, CarbonAudioHost::RoomSettings& out )
	{
		out.name = room.m_name;
		out.reverbAuxBus = room.m_reverbAuxBus;
		out.reverbLevel = room.m_reverbLevel;
		out.transmissionLoss = room.m_transmissionLoss;
		out.priority = room.m_priority;
		out.auxSendLevelToSelf = room.m_auxSendLevelToSelf;
		out.keepRegistered = room.m_keepRegistered;
		out.roomToneEvent = room.m_roomToneEvent;
		out.enabled = room.m_enabled;
	}

	static void Write( AudRoom& room, const CarbonAudioHost::RoomSettings& settings )
	{
		room.m_name = settings.name;
		room.m_reverbAuxBus = settings.reverbAuxBus;
		room.m_reverbLevel = settings.reverbLevel;
		room.m_transmissionLoss = settings.transmissionLoss;
		room.m_priority = settings.priority;
		room.m_auxSendLevelToSelf = settings.auxSendLevelToSelf;
		room.m_keepRegistered = settings.keepRegistered;
		room.m_roomToneEvent = settings.roomToneEvent;
		room.m_enabled = settings.enabled;
		// Same as an attribute edit from Python or Graphite.
		room.Sync();
	}

	static void Read( const AudPortal& portal, CarbonAudioHost::PortalSettings& out )
	{
		out.name = portal.m_name;
		out.open = portal.m_open;
		out.adjacentRoomBleed = portal.m_adjacentRoomBleed;
		out.obstruction = portal.m_obstruction;
		out.occlusion = portal.m_occlusion;
		out.obstructionOcclusionTransition = portal.m_obstructionOcclusionTransition;
		out.enabled = portal.m_enabled;
	}

	static void Write( AudPortal& portal, const CarbonAudioHost::PortalSettings& settings )
	{
		portal.m_name = settings.name;
		portal.m_open = settings.open;
		portal.m_adjacentRoomBleed = settings.adjacentRoomBleed;
		portal.m_obstruction = settings.obstruction;
		portal.m_occlusion = settings.occlusion;
		portal.m_obstructionOcclusionTransition = settings.obstructionOcclusionTransition;
		portal.m_enabled = settings.enabled;
	}

	static void Read( const AudPortal& portal, CarbonAudioHost::PortalStatus& out )
	{
		out.portalID = portal.m_portalID;
		out.inWwise = portal.m_sentToWwise;
		out.usableShape = portal.GetBox().IsUsable();
		out.frontRoomID = portal.m_frontRoomID;
		out.backRoomID = portal.m_backRoomID;
		out.obstructionOcclusionSent = portal.m_obstructionOcclusionSent;
		out.sentObstruction = portal.m_sentObstruction;
		out.sentOcclusion = portal.m_sentOcclusion;
	}
};

namespace
{
	Vector3 ToVector3( const CarbonAudioHost::Vec3& v )
	{
		return Vector3( v.x, v.y, v.z );
	}

	/// AudSettings paths are wide on Windows and narrow on macOS.
#if defined( _WIN32 )
	const std::wstring& ToSettingsString( const std::wstring& value )
	{
		return value;
	}
#else
	std::string ToSettingsString( const std::wstring& value )
	{
		return std::string( static_cast<const char*>( CW2A( value.c_str() ) ) );
	}
#endif

	AudRoomPtr AsRoom( IRoot* object )
	{
		AudRoomPtr room = BlueCastPtr( object );
		return room;
	}

	AudPortalPtr AsPortal( IRoot* object )
	{
		AudPortalPtr portal = BlueCastPtr( object );
		return portal;
	}

	/// Wwise positions are left-handed.
	CarbonAudioHost::Vec3 ToRightHanded( const AkVector64& v )
	{
		return { static_cast<float>( v.X ), static_cast<float>( v.Y ), static_cast<float>( -v.Z ) };
	}

	/// The most paths Wwise keeps for one game object (AK::SpatialAudio::SetMaxDiffractionPaths).
	constexpr AkUInt32 MAX_DIFFRACTION_PATHS = 32;

	AudGameObjResourcePtr AsGameObject( IRoot* object )
	{
		AudGameObjResourcePtr gameObject = BlueCastPtr( object );
		return gameObject;
	}

	bool s_outputCapturing = false;

	/// Registers the UI or music player with Wwise while culling leaves it awake. Created outside the culling pass, it
	/// is otherwise only registered by a wake, which never comes with culling off.
	template<typename Player>
	Player* Registered( Player* player )
	{
		if( !player->IsCulled() )
		{
			player->RegisterWwiseObject();
		}
		return player;
	}

#ifndef AK_OPTIMIZED
	/// The host's own resource monitor callback, registered while Wwise is enabled: AudManager's is not registered
	/// again after a Disable, as Restart does.
	std::mutex s_resourceMonitorMutex;
	AkResourceMonitorDataSummary s_resourceMonitorSummary;
	bool s_hasResourceMonitorSummary = false;
	bool s_resourceMonitorRegistered = false;

	void ResourceMonitorCallback( const AkResourceMonitorDataSummary* summary )
	{
		std::lock_guard<std::mutex> lock( s_resourceMonitorMutex );
		s_resourceMonitorSummary = *summary;
		s_hasResourceMonitorSummary = true;
	}
#endif

	/// After Wwise is enabled.
	void RegisterResourceMonitor()
	{
#ifndef AK_OPTIMIZED
		if( s_resourceMonitorRegistered )
		{
			return;
		}
		const AKRESULT result = AK::SoundEngine::RegisterResourceMonitorCallback( ResourceMonitorCallback );
		if( result != AK_Success )
		{
			CCP_LOGERR_CH( s_ch, "Registering the resource monitor callback failed with result %d.", result );
			return;
		}
		s_resourceMonitorRegistered = true;
#endif
	}

	/// Before Wwise is disabled.
	void UnregisterResourceMonitor()
	{
#ifndef AK_OPTIMIZED
		if( s_resourceMonitorRegistered )
		{
			AK::SoundEngine::UnregisterResourceMonitorCallback( ResourceMonitorCallback );
			s_resourceMonitorRegistered = false;
		}
		std::lock_guard<std::mutex> lock( s_resourceMonitorMutex );
		s_hasResourceMonitorSummary = false;
#endif
	}
}

namespace CarbonAudioHost
{
	bool Start( const StaticData& staticData, const Settings& settings, const std::vector<std::wstring>& soundBanks )
	{
		if( BePaths == nullptr )
		{
			CCP_LOGERR_CH( s_ch, "Start: Blue is not booted. Call BlueModuleStartup and BlueInitializePaths first." );
			return false;
		}

		if( !RegisterModule() )
		{
			CCP_LOGERR_CH( s_ch, "Start: the main thread queue could not be created." );
			return false;
		}

		UpdateStaticData( staticData );

		AudManager* manager = GetOrCreateAudioManager();
		if( manager->GetState() == AudioState::Enabled )
		{
			CCP_LOGWARN_CH( s_ch, "Start: CarbonAudio is already running." );
			return true;
		}

		AudSettingsPtr audioSettings;
		if( !audioSettings.CreateInstance() )
		{
			CCP_LOGERR_CH( s_ch, "Start: AudSettings could not be created." );
			return false;
		}
		audioSettings->m_baseSoundBankPath = ToSettingsString( settings.soundBankPath );
		audioSettings->m_soundbankLanguage = ToSettingsString( settings.language );
		audioSettings->m_applicationName = settings.applicationName;
		audioSettings->m_spatialAudioEnabled = settings.spatialAudioEnabled;
		audioSettings->m_spatialAudioDeviceName = ToSettingsString( settings.spatialAudioDeviceName );
		audioSettings->m_stereoAudioDeviceName = ToSettingsString( settings.stereoAudioDeviceName );
		audioSettings->m_essentialPath = ToSettingsString( settings.essentialPath );
		audioSettings->m_audioSrcPath = ToSettingsString( settings.audioSrcPath );
		manager->UpdateSettings( audioSettings );

		SetSpatialAudioSettings( settings.spatialAudio );
		// Stored while audio is off; Enable() starts Spatial Audio for it.
		manager->SetSpatialAudioRoomsEnabled( settings.roomsEnabled );
		manager->SetSpatialAudioGeometryEnabled( settings.geometryEnabled );
		if( !settings.audioCulling )
		{
			// Before any emitter exists, so new emitters start awake.
			manager->DisableAudioCulling();
		}

		manager->Enable( BankVector( soundBanks ) );
		if( manager->GetState() != AudioState::Enabled )
		{
			CCP_LOGERR_CH( s_ch, "Start: Wwise did not start, see the errors above." );
			return false;
		}

		RegisterResourceMonitor();
		GetOrCreateListener();
		return true;
	}

	void Tick()
	{
		if( g_audioManager != nullptr )
		{
			// Process() ignores the times.
			g_audioManager->OnTick( 0, 0, nullptr );
		}
	}

	void Stop()
	{
		if( g_audioManager != nullptr )
		{
			StopOutputCapture();
			if( g_audioManager->IsProfilerCapturing() )
			{
				g_audioManager->StopProfilerCapture();
			}
			UnregisterResourceMonitor();
			g_audioManager->Disable();
		}
	}

	bool IsRunning()
	{
		return g_audioManager != nullptr && g_audioManager->GetState() == AudioState::Enabled;
	}

	void SetListenerPlacement( const Vec3& front, const Vec3& top, const Vec3& position )
	{
		if( g_audioManager == nullptr )
		{
			return;
		}
		if( AudListenerPtr listener = g_audioManager->GetListener() )
		{
			listener->SetPlacementFromParent( ToVector3( front ), ToVector3( top ), ToVector3( position ) );
		}
	}

	uint64_t GetListenerID()
	{
		if( g_audioManager == nullptr )
		{
			return 0;
		}
		AudListenerPtr listener = g_audioManager->GetListener();
		return listener ? listener->GetID() : 0;
	}

	void SetRoomsEnabled( bool enabled )
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->SetSpatialAudioRoomsEnabled( enabled );
		}
	}

	bool GetRoomsEnabled()
	{
		return g_audioManager != nullptr && g_audioManager->GetSpatialAudioRoomsEnabled();
	}

	bool GetRoomSettings( IRoot* room, RoomSettings& out )
	{
		AudRoomPtr audRoom = AsRoom( room );
		if( !audRoom )
		{
			return false;
		}
		CarbonAudioHostAccess::Read( *audRoom.p, out );
		return true;
	}

	bool SetRoomSettings( IRoot* room, const RoomSettings& settings )
	{
		AudRoomPtr audRoom = AsRoom( room );
		if( !audRoom )
		{
			return false;
		}
		CarbonAudioHostAccess::Write( *audRoom.p, settings );
		return true;
	}

	bool GetRoomStatus( IRoot* room, RoomStatus& out )
	{
		AudRoomPtr audRoom = AsRoom( room );
		if( !audRoom )
		{
			return false;
		}
		out.roomID = audRoom->GetRoomID();
		out.inWwise = audRoom->IsRegistered();
		out.usableShape = audRoom->GetBox().IsUsable();
		out.roomTonePosted = false;
		out.roomTonePending = false;
		if( g_audioManager != nullptr )
		{
			g_audioManager->GetRoomManager().GetRoomToneState( *audRoom.p, out.roomTonePosted, out.roomTonePending );
		}
		return true;
	}

	bool GetPortalSettings( IRoot* portal, PortalSettings& out )
	{
		AudPortalPtr audPortal = AsPortal( portal );
		if( !audPortal )
		{
			return false;
		}
		CarbonAudioHostAccess::Read( *audPortal.p, out );
		return true;
	}

	bool SetPortalSettings( IRoot* portal, const PortalSettings& settings )
	{
		AudPortalPtr audPortal = AsPortal( portal );
		if( !audPortal )
		{
			return false;
		}
		CarbonAudioHostAccess::Write( *audPortal.p, settings );
		return true;
	}

	bool GetPortalStatus( IRoot* portal, PortalStatus& out )
	{
		AudPortalPtr audPortal = AsPortal( portal );
		if( !audPortal )
		{
			return false;
		}
		CarbonAudioHostAccess::Read( *audPortal.p, out );
		return true;
	}

	std::vector<DiffractionPath> GetDiffractionPaths( uint64_t gameObjectID )
	{
		std::vector<DiffractionPath> paths;
		if( g_audioManager == nullptr || !g_audioManager->IsSpatialAudioInitialized() )
		{
			return paths;
		}

		std::vector<AkDiffractionPathInfo> infos( MAX_DIFFRACTION_PATHS );
		AkUInt32 count = MAX_DIFFRACTION_PATHS;
		AkVector64 listenerPosition;
		AkVector64 emitterPosition;
		if( AK::SpatialAudio::QueryDiffractionPaths( gameObjectID, 0, listenerPosition, emitterPosition, infos.data(), count ) != AK_Success )
		{
			return paths;
		}

		for( AkUInt32 i = 0; i < count; ++i )
		{
			const AkDiffractionPathInfo& info = infos[i];
			DiffractionPath path;
			for( AkUInt32 node = 0; node < info.nodeCount && node < AkDiffractionPathInfo::kMaxNodes; ++node )
			{
				path.nodes.push_back( ToRightHanded( info.nodes[node] ) );
				path.portals.push_back( info.portals[node].IsValid() ? info.portals[node].id : 0 );
			}
			path.emitter = ToRightHanded( info.emitterPos );
			path.diffraction = info.diffraction;
			path.transmissionLoss = info.transmissionLoss;
			path.length = info.totLength;
			paths.push_back( std::move( path ) );
		}
		return paths;
	}

	uint64_t GetGameObjectID( IRoot* emitter )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		return gameObject ? gameObject->GetID() : 0;
	}

	bool StopAll( IRoot* emitter )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		if( !gameObject )
		{
			return false;
		}
		gameObject->StopAll();
		return true;
	}

	bool GetAssignedRoom( uint64_t gameObjectID, uint64_t& roomID )
	{
		if( g_audioManager == nullptr )
		{
			return false;
		}
		AkUInt64 assigned = 0;
		if( !g_audioManager->GetRoomManager().GetAssignedRoom( gameObjectID, assigned ) )
		{
			return false;
		}
		roomID = assigned;
		return true;
	}

	void LoadBank( const std::wstring& name )
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->LoadBank( name );
		}
	}

	void UnloadBank( const std::wstring& name )
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->UnloadBank( name );
		}
	}

	void ClearBanks()
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->ClearBanks();
		}
	}

	std::vector<std::wstring> GetLoadedSoundBanks()
	{
		if( g_audioManager == nullptr )
		{
			return {};
		}
		return g_audioManager->GetLoadedSoundBanks();
	}

	SoundBankStatus GetSoundBankStatus( const std::wstring& name )
	{
		if( g_audioManager == nullptr )
		{
			return SoundBankStatus::NotLoaded;
		}
		switch( g_audioManager->GetSoundBankStatus( name ) )
		{
		case ::SoundBankStatus::LOADING:
			return SoundBankStatus::Loading;
		case ::SoundBankStatus::LOADED:
			return SoundBankStatus::Loaded;
		case ::SoundBankStatus::UNLOADING:
			return SoundBankStatus::Unloading;
		default:
			return SoundBankStatus::NotLoaded;
		}
	}

	void UpdateStaticData( const StaticData& staticData )
	{
		std::vector<AudStaticDataRepository::EventData> events;
		events.reserve( staticData.events.size() );
		for( const EventInfo& event : staticData.events )
		{
			events.push_back( {
				event.name,
				event.eventID,
				event.maxAttenuationRadius,
				event.isLoop,
				event.is2D,
				event.isVital,
				event.stoppedBy,
				event.soundBanks } );
		}
		GetOrCreateStaticDataRepository()->InitializeFromData( events, staticData.soundBanks, staticData.sources );
	}

	bool Restart( const std::vector<std::wstring>& soundBanks )
	{
		if( g_audioManager == nullptr || g_audioManager->GetState() == AudioState::Uninitialized )
		{
			CCP_LOGERR_CH( s_ch, "Restart: CarbonAudio has not been started." );
			return false;
		}

		IEveObstructionQueryPtr query = g_audioManager->GetObstructionQuery();
		UnregisterResourceMonitor();
		g_audioManager->Disable();
		g_audioManager->Enable( BankVector( soundBanks ) );
		if( g_audioManager->GetState() != AudioState::Enabled )
		{
			CCP_LOGERR_CH( s_ch, "Restart: Wwise did not start again, see the errors above." );
			return false;
		}

		RegisterResourceMonitor();
		GetOrCreateListener();
		g_audioManager->SetObstructionQuery( query );
		return true;
	}

	bool SetGlobalRTPC( const std::wstring& rtpcName, float value )
	{
		return g_audioManager != nullptr && g_audioManager->SetGlobalRTPC( rtpcName, value );
	}

	bool SetState( const std::wstring& stateGroup, const std::wstring& stateName )
	{
		return g_audioManager != nullptr && g_audioManager->SetState( stateGroup, stateName );
	}

	void StopAll()
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->StopAll();
		}
	}

	bool StopEvent( IRoot* emitter, const std::wstring& eventName, uint32_t fadeOutMs )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		return gameObject && gameObject->StopEvent( eventName, fadeOutMs );
	}

	bool StopSound( IRoot* emitter, uint32_t playingID, uint32_t fadeOutMs )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		if( !gameObject )
		{
			return false;
		}
		gameObject->StopSound( playingID, fadeOutMs );
		return true;
	}

	bool SeekOnEventPercent( IRoot* emitter, uint32_t playingID, float percent )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		return gameObject && gameObject->SeekOnEventPercent( playingID, percent );
	}

	bool SeekOnEventMs( IRoot* emitter, uint32_t playingID, uint32_t ms )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		return gameObject && gameObject->SeekOnEventMs( playingID, ms );
	}

	std::vector<PlayingEvent> GetPlayingEvents( IRoot* emitter )
	{
		std::vector<PlayingEvent> events;
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		if( gameObject )
		{
			for( const auto& playing : gameObject->GetPlayingEvents() )
			{
				events.push_back( { playing.first, playing.second } );
			}
		}
		return events;
	}

	std::vector<std::pair<std::wstring, std::wstring>> GetSwitches( IRoot* emitter )
	{
		AudGameObjResourcePtr gameObject = AsGameObject( emitter );
		if( !gameObject )
		{
			return {};
		}
		const auto& switches = gameObject->GetSwitches();
		return std::vector<std::pair<std::wstring, std::wstring>>( switches.begin(), switches.end() );
	}

	void SetGeometryEnabled( bool enabled )
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->SetSpatialAudioGeometryEnabled( enabled );
		}
	}

	bool GetGeometryEnabled()
	{
		return g_audioManager != nullptr && g_audioManager->GetSpatialAudioGeometryEnabled();
	}

	bool IsSpatialAudioInitialized()
	{
		return g_audioManager != nullptr && g_audioManager->IsSpatialAudioInitialized();
	}

	void GetSpatialAudioSettings( SpatialAudioSettings& out )
	{
		if( g_audioManager == nullptr )
		{
			out = SpatialAudioSettings();
			return;
		}
		out.movementThreshold = g_audioManager->GetMovementThreshold();
		out.numberOfPrimaryRays = g_audioManager->GetNumberOfPrimaryRays();
		out.maxReflectionOrder = g_audioManager->GetMaxReflectionOrder();
		out.maxDiffractionOrder = g_audioManager->GetMaxDiffractionOrder();
		out.maxEmitterRoomAuxSends = g_audioManager->GetMaxEmitterRoomAuxSends();
		out.diffractionOnReflectionsOrder = g_audioManager->GetDiffractionOnReflectionsOrder();
		out.maxPathLength = g_audioManager->GetMaxPathLength();
		out.cpuLimitPercentage = g_audioManager->GetCPULimitPercentage();
		out.loadBalancingSpread = g_audioManager->GetLoadBalancingSpread();
		out.enableDiffractionAndTransmission = g_audioManager->GetEnableDiffractionAndTransmission();
		out.calcEmitterVirtualPosition = g_audioManager->GetCalcEmitterVirtualPosition();
		out.transmissionLoss = g_audioManager->GetTransmissionLoss();
		out.enableDiffraction = g_audioManager->GetEnableDiffraction();
		out.enableDiffractionOnBoundaryEdges = g_audioManager->GetEnableDiffractionOnBoundaryEdges();
	}

	bool SetSpatialAudioSettings( const SpatialAudioSettings& settings )
	{
		if( g_audioManager == nullptr )
		{
			return false;
		}
		SpatialAudioSettings current;
		GetSpatialAudioSettings( current );
		const bool initSettingsChanged = current.movementThreshold != settings.movementThreshold
			|| current.numberOfPrimaryRays != settings.numberOfPrimaryRays
			|| current.maxReflectionOrder != settings.maxReflectionOrder
			|| current.maxDiffractionOrder != settings.maxDiffractionOrder
			|| current.maxEmitterRoomAuxSends != settings.maxEmitterRoomAuxSends
			|| current.diffractionOnReflectionsOrder != settings.diffractionOnReflectionsOrder
			|| current.maxPathLength != settings.maxPathLength
			|| current.cpuLimitPercentage != settings.cpuLimitPercentage
			|| current.loadBalancingSpread != settings.loadBalancingSpread
			|| current.enableDiffractionAndTransmission != settings.enableDiffractionAndTransmission
			|| current.calcEmitterVirtualPosition != settings.calcEmitterVirtualPosition;

		g_audioManager->SetMovementThreshold( settings.movementThreshold );
		g_audioManager->SetNumberOfPrimaryRays( settings.numberOfPrimaryRays );
		g_audioManager->SetMaxReflectionOrder( settings.maxReflectionOrder );
		g_audioManager->SetMaxDiffractionOrder( settings.maxDiffractionOrder );
		g_audioManager->SetMaxEmitterRoomAuxSends( settings.maxEmitterRoomAuxSends );
		g_audioManager->SetDiffractionOnReflectionsOrder( settings.diffractionOnReflectionsOrder );
		g_audioManager->SetMaxPathLength( settings.maxPathLength );
		g_audioManager->SetCPULimitPercentage( settings.cpuLimitPercentage );
		g_audioManager->SetLoadBalancingSpread( settings.loadBalancingSpread );
		g_audioManager->SetEnableDiffractionAndTransmission( settings.enableDiffractionAndTransmission );
		g_audioManager->SetCalcEmitterVirtualPosition( settings.calcEmitterVirtualPosition );
		g_audioManager->SetTransmissionLoss( settings.transmissionLoss );
		g_audioManager->SetEnableDiffraction( settings.enableDiffraction );
		g_audioManager->SetEnableDiffractionOnBoundaryEdges( settings.enableDiffractionOnBoundaryEdges );

		if( initSettingsChanged && g_audioManager->IsSpatialAudioInitialized() )
		{
			CCP_LOGWARN_CH( s_ch, "SetSpatialAudioSettings: Wwise Spatial Audio has started, the init settings apply after a restart of the process." );
			return false;
		}
		return true;
	}

	void SetObstructionOcclusionEnabled( bool enabled )
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->SetObstructionOcclusionEnabled( enabled );
		}
	}

	bool GetObstructionOcclusionEnabled()
	{
		return g_audioManager != nullptr && g_audioManager->GetObstructionOcclusionEnabled();
	}

	void SetObstructionOcclusionFadeRate( float rate )
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->SetObstructionOcclusionFadeRate( rate );
		}
	}

	float GetObstructionOcclusionFadeRate()
	{
		return g_audioManager != nullptr ? g_audioManager->GetObstructionOcclusionFadeRate() : 0.0f;
	}

	bool SetObstructionQuery( IRoot* query )
	{
		if( g_audioManager == nullptr )
		{
			return false;
		}
		if( query == nullptr )
		{
			g_audioManager->SetObstructionQuery( nullptr );
			return true;
		}
		IEveObstructionQueryPtr obstructionQuery = BlueCastPtr( query );
		if( !obstructionQuery )
		{
			return false;
		}
		g_audioManager->SetObstructionQuery( obstructionQuery );
		return true;
	}

	IRoot* GetObstructionQuery()
	{
		if( g_audioManager == nullptr )
		{
			return nullptr;
		}
		IEveObstructionQuery* query = g_audioManager->GetObstructionQuery();
		return query != nullptr ? query->GetRootObject() : nullptr;
	}

	bool SetEmitterLineOfSightBlockage( uint64_t gameObjectID, float blockage )
	{
		return g_audioManager != nullptr && g_audioManager->SetEmitterLineOfSightBlockage( gameObjectID, blockage );
	}

	float GetEmitterOcclusion( uint64_t gameObjectID )
	{
		return g_audioManager != nullptr ? g_audioManager->GetEmitterOcclusion( gameObjectID ) : 0.0f;
	}

	std::vector<SightlineResult> GetLastSightlineResults()
	{
		std::vector<SightlineResult> results;
		if( g_audioManager != nullptr )
		{
			for( const auto& result : g_audioManager->GetLastSightlineResults() )
			{
				results.push_back( { result.first, result.second } );
			}
		}
		return results;
	}

	void ClearObstructionOcclusion()
	{
		if( g_audioManager != nullptr )
		{
			g_audioManager->ClearObstructionOcclusion();
		}
	}

	bool EnableSpatialAudio()
	{
		return g_audioManager != nullptr && g_audioManager->EnableSpatialAudio();
	}

	bool DisableSpatialAudio()
	{
		return g_audioManager != nullptr && g_audioManager->DisableSpatialAudio();
	}

	bool GetSpatialAudioEnabled()
	{
		return g_audioManager != nullptr && g_audioManager->GetSpatialAudioEnabled();
	}

	bool SpatialAudioIsSupported()
	{
		return g_audioManager != nullptr && g_audioManager->SpatialAudioIsSupported();
	}

	IRoot* GetUIPlayer()
	{
		if( g_audioManager == nullptr )
		{
			return nullptr;
		}
		return Registered( GetOrCreateUIPlayer() )->GetRawRoot();
	}

	IRoot* GetMusicPlayer()
	{
		if( g_audioManager == nullptr )
		{
			return nullptr;
		}
		return Registered( GetOrCreateMusicPlayer() )->GetRawRoot();
	}

	uint32_t PostDialogueEvent( const std::wstring& eventName )
	{
		if( g_audioManager == nullptr )
		{
			return 0;
		}
		return Registered( GetOrCreateUIPlayer() )->PostDialogueEvent( eventName );
	}

	int32_t GetEventPlayPosition( uint32_t playingID )
	{
		if( g_audioManager == nullptr )
		{
			return -1;
		}
		return GetOrCreateUIPlayer()->GetEventPlayPosition( playingID );
	}

	std::string GetWwiseVersion()
	{
		return g_wwiseVersion;
	}

	bool GetWwiseCommunicationEnabled()
	{
		return g_wwiseCommunicationEnabled;
	}

	bool GetCommSettings( CommSettings& out )
	{
#ifndef AK_OPTIMIZED
		if( !g_wwiseCommunicationEnabled )
		{
			return false;
		}
		const AkCommSettings& settings = AK::Comm::GetCurrentSettings();
		out.appNetworkName = settings.szAppNetworkName;
		out.discoveryBroadcastPort = settings.ports.uDiscoveryBroadcast;
		out.commandPort = AK::Comm::GetCommandPort();
		return true;
#else
		return false;
#endif
	}

	bool ResetComm()
	{
#ifndef AK_OPTIMIZED
		return g_wwiseCommunicationEnabled && AK::Comm::Reset() == AK_Success;
#else
		return false;
#endif
	}

	bool StartProfilerCapture()
	{
		return g_audioManager != nullptr && g_audioManager->StartProfilerCapture() == AK_Success;
	}

	bool StopProfilerCapture()
	{
		return g_audioManager != nullptr && g_audioManager->StopProfilerCapture() == AK_Success;
	}

	bool IsProfilerCapturing()
	{
		return g_audioManager != nullptr && g_audioManager->IsProfilerCapturing();
	}

	bool StartOutputCapture( const std::wstring& fileName )
	{
		if( g_audioManager == nullptr || g_audioManager->GetState() != AudioState::Enabled || s_outputCapturing )
		{
			return false;
		}
#if defined( _WIN32 )
		const AKRESULT result = AK::SoundEngine::StartOutputCapture( fileName.c_str() );
#else
		const AKRESULT result = AK::SoundEngine::StartOutputCapture( ToSettingsString( fileName ).c_str() );
#endif
		if( result != AK_Success )
		{
			CCP_LOGERR_CH( s_ch, "StartOutputCapture failed with result %d.", result );
			return false;
		}
		s_outputCapturing = true;
		return true;
	}

	bool StopOutputCapture()
	{
		if( !s_outputCapturing )
		{
			return false;
		}
		s_outputCapturing = false;
		return AK::SoundEngine::StopOutputCapture() == AK_Success;
	}

	bool IsOutputCapturing()
	{
		return s_outputCapturing;
	}

	bool AddOutputCaptureMarker( const std::string& text )
	{
		return s_outputCapturing && AK::SoundEngine::AddOutputCaptureMarker( text.c_str() ) == AK_Success;
	}

	bool GetResourceMonitorSummary( ResourceMonitorSummary& out )
	{
#ifndef AK_OPTIMIZED
		std::lock_guard<std::mutex> lock( s_resourceMonitorMutex );
		if( !s_hasResourceMonitorSummary )
		{
			return false;
		}
		const AkResourceMonitorDataSummary& summary = s_resourceMonitorSummary;
		out.totalCPU = summary.totalCPU;
		out.pluginCPU = summary.pluginCPU;
		out.physicalVoices = summary.physicalVoices;
		out.virtualVoices = summary.virtualVoices;
		out.totalVoices = summary.totalVoices;
		out.activeEvents = summary.nbActiveEvents;
		return true;
#else
		return false;
#endif
	}

	void SetDebugDisplayAllEmitters( bool enabled )
	{
		if( g_audioManager == nullptr )
		{
			return;
		}
		if( enabled )
		{
			g_audioManager->EnableDebugDisplayAllEmitters();
		}
		else
		{
			g_audioManager->DisableDebugDisplayAllEmitters();
		}
	}

	bool GetDebugDisplayAllEmitters()
	{
		return g_audioManager != nullptr && g_audioManager->GetDebugDisplayAllEmitters();
	}
}
