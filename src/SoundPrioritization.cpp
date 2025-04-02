#include "stdafx.h"
#include "SoundPrioritization.h"
#include "IPrioritizedObject.h"
#include "tbb/parallel_for.h"

#ifndef AK_OPTIMIZED
CCP_STATS_DECLARE(numActiveAudioEmmiters, "CarbonAudio/AudManager/NumActiveAudioEmitters", false, CST_COUNTER_LOW, " Number of active audio emitters");
#endif


SoundPrioritization::SoundPrioritization() :
	m_audioCullingEnabled( true ), m_listener( nullptr ), m_objectsMutex( "SoundPrioritization", "m_objectsMutex" )
{
	// Initialize default values
	m_settings.maxAwakeGameObjects = 75;
	m_settings.oneShotWindow = 50;
	m_settings.weightMultiplier = 10000000.0f;
	m_settings.playingVitalSoundWeight = std::numeric_limits<float>::max();
	m_settings.playing2DWeight = 999.0f;
	m_settings.rangeWeight = 400.0f;
	m_settings.activeSoundsWeight = 200.0f;
	m_settings.waitingOneShotWeight = 100.0f;
	m_settings.visibleWeight = 100.0f;
	m_settings.usedEmitterWeight = 50.0f;

	// Store the defaults
	m_defaultSettings = m_settings;
}

SoundPrioritization::~SoundPrioritization()
{
}

void SoundPrioritization::RegisterGameObject( IPrioritizedObject* object )
{
	if( !object )
		return;

	CcpAutoMutex mutex( m_objectsMutex );
	m_gameObjects.push_back( object );

	if( object->GetID() == LISTENER_GAME_OBJ_ID )
	{
		m_listener = object;
	}
}

void SoundPrioritization::UnregisterGameObject( AkGameObjectID objectID )
{
	CcpAutoMutex mutex( m_objectsMutex );

	if( m_listener && m_listener->GetID() == objectID )
	{
		m_listener = nullptr;
	}

	m_gameObjects.erase(
		std::remove_if(
			m_gameObjects.begin(),
			m_gameObjects.end(),
			[objectID]( IPrioritizedObject* obj ) { return obj->GetID() == objectID; } ),
		m_gameObjects.end() );
}

float SoundPrioritization::CalculateObjectWeight(
	float distanceSq,
	bool isMuted,
	bool isInRange,
	bool isUsed,
	bool isVisible,
	bool isPlaying2D,
	bool isPlayingVital,
	float additionalWeight,
	size_t activeEventCount,
	float waitingOneShotWeight,
	float usedEmitterWeight,
	float rangeWeight,
	float activeSoundsWeight,
	float visibleWeight,
	float playing2DWeight,
	float playingVitalSoundWeight )
{
	if( isMuted )
	{
		return std::numeric_limits<float>::max() - additionalWeight;
	}
	else
	{
		float usedEmitterWeightValue = isUsed ? usedEmitterWeight : 0.0f;
		float rangeWeightValue = isInRange ? rangeWeight : 0.0f;
		float activeSoundsWeightValue = activeEventCount > 0 ? activeSoundsWeight : 0.0f;
		float visibleWeightValue = isVisible ? visibleWeight : 0.0f;
		float playing2DWeightValue = isPlaying2D ? playing2DWeight : 0.0f;
		float playingVitalSoundWeightValue = isPlayingVital ? playingVitalSoundWeight : 0.0f;

		return ( distanceSq - activeSoundsWeightValue - rangeWeightValue - visibleWeightValue -
				 usedEmitterWeightValue - waitingOneShotWeight - playing2DWeightValue -
				 playingVitalSoundWeightValue ) -
			additionalWeight;
	}
}

void SoundPrioritization::CullAudio()
{
	if( !m_audioCullingEnabled || m_gameObjects.empty() || !m_listener )
	{
		return;
	}

	CcpAutoMutex mutex( m_objectsMutex );

	CCP_STATS_ZONE( __FUNCTION__ );

	Vector3 listenerPosition = m_listener->GetPosition();
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	// Step 1: Calculate distance from the listener for all game objects and update their culling weight
	{
		CCP_STATS_ZONE( "CullAudio_CalculateCullingWeight" );
		tbb::parallel_for(
			tbb::blocked_range<size_t>( 0, m_gameObjects.size() ),
			[&]( const tbb::blocked_range<size_t>& range ) -> void {
				for( size_t index = range.begin(); index != range.end(); ++index )
				{
					IPrioritizedObject* gameObject = m_gameObjects[index];
					if( gameObject != m_listener )
					{
						float distanceSq = LengthSq( gameObject->GetPosition() - listenerPosition );
						gameObject->SetDistanceSqFromListener( distanceSq );
					}
					gameObject->CalculateCullingWeight( now );
				}
			} );
	}

	// Step 2: Sort all game objects by their culling weight
	{
		CCP_STATS_ZONE( "CullAudio_SortGameObjects" );
		std::sort( m_gameObjects.begin(), m_gameObjects.end(), []( IPrioritizedObject* a, IPrioritizedObject* b ) -> bool {
			return a->GetCullingWeight() < b->GetCullingWeight();
		} );
	}

	// Step 3: Keep the first m_maxAwakeGameObjects game objects awake
	{
		CCP_STATS_ZONE( "CullAudio_WakeAndCullGameObjects" );
		int numAwake = 0;
		for( auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it )
		{
			if( numAwake > m_settings.maxAwakeGameObjects )
			{
				if( !( *it )->IsCulled() )
				{
					( *it )->Cull();
				}
			}
			else
			{
				if( ( *it )->IsCulled() )
				{
					( *it )->Wake();
				}
				++numAwake;
			}
#ifndef AK_OPTIMIZED
			CCP_STATS_SET( numActiveAudioEmmiters, numAwake );
#endif
		}
	}
}

void SoundPrioritization::EnableAudioCulling()
{
	m_audioCullingEnabled = true;
	CCP_LOG( "The sound prioritization system has been enabled." );
}

void SoundPrioritization::DisableAudioCulling()
{
	// Wake up all objects when disabling
	CcpAutoMutex mutex( m_objectsMutex );
	for( auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it )
	{
		if( ( *it )->IsCulled() )
		{
			( *it )->Wake();
		}
	}

	m_audioCullingEnabled = false;
	CCP_LOG( "The sound prioritization system has been disabled." );
}

bool SoundPrioritization::GetAudioCullingEnabled() const
{
	return m_audioCullingEnabled;
}

void SoundPrioritization::ResetCullingSettings()
{
	m_settings = m_defaultSettings;
}

// Settings getters and setters

int SoundPrioritization::GetMaxAwakeGameObjects() const
{
	return m_settings.maxAwakeGameObjects;
}

void SoundPrioritization::SetMaxAwakeGameObjects( int value )
{
	m_settings.maxAwakeGameObjects = value;
}

long long SoundPrioritization::GetOneShotWindow() const
{
	return m_settings.oneShotWindow;
}

void SoundPrioritization::SetOneShotWindow( long long numMilliseconds )
{
	m_settings.oneShotWindow = numMilliseconds;
}

float SoundPrioritization::GetPlaying2DWeight() const
{
	return m_settings.weightMultiplier * m_settings.playing2DWeight;
}

void SoundPrioritization::SetPlaying2DWeight( float weight )
{
	m_settings.playing2DWeight = weight;
}

float SoundPrioritization::GetPlayingEventsWeight() const
{
	return m_settings.weightMultiplier * m_settings.activeSoundsWeight;
}

void SoundPrioritization::SetPlayingEventsWeight( float weight )
{
	m_settings.activeSoundsWeight = weight;
}

float SoundPrioritization::GetPlayingVitalSoundWeight() const
{
	return m_settings.weightMultiplier * m_settings.playingVitalSoundWeight;
}

void SoundPrioritization::SetPlayingVitalSoundWeight( float weight )
{
	m_settings.playingVitalSoundWeight = weight;
}

float SoundPrioritization::GetRangeWeight() const
{
	return m_settings.weightMultiplier * m_settings.rangeWeight;
}

void SoundPrioritization::SetRangeWeight( float weight )
{
	m_settings.rangeWeight = weight;
}

float SoundPrioritization::GetUsedEmitterWeight() const
{
	return m_settings.weightMultiplier * m_settings.usedEmitterWeight;
}

void SoundPrioritization::SetUsedEmitterWeight( float weight )
{
	m_settings.usedEmitterWeight = weight;
}

float SoundPrioritization::GetVisibleWeight() const
{
	return m_settings.weightMultiplier * m_settings.visibleWeight;
}

void SoundPrioritization::SetVisibleWeight( float weight )
{
	m_settings.visibleWeight = weight;
}

float SoundPrioritization::GetWaitingOneShotWeight() const
{
	return m_settings.weightMultiplier * m_settings.waitingOneShotWeight;
}

void SoundPrioritization::SetWaitingOneShotWeight( float weight )
{
	m_settings.waitingOneShotWeight = weight;
}

float SoundPrioritization::GetWeightMultiplier() const
{
	return m_settings.weightMultiplier;
}

void SoundPrioritization::SetWeightMultiplier( float value )
{
	m_settings.weightMultiplier = value;
}

const std::vector<IPrioritizedObject*>& SoundPrioritization::GetPrioritizedAudioObjects() const
{
	return m_gameObjects;
}