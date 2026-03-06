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
	AkDiffractionPathInfo paths[4];
	AkUInt32 numPaths = 4;
	AkVector64 listenerPos, emitterPos;

	AKRESULT result = AK::SpatialAudio::QueryDiffractionPaths(
		emitterID, 0, listenerPos, emitterPos, paths, numPaths );

	state.targetObstruction = 0.0f;
	state.targetOcclusion = 0.0f;

	if( result == AK_Success && numPaths > 0 )
	{
		float bestDiffraction = 1.0f;
		float bestReportedDiffraction = 1.0f;
		float strongestTransmission = 0.0f;
		bool hasDiffractionPath = false;
		bool hasReportedDiffraction = false;
		bool hasBlockedDirectPath = false;

		for( AkUInt32 i = 0; i < numPaths; ++i )
		{
			float diffraction = std::max( 0.0f, std::min( 1.0f, paths[i].diffraction ) );
			float transmission = std::max( 0.0f, std::min( 1.0f, paths[i].transmissionLoss ) );
			strongestTransmission = std::max( strongestTransmission, transmission );
			if( diffraction > 0.0f )
			{
				hasReportedDiffraction = true;
				bestReportedDiffraction = std::min( bestReportedDiffraction, diffraction );
			}

			// A path with at least one node represents diffraction around geometry edges.
			// This is the gradual value the demo exposes while moving behind/away from a wall.
			if( paths[i].nodeCount > 0 )
			{
				hasDiffractionPath = true;
				bestDiffraction = std::min( bestDiffraction, diffraction );
			}
			else if( transmission > 0.0f )
			{
				// No edge nodes and non-zero transmission indicates a blocked direct path.
				hasBlockedDirectPath = true;
			}
		}

		if( hasDiffractionPath )
			state.targetObstruction = bestDiffraction;
		else if( hasReportedDiffraction )
			state.targetObstruction = bestReportedDiffraction;
		else
			state.targetObstruction = hasBlockedDirectPath ? 1.0f : 0.0f;

		state.targetOcclusion = strongestTransmission;
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
