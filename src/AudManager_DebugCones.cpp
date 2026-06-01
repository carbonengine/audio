#include "stdafx.h"
#include "AudManager.h"

#ifndef AK_OPTIMIZED
#include "WAAPI/WaapiManager.h"
#endif

#include <cmath>
#ifndef AK_OPTIMIZED
#include <thread>
#endif
#include <unordered_set>

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "AudioManager" );
#ifndef AK_OPTIMIZED
static const std::chrono::milliseconds DEBUG_CONE_CACHE_REFRESH_INTERVAL( 500 );
static const std::chrono::milliseconds DEBUG_CONE_CACHE_STALE_INTERVAL( 2000 );
static const std::chrono::milliseconds DEBUG_WAAPI_RECONNECT_INTERVAL( 5000 );
static const std::chrono::milliseconds DEBUG_WAAPI_REQUEST_STALL_INTERVAL( 2000 );
static const std::chrono::milliseconds DEBUG_WAAPI_RECONNECT_STALL_INTERVAL( 5000 );

static long long GetDebugSteadyClockMillis()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now().time_since_epoch() ).count();
}
#endif

#ifndef AK_OPTIMIZED
static std::string NarrowDebugEventName( const std::wstring& eventName )
{
	std::string result;
	result.reserve( eventName.size() );
	for( wchar_t character : eventName )
	{
		result.push_back( static_cast<char>( character ) );
	}
	return result;
}

static DebugAttenuationConeRequest BuildDebugAttenuationConeRequest( const std::map<unsigned int, std::wstring>& playingEvents )
{
	DebugAttenuationConeRequest request;
	for( const auto& playingEvent : playingEvents )
	{
		const std::string eventName = NarrowDebugEventName( playingEvent.second );
		if( eventName.empty() )
		{
			continue;
		}

		request.events.push_back( DebugAttenuationConeEvent{ static_cast<AkPlayingID>( playingEvent.first ), eventName } );
		if( !request.cacheKey.empty() )
		{
			request.cacheKey += "\n";
		}
		request.cacheKey += std::to_string( playingEvent.first );
		request.cacheKey += ":";
		request.cacheKey += eventName;
	}
	return request;
}

static bool DebugConeDataChanged( const DebugAttenuationConeData& lhs, const DebugAttenuationConeData& rhs )
{
	constexpr float epsilon = 0.01f;
	return lhs.coneEnabled != rhs.coneEnabled ||
		   std::fabs( lhs.innerAngle - rhs.innerAngle ) > epsilon ||
		   std::fabs( lhs.outerAngle - rhs.outerAngle ) > epsilon ||
		   std::fabs( lhs.coneAttenuation - rhs.coneAttenuation ) > epsilon ||
		   std::fabs( lhs.maxRadius - rhs.maxRadius ) > epsilon ||
		   lhs.attenuationId != rhs.attenuationId ||
		   lhs.sourceObjectId != rhs.sourceObjectId;
}

static bool DebugConeDataListChanged( const std::vector<DebugAttenuationConeData>& lhs, const std::vector<DebugAttenuationConeData>& rhs )
{
	if( lhs.size() != rhs.size() )
	{
		return true;
	}

	for( size_t index = 0; index < lhs.size(); ++index )
	{
		if( DebugConeDataChanged( lhs[index], rhs[index] ) )
		{
			return true;
		}
	}

	return false;
}

static std::string GetDebugConeDataDedupKey( const DebugAttenuationConeData& data )
{
	if( !data.sourceObjectId.empty() )
	{
		return data.sourceObjectId;
	}
	if( !data.attenuationId.empty() )
	{
		return data.attenuationId;
	}
	return data.eventName;
}

static bool ApplyWaapiConeData( const WaapiAttenuationConeData& waapiData, DebugAttenuationConeData& data )
{
	data.coneEnabled = waapiData.coneEnabled;
	data.innerAngle = static_cast<float>( waapiData.innerAngle );
	data.outerAngle = static_cast<float>( waapiData.outerAngle );
	data.coneAttenuation = static_cast<float>( waapiData.coneAttenuation );
	data.maxRadius = static_cast<float>( waapiData.maxRadius );
	data.attenuationId = waapiData.attenuationId;
	data.attenuationName = waapiData.attenuationName;
	data.sourceObjectId = waapiData.sourceObjectId;
	data.sourceObjectName = waapiData.sourceObjectName;
	data.sourceObjectPath = waapiData.sourceObjectPath;
	return data.coneEnabled && data.innerAngle > 0.0f && data.maxRadius > 0.0f;
}
#endif

void AudManager::EnableDebugDisplayAllEmitters()
{
	g_debugDisplayAllEmitters = true;
#ifndef AK_OPTIMIZED
	m_nextDebugWaapiReconnectTime = std::chrono::steady_clock::now();
#endif
}

void AudManager::DisableDebugDisplayAllEmitters()
{
	g_debugDisplayAllEmitters = false;
#ifndef AK_OPTIMIZED
	ClearDebugAttenuationConeData();
#endif
}

bool AudManager::GetDebugDisplayAllEmitters()
{
	return g_debugDisplayAllEmitters;
}

void AudManager::SetDebugEmitterVisualizationEnabled( AkGameObjectID emitterID, bool enabled )
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	if( enabled )
	{
		m_hiddenDebugEmitterVisualizations.erase( emitterID );
	}
	else
	{
		m_hiddenDebugEmitterVisualizations.insert( emitterID );
	}
}

bool AudManager::GetDebugEmitterVisualizationEnabled( AkGameObjectID emitterID ) const
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	return m_hiddenDebugEmitterVisualizations.find( emitterID ) == m_hiddenDebugEmitterVisualizations.end();
}

void AudManager::SoloDebugEmitterVisualization( AkGameObjectID emitterID )
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	m_soloDebugEmitterVisualization = emitterID;
}

void AudManager::ClearDebugEmitterVisualizationSolo()
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	m_soloDebugEmitterVisualization = AK_INVALID_GAME_OBJECT;
}

AkGameObjectID AudManager::GetDebugEmitterVisualizationSolo() const
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	return m_soloDebugEmitterVisualization;
}

void AudManager::ClearDebugEmitterVisualizationFilters()
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	m_hiddenDebugEmitterVisualizations.clear();
	m_soloDebugEmitterVisualization = AK_INVALID_GAME_OBJECT;
}

bool AudManager::ShouldDebugDisplayEmitter( AkGameObjectID emitterID ) const
{
	CcpAutoMutex lock( m_debugEmitterVisualizationMutex );
	if( m_soloDebugEmitterVisualization != AK_INVALID_GAME_OBJECT )
	{
		return m_soloDebugEmitterVisualization == emitterID;
	}
	return m_hiddenDebugEmitterVisualizations.find( emitterID ) == m_hiddenDebugEmitterVisualizations.end();
}

#ifdef AK_OPTIMIZED
bool AudManager::ConnectWaapiForDebugConeVisualization( const std::string& host, int port )
{
	CCP_LOGWARN_CH( s_ch, "WAAPI connection unavailable for audio debug cones in optimized audio builds. Existing audio emitter debug rendering will continue without cone data." );
	return false;
}

bool AudManager::IsWaapiConnectedForDebugConeVisualization() const
{
	return false;
}
#else
void AudManager::ClearDebugAttenuationConeData()
{
	CcpAutoMutex lock( m_debugConeDataMutex );
	m_debugConeCache.clear();
	m_pendingDebugConeRequests.clear();
	m_debugConeCacheKeys.clear();
	m_debugConeCacheRefreshTimes.clear();
}

void AudManager::ClearDebugAttenuationConeDataForEmitter( AkGameObjectID emitterID )
{
	CcpAutoMutex lock( m_debugConeDataMutex );
	m_debugConeCache.erase( emitterID );
	m_pendingDebugConeRequests.erase( emitterID );
	m_debugConeCacheKeys.erase( emitterID );
	m_debugConeCacheRefreshTimes.erase( emitterID );
}

bool AudManager::ConnectWaapiForDebugConeVisualization( const std::string& host, int port )
{
	m_debugWaapiHost = host;
	m_debugWaapiPort = port;

	{
		CcpAutoMutex lock( m_debugConeDataMutex );
		if( m_debugWaapiManager && m_debugWaapiManager->IsConnected() )
		{
			return true;
		}
	}

	m_nextDebugWaapiReconnectTime = std::chrono::steady_clock::now();
	PollWaapiConnectionForDebugConeVisualization();
	return m_debugWaapiReconnectWorkerRunning.load();
}

void AudManager::PollWaapiConnectionForDebugConeVisualization()
{
	{
		CcpAutoMutex lock( m_debugConeDataMutex );
		if( m_debugWaapiManager && m_debugWaapiManager->IsConnected() )
		{
			return;
		}
	}

	if( m_debugWaapiReconnectWorkerRunning.load() )
	{
		return;
	}

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if( now < m_nextDebugWaapiReconnectTime )
	{
		return;
	}

	m_nextDebugWaapiReconnectTime = now + DEBUG_WAAPI_RECONNECT_INTERVAL;
	m_debugWaapiReconnectWorkerRunning.store( true );
	m_debugWaapiReconnectWorkerStartMs.store( GetDebugSteadyClockMillis() );

	const std::string host = m_debugWaapiHost;
	const int port = m_debugWaapiPort;
	AudManagerPtr selfPtr = this;
	std::thread( [selfPtr, host, port]() {
		WaapiManagerPtr waapiManager = new OWaapiManager;
		const bool connected = waapiManager->TryConnect( host, port );
		if( connected )
		{
			{
				CcpAutoMutex lock( selfPtr->m_debugConeDataMutex );
				selfPtr->m_debugWaapiManager = waapiManager;
			}
			CCP_LOG_CH( s_ch, "WAAPI connection restored for audio debug cones on %s:%d.", host.c_str(), port );
		}
		else
		{
			CCP_LOGWARN_CH( s_ch, "WAAPI connection unavailable for audio debug cones. Existing audio emitter debug rendering will continue without cone data." );
			selfPtr->ClearDebugAttenuationConeData();
		}

		selfPtr->m_debugWaapiReconnectWorkerRunning.store( false );
	} ).detach();
}

bool AudManager::IsWaapiConnectedForDebugConeVisualization() const
{
	if( m_debugWaapiReconnectWorkerRunning.load() )
	{
		const long long requestAgeMs = GetDebugSteadyClockMillis() - m_debugWaapiReconnectWorkerStartMs.load();
		if( requestAgeMs > DEBUG_WAAPI_RECONNECT_STALL_INTERVAL.count() )
		{
			return false;
		}
	}

	WaapiManagerPtr waapiManager;
	{
		CcpAutoMutex lock( m_debugConeDataMutex );
		waapiManager = m_debugWaapiManager;
	}

	if( !waapiManager || !waapiManager->IsConnected() )
	{
		return false;
	}

	if( m_debugConeRequestWorkerRunning.load() )
	{
		const long long requestAgeMs = GetDebugSteadyClockMillis() - m_debugConeRequestWorkerStartMs.load();
		if( requestAgeMs > DEBUG_WAAPI_REQUEST_STALL_INTERVAL.count() )
		{
			return false;
		}
	}

	return true;
}

bool AudManager::GetDebugAttenuationConeData( AkGameObjectID emitterID, DebugAttenuationConeData& outData )
{
	if( !IsWaapiConnectedForDebugConeVisualization() )
	{
		ClearDebugAttenuationConeDataForEmitter( emitterID );
		return false;
	}

	CcpAutoMutex lock( m_debugConeDataMutex );
	auto it = m_debugConeCache.find( emitterID );
	if( it == m_debugConeCache.end() || it->second.empty() )
	{
		return false;
	}

	auto refreshTimeIt = m_debugConeCacheRefreshTimes.find( emitterID );
	if( refreshTimeIt == m_debugConeCacheRefreshTimes.end() ||
		std::chrono::steady_clock::now() - refreshTimeIt->second > DEBUG_CONE_CACHE_STALE_INTERVAL )
	{
		m_debugConeCache.erase( emitterID );
		m_debugConeCacheKeys.erase( emitterID );
		m_debugConeCacheRefreshTimes.erase( emitterID );
		return false;
	}

	outData = it->second.front();
	return true;
}

bool AudManager::GetDebugAttenuationConeDataList( AkGameObjectID emitterID, std::vector<DebugAttenuationConeData>& outData )
{
	if( !IsWaapiConnectedForDebugConeVisualization() )
	{
		ClearDebugAttenuationConeDataForEmitter( emitterID );
		return false;
	}

	CcpAutoMutex lock( m_debugConeDataMutex );
	auto it = m_debugConeCache.find( emitterID );
	if( it == m_debugConeCache.end() || it->second.empty() )
	{
		return false;
	}

	auto refreshTimeIt = m_debugConeCacheRefreshTimes.find( emitterID );
	if( refreshTimeIt == m_debugConeCacheRefreshTimes.end() ||
		std::chrono::steady_clock::now() - refreshTimeIt->second > DEBUG_CONE_CACHE_STALE_INTERVAL )
	{
		m_debugConeCache.erase( emitterID );
		m_debugConeCacheKeys.erase( emitterID );
		m_debugConeCacheRefreshTimes.erase( emitterID );
		return false;
	}

	outData = it->second;
	return true;
}

void AudManager::RequestDebugAttenuationConeData( AkGameObjectID emitterID, const std::map<unsigned int, std::wstring>& playingEvents )
{
	if( !g_debugDisplayAllEmitters || !IsWaapiConnectedForDebugConeVisualization() || playingEvents.empty() )
	{
		ClearDebugAttenuationConeDataForEmitter( emitterID );
		return;
	}

	DebugAttenuationConeRequest request = BuildDebugAttenuationConeRequest( playingEvents );
	if( request.events.empty() )
	{
		return;
	}

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	CcpAutoMutex lock( m_debugConeDataMutex );
	auto cacheKeyIt = m_debugConeCacheKeys.find( emitterID );
	auto refreshTimeIt = m_debugConeCacheRefreshTimes.find( emitterID );
	if( cacheKeyIt != m_debugConeCacheKeys.end() &&
		cacheKeyIt->second == request.cacheKey &&
		refreshTimeIt != m_debugConeCacheRefreshTimes.end() &&
		now - refreshTimeIt->second < DEBUG_CONE_CACHE_REFRESH_INTERVAL )
	{
		return;
	}

	auto pendingIt = m_pendingDebugConeRequests.find( emitterID );
	if( pendingIt != m_pendingDebugConeRequests.end() && pendingIt->second.cacheKey == request.cacheKey )
	{
		return;
	}

	m_pendingDebugConeRequests[emitterID] = request;
}

void AudManager::ProcessDebugAttenuationConeRequests()
{
	WaapiManagerPtr waapiManager;
	{
		CcpAutoMutex lock( m_debugConeDataMutex );
		waapiManager = m_debugWaapiManager;
	}

	if( !waapiManager )
	{
		return;
	}

	if( !waapiManager->IsConnected() )
	{
		ClearDebugAttenuationConeData();
		return;
	}

	if( m_debugConeRequestWorkerRunning.load() )
	{
		return;
	}

	std::unordered_map<AkGameObjectID, DebugAttenuationConeRequest> pendingRequests;
	{
		CcpAutoMutex lock( m_debugConeDataMutex );
		pendingRequests.swap( m_pendingDebugConeRequests );
	}

	if( pendingRequests.empty() )
	{
		return;
	}

	m_debugConeRequestWorkerRunning.store( true );
	m_debugConeRequestWorkerStartMs.store( GetDebugSteadyClockMillis() );

	AudManagerPtr selfPtr = this;
	std::thread( [selfPtr, pendingRequests]() {
		try
		{
			for( const auto& request : pendingRequests )
			{
				std::vector<DebugAttenuationConeData> coneDataList;
				std::unordered_set<std::string> coneDataDedupKeys;
				for( const DebugAttenuationConeEvent& eventData : request.second.events )
				{
					DebugAttenuationConeData data = selfPtr->ResolveDebugAttenuationConeData( request.first, eventData.playingID, eventData.eventName );
					if( data.resolved && data.coneEnabled )
					{
						const std::string dedupKey = GetDebugConeDataDedupKey( data );
						if( dedupKey.empty() || coneDataDedupKeys.insert( dedupKey ).second )
						{
							coneDataList.push_back( data );
						}
					}
				}

				CcpAutoMutex lock( selfPtr->m_debugConeDataMutex );
				auto oldCacheIt = selfPtr->m_debugConeCache.find( request.first );
				const bool changed = oldCacheIt == selfPtr->m_debugConeCache.end() || DebugConeDataListChanged( oldCacheIt->second, coneDataList );
				selfPtr->m_debugConeCache[request.first] = coneDataList;
				selfPtr->m_debugConeCacheKeys[request.first] = request.second.cacheKey;
				selfPtr->m_debugConeCacheRefreshTimes[request.first] = std::chrono::steady_clock::now();
				if( changed && !coneDataList.empty() )
				{
					const DebugAttenuationConeData& firstCone = coneDataList.front();
					CCP_LOG_CH( s_ch,
								"Audio debug cone refreshed: emitter=%llu event=%s source=%s attenuation=%s inner=%.2f outer=%.2f coneAttenuation=%.2f radius=%.2f",
								static_cast<unsigned long long>( request.first ),
								firstCone.eventName.c_str(),
								firstCone.sourceObjectPath.empty() ? firstCone.sourceObjectName.c_str() : firstCone.sourceObjectPath.c_str(),
								firstCone.attenuationName.c_str(),
								firstCone.innerAngle,
								firstCone.outerAngle,
								firstCone.coneAttenuation,
								firstCone.maxRadius );
				}
			}
		}
		catch( const std::exception& e )
		{
			CCP_LOGWARN_CH( s_ch, "Audio debug cone WAAPI worker failed: %s", e.what() );
		}
		catch( ... )
		{
			CCP_LOGWARN_CH( s_ch, "Audio debug cone WAAPI worker failed with an unknown exception." );
		}
		selfPtr->m_debugConeRequestWorkerRunning.store( false );
	} ).detach();
}

DebugAttenuationConeData AudManager::ResolveDebugAttenuationConeData( AkGameObjectID emitterID, AkPlayingID playingID, const std::string& eventName )
{
	DebugAttenuationConeData data;
	data.resolved = true;
	data.eventName = eventName;

	if( !IsWaapiConnectedForDebugConeVisualization() )
	{
		return data;
	}

	WaapiManagerPtr waapiManager;
	{
		CcpAutoMutex lock( m_debugConeDataMutex );
		waapiManager = m_debugWaapiManager;
	}

	if( !waapiManager )
	{
		return data;
	}

	const std::string profilerVoiceObjectId = waapiManager->GetBestProfilerVoiceObjectIdForPlayingEvent( static_cast<uint64_t>( emitterID ), static_cast<uint32_t>( playingID ) );
	if( !profilerVoiceObjectId.empty() )
	{
		WaapiAttenuationConeData waapiData;
		if( waapiManager->GetObjectEffectiveAttenuationConeData( profilerVoiceObjectId, waapiData ) && ApplyWaapiConeData( waapiData, data ) )
		{
			return data;
		}
	}

	const std::vector<std::string> targetIds = waapiManager->GetEventReferencedTargetIds( eventName );
	for( const std::string& targetId : targetIds )
	{
		WaapiAttenuationConeData waapiData;
		if( !waapiManager->GetObjectEffectiveAttenuationConeData( targetId, waapiData ) )
		{
			continue;
		}

		if( ApplyWaapiConeData( waapiData, data ) )
		{
			return data;
		}
	}

	data.coneEnabled = false;
	return data;
}
#endif
