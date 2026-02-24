#include "stdafx.h"
#include "AudObstruction.h"
#include "Audio2.h"
#include <cmath>

AudObstruction::AudObstruction()
	: m_lastUpdateTime( std::chrono::steady_clock::now() )
{
}

void AudObstruction::Update(
	AkGameObjectID listenerID,
	const std::vector<IPrioritizedObject*>& gameObjects )
{
	if( !g_audioInitialized )
		return;

	// Delta time
	auto now = std::chrono::steady_clock::now();
	float dt = std::chrono::duration<float>( now - m_lastUpdateTime ).count();
	m_lastUpdateTime = now;
	dt = std::min( dt, 0.1f ); // Clamp to avoid huge jumps after pauses

	// Exponential smoothing factor: 1 - e^(-rate * dt)
	float alpha = 1.0f - std::exp( -kSmoothingRate * dt );

	for( IPrioritizedObject* obj : gameObjects )
	{
		AkGameObjectID emitterID = obj->GetID();

		if( emitterID == listenerID )
			continue;

		if( obj->IsCulled() )
		{
			m_smoothed.erase( emitterID );
			continue;
		}

		// Query the diffraction/transmission paths that Wwise already computed.
		AkDiffractionPathInfo paths[8];
		AkUInt32 numPaths = 8;
		AkVector64 listenerPos, emitterPos;

		AKRESULT result = AK::SpatialAudio::QueryDiffractionPaths(
			emitterID, 0, listenerPos, emitterPos, paths, numPaths );

		float targetObstruction = 0.0f;
		float targetOcclusion = 0.0f;

		if( result == AK_Success && numPaths > 0 )
		{
			// Find the best (least-obstructed) values across all paths.
			float bestDiffraction = 1.0f;
			float bestTransmission = 1.0f;

			for( AkUInt32 i = 0; i < numPaths; ++i )
			{
				if( paths[i].diffraction < bestDiffraction )
					bestDiffraction = paths[i].diffraction;
				if( paths[i].transmissionLoss < bestTransmission )
					bestTransmission = paths[i].transmissionLoss;
			}

			targetObstruction = bestDiffraction;
			targetOcclusion = bestTransmission;
		}

		// Smooth
		auto it = m_smoothed.find( emitterID );
		if( it == m_smoothed.end() )
		{
			// First frame for this emitter -- snap to target (no pop on spawn)
			m_smoothed[emitterID] = { targetObstruction, targetOcclusion };
		}
		else
		{
			it->second.obstruction += ( targetObstruction - it->second.obstruction ) * alpha;
			it->second.occlusion += ( targetOcclusion - it->second.occlusion ) * alpha;
		}

		SmoothedValues& sv = m_smoothed[emitterID];
		float obstruction = std::max( 0.0f, std::min( 1.0f, sv.obstruction ) );
		float occlusion = std::max( 0.0f, std::min( 1.0f, sv.occlusion ) );

		AK::SoundEngine::SetObjectObstructionAndOcclusion(
			emitterID, listenerID, obstruction, occlusion );
	}
}
