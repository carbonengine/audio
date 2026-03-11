#include "stdafx.h"
#include "AudObstructionOcclusion.h"
#include "Audio2.h"
#include <cstdlib>
#include <unordered_set>

AudObstructionOcclusion::AudObstructionOcclusion()
	: m_lastUpdateTime( std::chrono::steady_clock::now() )
{
}

void AudObstructionOcclusion::Update(
	AkGameObjectID listenerID,
	const std::vector<IPrioritizedObject*>& gameObjects )
{
	if( !g_audioInitialized )
		return;

	auto now = std::chrono::steady_clock::now();
	float dt = std::chrono::duration<float>( now - m_lastUpdateTime ).count();

	m_lastUpdateTime = now;
	dt = std::min( dt, 0.1f );
	m_time += dt;

	for( const IPrioritizedObject* obj : gameObjects )
	{
		AkGameObjectID emitterID = obj->GetID();

		if( emitterID == listenerID )
			continue;

		if( obj->IsCulled() )
		{
			m_emitters.erase( emitterID );
			continue;
		}

		EmitterState& state = m_emitters[emitterID];

		if( !state.initialized )
		{
			QueryEmitter( emitterID, state );
			state.obstruction = state.targetObstruction;
			state.occlusion = state.targetOcclusion;

			// Stagger future queries so emitters don't all query on the same frame.
			state.nextQueryTime = m_time + m_refreshInterval * ( (float)std::rand() / RAND_MAX );
			state.initialized = true;
		}
		else
		{
			if( m_time >= state.nextQueryTime )
			{
				QueryEmitter( emitterID, state );
				state.nextQueryTime = m_time + m_refreshInterval;
			}

			float fadeStep = m_fadeRate * dt;

			if( state.obstruction < state.targetObstruction )
				state.obstruction = std::min( state.obstruction + fadeStep, state.targetObstruction );
			else if( state.obstruction > state.targetObstruction )
				state.obstruction = std::max( state.obstruction - fadeStep, state.targetObstruction );

			if( state.occlusion < state.targetOcclusion )
				state.occlusion = std::min( state.occlusion + fadeStep, state.targetOcclusion );
			else if( state.occlusion > state.targetOcclusion )
				state.occlusion = std::max( state.occlusion - fadeStep, state.targetOcclusion );
		}

		float obstruction = std::max( 0.0f, std::min( 1.0f, state.obstruction ) );
		float occlusion = std::max( 0.0f, std::min( 1.0f, state.occlusion ) );

		AK::SoundEngine::SetObjectObstructionAndOcclusion(
			emitterID, listenerID, obstruction, occlusion );
	}

	if( ++m_cleanupCounter >= kCleanupEveryNFrames )
	{
		m_cleanupCounter = 0;
		CleanupStaleEmitters( gameObjects );
	}
}

void AudObstructionOcclusion::QueryEmitter( AkGameObjectID emitterID, EmitterState& state )
{
	// Query a single direct path (1 ray) to check line-of-sight.
	// With uMaxDiffractionOrder = 0 and EnableDiffraction = false,
	// Wwise only returns the direct path with transmission loss if blocked.
	AkDiffractionPathInfo path;
	AkUInt32 numPaths = 1;
	AkVector64 listenerPos, emitterPos;

	AKRESULT result = AK::SpatialAudio::QueryDiffractionPaths(
		emitterID, 0, listenerPos, emitterPos, &path, numPaths );

	state.targetObstruction = 0.0f;
	state.targetOcclusion = 0.0f;

	if( result == AK_Success && numPaths > 0 )
	{
		state.targetOcclusion = std::max( 0.0f, std::min( 1.0f, path.transmissionLoss ) );
	}
}

void AudObstructionOcclusion::CleanupStaleEmitters(
	const std::vector<IPrioritizedObject*>& gameObjects )
{
	std::unordered_set<AkGameObjectID> activeIDs;
	activeIDs.reserve( gameObjects.size() );
	for( IPrioritizedObject* obj : gameObjects )
		activeIDs.insert( obj->GetID() );

	auto it = m_emitters.begin();
	while( it != m_emitters.end() )
	{
		if( activeIDs.find( it->first ) == activeIDs.end() )
			it = m_emitters.erase( it );
		else
			++it;
	}
}

void AudObstructionOcclusion::SetRefreshInterval( float seconds )
{
	m_refreshInterval = std::max( 0.0f, seconds );
}

float AudObstructionOcclusion::GetRefreshInterval() const
{
	return m_refreshInterval;
}

void AudObstructionOcclusion::SetFadeRate( float unitsPerSecond )
{
	m_fadeRate = std::max( 0.0f, unitsPerSecond );
}

float AudObstructionOcclusion::GetFadeRate() const
{
	return m_fadeRate;
}
