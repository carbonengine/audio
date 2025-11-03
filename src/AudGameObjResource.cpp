#include "stdafx.h"
#include "AudGameObjResource.h"

#include "AudManager.h"
#include "AudStaticDataRepository.h"
#include "Vector3.h"
#include "Utilities.h"
#include "SoundPrioritization.h"

//-----------------------------------------------------------------------------
// Helper function to distribute ID's
//-----------------------------------------------------------------------------
static AkGameObjectID GenerateEntityID()
{
	static AkGameObjectID s_currentID = START_GAME_OBJ_COUNT;
	return s_currentID++;
}

AudGameObjResource::AudGameObjResource( IRoot* lockobj ) : PARENTLOCK( m_parameters ),
														 m_eventPrefix(L""),
														 m_scalingFactor( 1.0 ),
														 m_position( WWISE_INIT_POSITION ), 
														 m_mutex( "AudGameObjResource", "m_mutex" ),
														 m_gameObjRegistered( false ),
														 m_culled( true ),
														 m_isVisible(false),
														 m_listenerInRange( false ),
														 m_isUsed( false ),
														 m_playing2DSound( false ),
														 m_playingVitalSound( false ),
														 m_forceCullingState( false ),
														 m_muted(false),
														 m_distanceSqFromListener( 0.0f ),
														 m_additionalCullingWeight( 0.0f ),
														 m_cumulativeWeight( 0.0f ),
														 m_maxAttenuationRadiusSq( 0.0f ),
														 m_hasReceivedPosition(false),
														 m_waitingOneShotInRange( std::pair( std::chrono::steady_clock::now(), L"" ) ),
														 m_eventName(L"")
{
	m_ID = GenerateEntityID();

	if (g_audioManager != nullptr)
	{
		if( !g_audioManager->GetAudioCullingEnabled() )
		{
			m_culled = false;	
		}
		g_audioManager->RegisterGameObject( m_ID, this );
	}

	m_parameters.SetNotify( this );
}

AudGameObjResource::AudGameObjResource( AkGameObjectID gameObjID, IRoot* lockobj ) : PARENTLOCK( m_parameters ),
																				   m_eventPrefix(L""),
																				   m_scalingFactor( 1.0 ),
																				   m_position( WWISE_INIT_POSITION ), 
														 						   m_mutex( "AudGameObjResource", "m_mutex" ),
																				   m_gameObjRegistered( false ),
														 						   m_culled( true ),
														 						   m_isVisible(false),
																				   m_listenerInRange( false ),
																				   m_isUsed( false ),
																				   m_playing2DSound( false ),
																				   m_playingVitalSound( false ),
																				   m_forceCullingState( false ),
																				   m_muted( false ),
																				   m_distanceSqFromListener( 0.0f ),
														 						   m_additionalCullingWeight( 0.0f ),
																				   m_cumulativeWeight( 0.0f ),
																				   m_maxAttenuationRadiusSq( 0.0f ),
																				   m_waitingOneShotInRange( std::pair( std::chrono::steady_clock::now(), L"" ) ),
																				   m_eventName(L"")
{
	m_ID = gameObjID;

	if (g_audioManager != nullptr)
	{
		if( !g_audioManager->GetAudioCullingEnabled() )
		{
			m_culled = false;	
		}
		g_audioManager->RegisterGameObject( m_ID, this );
	}

	m_parameters.SetNotify( this );
}

AudGameObjResource::~AudGameObjResource()
{
	if ( g_audioManager != nullptr )
	{
		g_audioManager->UnregisterGameObject( m_ID );
	}

	if( g_audioInitialized )
    {
		// Make sure end of event callbacks do not happen after this object is destroyed.
		AK::SoundEngine::CancelEventCallbackGameObject( m_ID );
		StopAll();
		UnregisterWwiseObject();
	}
}

void AudGameObjResource::RegisterWwiseObject()
{
	if( g_audioInitialized && m_gameObjRegistered == false )
	{
		AKRESULT result = AK::SoundEngine::RegisterGameObj( m_ID, m_name.c_str() );
		if( result != AK_Success )
		{
			CCP_LOGERR( "Failed to register game object %d with Wwise result code %d", m_ID, result );
		}
		m_gameObjRegistered = true;
	}
}

void AudGameObjResource::UnregisterWwiseObject()
{
	if( g_audioInitialized && m_gameObjRegistered == true )
	{
		AKRESULT result = AK::SoundEngine::UnregisterGameObj( m_ID );
		if( result != AK_Success )
		{
			CCP_LOGERR("Audio emitter %d was unable to be unregistered from Wwise with Wwise result code %d", m_ID, result);
			return;
		}
		m_gameObjRegistered = false;
	}
}

//-----------------------------------------------------
// Description:
//   Logic around sending events to Wwise. 
//   If this game object is not culled then an event will be sent to Wwise and registered in m_playingEvents. 
//   If the game object is culled then one of two things happen: 
//    * If the event is a loop it will be added to a set of events waiting to be played when this game object wakes up. 
//    * If the event is a one shot then it will be registered along with a timestamp and that event will be played if 
//      this game object wakes up within a configurable number of milliseconds.
// Arguments:
//   eventName - The name of the event to send to the audio engine. 
//   bypassPrefix - If false then the event name will be prefixed with an event prefix if it is set on this game object.
//					Defaults to false.
//	 additionalFlags - Additional bitmask flags that aren't Ak_EndOfEvent that you wish to pass to the Wwise PostEvent function. 
//					   See the following Wwise documentation for more info on other flags available for use:
//					   https://www.audiokinetic.com/library/edge/?source=SDK&id=_ak_callback_8h_a948c083ff18dc4c8dfe1d32cb0eb6732.html
// Return value:
//   A Wwise playing ID if the event was successfully sent to the audio engine. Otherwise 0.
//-----------------------------------------------------
unsigned int AudGameObjResource::PostEvent( const std::wstring& eventName, bool bypassPrefix, AkUInt32 additionalFlags )
{

	if( eventName.empty() )
	{
		CCP_LOG( "An empty event string was requested to be sent to Wwise. This request has been ignored." );
		return AK_INVALID_PLAYING_ID;
	}

	// This function relies heavily on static data and cannot function without it
	if (g_staticDataRepository == nullptr)
	{
		return AK_INVALID_PLAYING_ID;
	}

	CcpAutoMutex mutex( m_mutex );
	m_isUsed = true;
	bool eventUsed = false;
	std::wstring fullEventName = PrepareEvent( eventName, bypassPrefix );

	bool eventIsVital = g_staticDataRepository->EventIsVital( fullEventName );
	AkPlayingID playingID = AK_INVALID_PLAYING_ID;
	if ( m_culled || !g_audioEnabled )
	{
		for ( auto it = m_eventsOnWake.cbegin(); it != m_eventsOnWake.cend();)
		{
			if( g_staticDataRepository->EventIsStopped( *it, fullEventName ) )
			{
				it = m_eventsOnWake.erase( it );
				UpdateEventSoundPrioritizationAttributes();
			}
			else
			{
				++it;	
			}
		}

		if ( g_staticDataRepository->EventIsLoop( fullEventName ) || eventIsVital )
		{
			m_eventsOnWake.insert( fullEventName );
			eventUsed = true;
		}
		else
		{
			m_waitingOneShotInRange = std::pair( std::chrono::steady_clock::now(), fullEventName );
			eventUsed = true;
		}
	}
	else if ( m_gameObjRegistered )
	{
		bool soundbanksLoaded = true;

		const std::vector<std::wstring>& eventSoundBanks = g_staticDataRepository->SoundBanksRequiredForEvent( fullEventName );
		if (eventSoundBanks.empty())
		{
			CCP_LOGERR( "Wwise event %S failed to send because it does not exist in any SoundBanks.", fullEventName.c_str() );
			return AK_INVALID_PLAYING_ID;
		}

		for( auto it = eventSoundBanks.begin(); it != eventSoundBanks.end(); ++it )
		{	
			SoundBankStatus soundBankStatus = g_audioManager->GetSoundBankStatus( *it );
			if( soundBankStatus != SoundBankStatus::LOADED )
			{
				soundbanksLoaded = false;
				if( soundBankStatus == SoundBankStatus::LOADING )
				{
					std::wstring soundBank = *it;
					g_audioManager->RegisterEventAfterSoundBankLoad(soundBank, fullEventName, this);
				}
				else
				{
					CCP_LOGERR( "Wwise event %S will not be sent to Wwise because SoundBank %S is currently not loaded.", fullEventName.c_str(), it->c_str() );
					return AK_INVALID_PLAYING_ID;
				}
				break;
			}
		}

		if( soundbanksLoaded )
		{
			// Set up callback info so that we get a callback when every event is finished playing.
			AkUInt32 inFlags = AK_EndOfEvent | additionalFlags;
			AkCallbackFunc callback = &AudGameObjResource::PropagateWwiseCallback;
			unsigned int eventID = g_staticDataRepository->GetEventID( fullEventName );
			playingID = AK::SoundEngine::PostEvent( eventID, m_ID, inFlags, callback, this );
			g_audioManager->LogPostEvent( m_ID, playingID, eventID, fullEventName );

			if ( playingID != AK_INVALID_PLAYING_ID )
			{
				m_playingEvents.insert({playingID, fullEventName});
				eventUsed = true;
			}
			else
			{
				CCP_LOGERR( "Wwise event %S failed to play even though its SoundBanks are loaded.", fullEventName.c_str() );
			}
		}
	}

	if( eventUsed )
	{
		UpdateMaxAttenuationRadiusForEvent( fullEventName );
		if( g_staticDataRepository->EventIs2D( fullEventName ) )
		{
			m_playing2DSound = true;
		}

		if( eventIsVital )
		{
			m_playingVitalSound = true;
		}
	}
	return playingID;
}

//-----------------------------------------------------
// Description:
//   A callback function called by Wwise 
// Arguments:
//   in_eType - The type of callback that Wwise is calling this for.
//              See https://www.audiokinetic.com/library/edge/?source=SDK&id=_ak_callback_8h_a948c083ff18dc4c8dfe1d32cb0eb6732.html
//              for all possible callback types, keep in mind we don't ask for all of these to happen.
//   in_pCallbackInfo - A pointer to the callback information. 
//-----------------------------------------------------
void AudGameObjResource::PropagateWwiseCallback( AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo )
{
	if ( in_eType == AK_EndOfEvent )
	{
		AkEventCallbackInfo* cbInfo = reinterpret_cast<AkEventCallbackInfo*>(in_pCallbackInfo);
		AudGameObjResource* audGameObjResourcePtr = reinterpret_cast<AudGameObjResource*>(cbInfo->pCookie);
		audGameObjResourcePtr->EventFinishedCallback( cbInfo );
	}
	return;
}


//-----------------------------------------------------
// Description:
//   The callback that this class uses when an event finishes playing. Updates our
//   internal map of which events are currently playing on this game object.
// Arguments:
//   cbInfo - Callback information from Wwise, see https://www.audiokinetic.com/library/edge/?source=SDK&id=struct_ak_event_callback_info.html 
//-----------------------------------------------------
void AudGameObjResource::EventFinishedCallback( AkEventCallbackInfo* cbInfo )
{
	CcpAutoMutex mutex( m_mutex );
	m_playingEvents.erase( cbInfo->playingID );
	UpdateEventSoundPrioritizationAttributes();
}

bool AudGameObjResource::StopEvent( const std::wstring& eventName, uint32_t fadeOutDuration )
{
	bool stopped = false;
	std::wstring fullEventName = PrepareEvent( eventName, false );

	for ( auto it = begin( m_playingEvents ); it != end( m_playingEvents ); ++it)
	{
		if ( it->second == fullEventName )
		{
			StopSound(it->first, fadeOutDuration );
			stopped = true;
		}
	}
	return stopped;
}

void AudGameObjResource::StopSound( AkPlayingID playingID, uint32_t fadeOutDuration )
{
	if ( g_audioInitialized )
	{
		ExecuteActionOnPlayingID( playingID, ActionTypes::Stop, fadeOutDuration );
	}
}

void AudGameObjResource::BreakSound( AkPlayingID playingID, uint32_t fadeOutDuration )
{
	if ( g_audioInitialized )
	{
		ExecuteActionOnPlayingID( playingID, ActionTypes::Break, fadeOutDuration );
	}
}

void AudGameObjResource::StopAll()
{
	if( g_audioInitialized )
	{
		CcpAutoMutex mutex( m_mutex );
		for ( auto it = begin( m_playingEvents ); it != end( m_playingEvents ); ++it)
		{
			StopSound( it->first );
		}
	}
}

//-----------------------------------------------------
// Description:
//   Update the degree to which the attenuation of sounds playing on this game object are scaled by. 
//   This will also update the scaling of this game objects max attenuation radius.
//-----------------------------------------------------
bool AudGameObjResource::SetAttenuationScalingFactor( float value )
{
	if( g_audioInitialized )
	{
		if( m_gameObjRegistered )
		{
			AKRESULT result = AK::SoundEngine::SetScalingFactor( m_ID, value );
			if (result != AK_Success)
			{
				CCP_LOGERR( "Failed to set attenuation scaling factor for game object %d. " 
							"Received akresult %d", m_ID, result );
				return false;
			}
			m_scalingFactor = value;

			return true;
		}
	}
	return false;
}

int AudGameObjResource::SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position )
{
	m_position = position;

	if( g_audioInitialized )
	{
		if( m_gameObjRegistered )
		{
			AkSoundPosition tmp;
			Vector3 correctFront = Normalize( front );
			Vector3 correctUp = Normalize( top );
			correctUp = Normalize( Cross( Cross( correctFront, correctUp ), correctFront ) );
			tmp.Set( MakeAkVector(position), MakeAkVector(correctFront), MakeAkVector(correctUp) );

			// all vectors come in RH, but WWISE is LH, so convert
			AkSoundPosition soundPosLH;
			RH2LH::convertEmitter( &soundPosLH, &tmp);

			AKRESULT result = AK::SoundEngine::SetPosition( m_ID, soundPosLH );
		}
	}
	return AK_Success;
}

bool AudGameObjResource::Initialize()
{
	RegisterWwiseObject();
	SetPositionHelper( Vector3( 1,0,0 ), Vector3( 0,1,0 ), m_position );

	if ( !m_eventName.empty() ) 
	{
		PostEvent( m_eventName );
	}

	return true;
}

void AudGameObjResource::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if ( ( event & BELIST_LOADING ) == 0 )
	{
		if( ( IList* )&m_parameters == theList )
		{
			if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
			{
				AudParameterPtr param = dynamic_cast<AudParameter*>( value );
				param->m_ID = m_ID;
			}
		}
	}
}

bool AudGameObjResource::OnModified( Be::Var* value )
{
	if ( IsMatch( value, m_eventName ) )
	{
		StopAll();
		if ( !m_eventName.empty() )
		{
			PostEvent( m_eventName );
		}
	}
	return true;
}

void AudGameObjResource::Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position )
{
	m_name = name;
	m_eventPrefix = prefix;
	m_position = position;
	Initialize();
}

bool AudGameObjResource::SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState )
{
	if ( g_audioInitialized )
	{
		// Store switches sent to it can be used when waking up this game object.
		m_switchValues[switchGroup] = switchState;

		if( m_gameObjRegistered )
		{
			AKRESULT result = AK::SoundEngine::SetSwitch( switchGroup.c_str(), switchState.c_str(), m_ID );
			if(result != AK_Success)
			{
				CCP_LOGERR( "Failed to set switch %S %S for game object %d. " 
							"Received Wwise error code %d", switchGroup.c_str(), switchState.c_str(), m_ID, result );
				return false;
			}
			g_audioManager->LogSetSwitch( m_ID, switchGroup, switchState );
			return true;
		}
		else if( !m_culled )
		{
			CCP_LOGERR( "SetSwitch was requested on game object %d which was never registered with Wwise.", m_ID );
		}
	}
	return false;
}

bool AudGameObjResource::SetRTPC( const std::wstring& rtpcName, float rtpcValue )
{
	if ( g_audioInitialized )
	{
		// Store RTPCs that have been sent so it can be used when waking up this game object.
		m_rtpcValues[rtpcName] = rtpcValue;

		if( m_gameObjRegistered )
		{
			AKRESULT result = AK::SoundEngine::SetRTPCValue( rtpcName.c_str(), AkRtpcValue(rtpcValue), m_ID );
			if( result != AK_Success )
			{
				CCP_LOGERR( "Failed to set RTPC %S to %f for game object %d. " 
							"Received Wwise result code %d", rtpcName.c_str(), rtpcValue, m_ID, result );
				return false;
			}
			g_audioManager->LogSetRTPC( m_ID, rtpcName, rtpcValue );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------
// Description:
//   Seek on an event by using a percentage of its duration. 
// Arguments:
//   playingID - The playing ID of the event to seek. Returned from PostEvent().
//   percentToSeek - The desired position, in percentage, where you want playback of this event to restart.
//    				 Expressed in a percentage of the audio file's total duration between 0 and 1.
//-----------------------------------------------------
bool AudGameObjResource::SeekOnEventPercent( const unsigned int playingID, float percentToSeek )
{
	if ( g_audioInitialized )
	{
		CcpAutoMutex mutex( m_mutex );
		auto it = m_playingEvents.find( playingID );
		if ( it != m_playingEvents.end() )
		{
			AKRESULT result = AK::SoundEngine::SeekOnEvent( it->second.c_str(), m_ID, percentToSeek, false, it->first);
			if ( result != AK_Success )
			{
				CCP_LOGERR( "Failed to seek on event %S. Received akresult %d.", it->second.c_str(), result );
				return false;
			}
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------
// Description:
//   Seek on an event using milliseconds. 
// Arguments:
//   playingID - The playing ID of the event to seek. Returned from PostEvent().
//   msToSeek - Desired position where playback should restart, in milliseconds.
//-----------------------------------------------------
bool AudGameObjResource::SeekOnEventMs( const unsigned int playingID, const unsigned int msToSeek)
{
	if ( g_audioInitialized )
	{
		CcpAutoMutex mutex( m_mutex );
		auto it = m_playingEvents.find( playingID );
		if ( it != m_playingEvents.end() )
		{
			AKRESULT result = AK::SoundEngine::SeekOnEvent( it->second.c_str(), m_ID, AkTimeMs(msToSeek), false, it->first);
			if ( result != AK_Success )
			{
				CCP_LOGERR( "Failed to seek on event %S. Received aksresult %d", it->second.c_str(), result );
				return false;
			}
			return true;
		}
	}
	return false;
}


// Formats events to be sent to Wwise. bypassPrefix determines whether or not to apply the prefix
// defined on the emitter to the event.
std::wstring AudGameObjResource::PrepareEvent( const std::wstring& event, bool bypassPrefix )
{
	std::wstring eventName = StringUtils::trim( event );
	if ( m_eventPrefix != L"" && bypassPrefix == false )
	{
		eventName = std::wstring( m_eventPrefix ) + std::wstring( eventName );
	}

	return eventName; 
}

//-----------------------------------------------------
// Description:
//   Handle logic for when a game object is woken up from being culled and register this object with Wwise.
//   When a game object is woken up it will play any loops that are waiting to be played as well 
//   as sending a one shot event if it got requested within the last 10 milliseconds.
//-----------------------------------------------------
void AudGameObjResource::Wake()
{
	if( g_audioEnabled )
	{
		if ( m_forceCullingState || m_muted )
		{
			return;	
		}

		if(!m_hasReceivedPosition) 
		{
			return;
		}

		RegisterWwiseObject();	
		SetPositionHelper( Vector3( 1,0,0 ), Vector3( 0,1,0 ), m_position );
		m_culled = false;
		if ( m_waitingOneShotInRange.second != L"" && m_listenerInRange )
		{
			PostEvent( m_waitingOneShotInRange.second, true );
			m_waitingOneShotInRange = std::pair( std::chrono::steady_clock::now(), L"" );
		}

		for( auto it = m_rtpcValues.begin(); it != m_rtpcValues.end(); ++it )
		{
			SetRTPC( it->first, it->second );
		}

		for( auto it = m_switchValues.begin(); it != m_switchValues.end(); ++it )
		{
			SetSwitch( it->first, it->second );
		}

		SetAttenuationScalingFactor( m_scalingFactor );

		
		CcpAutoMutex mutex( m_mutex );
		for ( auto it = m_eventsOnWake.begin(); it != m_eventsOnWake.end(); ++it )
		{
			PostEvent( *it, true );
		}
		m_eventsOnWake.clear();

		CCP_LOG("Woke up game object %d", m_ID);
	}
}

//-----------------------------------------------------
// Description:
//   Handle logic for when a game object is culled and unregister this game object from Wwise.
//   When a game object is culled, any currently playing loops will be stopped and saved to be played again when the 
//   game object is woken up. If there are any one shot events currently playing within range of the listener 
//   they will be allowed to finish playing. However, if a one shot is currently playing outside of the range of the 
//	 the listener it will just be stopped.
//-----------------------------------------------------
void AudGameObjResource::Cull()
{
	if ( g_audioInitialized )
	{
		if ( m_forceCullingState )
		{
			return;	
		}

		CcpAutoMutex mutex( m_mutex );
		for ( auto it = m_playingEvents.begin(); it != m_playingEvents.end(); ++it )
		{
			if ( g_staticDataRepository->EventIsLoop( it->second ) )
			{
				StopSound( it->first, 3000 );
				m_eventsOnWake.insert( it->second );
			} 
			else
			{
				if ( m_listenerInRange )
				{
					BreakSound( it->first );
				}
				else
				{
					StopSound( it->first );
				}
			}
		}
		UnregisterWwiseObject();
		m_culled = true;
		CCP_LOG("Culled game object %d", m_ID);
		
	}
}

//------------------------------------------------------
// Description:
//   Calculates this game objects weight within the culling system and sets m_cumulativeWeight to the result.
//   The smaller the number the more prioritized this game object will be. If this game object is not muted, the following affects weight:
//     * How close this game object is to the listener.
//     * If any events that are playing or meant to be played on wake are within range of the listener.
//     * If any one shot was requested to play within the last 10 milliseconds.
//     * Whether this game object has been used at all to send events
//     * Whether this game object has currently active sounds being played on it. 
//     * Whether this game object is visible to the camera.
//	   * Whether this game object is currently playing a 2D sound.
//	   * Whether this game object is currently playing a sound tagged as vital in the Wwise project.
//------------------------------------------------------
void AudGameObjResource::CalculateCullingWeight( std::chrono::steady_clock::time_point now )
{
	// Determine if any one shots were requested within the last 10 milliseconds
	float waitingOneShotWeight = 0.0f;
	if ( m_culled )
	{
		if ( m_waitingOneShotInRange.second != L"" )
		{
			if ( std::chrono::duration_cast<std::chrono::milliseconds>( now - m_waitingOneShotInRange.first ) > std::chrono::milliseconds( g_audioManager->GetOneShotWindow() ) ) 
			{
				m_waitingOneShotInRange = std::pair( now, L"" );
			}
			else
			{
				if ( m_listenerInRange )
				{
					waitingOneShotWeight = g_audioManager->GetWaitingOneShotWeight();
				}
			}
		}
	}

	// Update m_listenerInRange if not muted
	if( !m_muted )
	{
		m_listenerInRange = m_distanceSqFromListener < GetMaxAttenuationRadius();
	}

    m_cumulativeWeight = SoundPrioritization::CalculateObjectWeight(
        m_distanceSqFromListener,
        m_muted,
        m_listenerInRange,
        m_isUsed,
        m_isVisible,
        m_playing2DSound,
        m_playingVitalSound,
        m_additionalCullingWeight,
        m_playingEvents.size(),
        waitingOneShotWeight,
        g_audioManager->GetUsedEmitterWeight(),
        g_audioManager->GetRangeWeight(),
        g_audioManager->GetPlayingEventsWeight(),
        g_audioManager->GetVisibleWeight(),
        g_audioManager->GetPlaying2DWeight(),
        g_audioManager->GetPlayingVitalSoundWeight()
    );
}


//-----------------------------------------------------
// Description:
//   Set the squared length of the distance between the listener and this game object.
// Arguments:
//   distanceSq - The squared length of the distance between the listener and this game object.
//-----------------------------------------------------
void AudGameObjResource::SetDistanceSqFromListener( const float distanceSq )
{
	m_distanceSqFromListener = distanceSq;
}

//-----------------------------------------------------
// Description:
//   Helper function to execute actions defined in the ActionTypes enum on a playingID.
// Arguments:
//   playingID - The playingID of the sound to execute the action on.
//   actionType - The action to execute on the sound (e.g. Stop, Break).
//   fadeOutDuration - If an action supports a fade then this sets the duration of that fade in milliseconds. Defaults to 1000
//-----------------------------------------------------
void AudGameObjResource::ExecuteActionOnPlayingID( const AkPlayingID playingID, const ActionTypes action, uint32_t fadeOutDuration )
{
	if ( g_audioInitialized )
	{
		CcpAutoMutex mutex( m_mutex );
		if ( m_playingEvents.find( playingID ) != m_playingEvents.end() )
		{
			switch ( action )
			{
			case ActionTypes::Stop:
				AK::SoundEngine::ExecuteActionOnPlayingID( AK::SoundEngine::AkActionOnEventType_Stop, playingID, fadeOutDuration );
				g_audioManager->LogExecuteActionOnPlayingID( m_ID, playingID, L"Stop" );
				break;
			case ActionTypes::Break:
				AK::SoundEngine::ExecuteActionOnPlayingID( AK::SoundEngine::AkActionOnEventType_Break, playingID, fadeOutDuration );
				g_audioManager->LogExecuteActionOnPlayingID( m_ID, playingID, L"Break" );
				break;
			}
		}
	}
}
//-----------------------------------------------------
// Description:
//	Calculates and updates sound prioritization attributes that are determined by currently playing events or events that will play on wake.
//  This should be called only after the state of what is currently playing or what is meant to be playing is changed. 
//-----------------------------------------------------
void AudGameObjResource::UpdateEventSoundPrioritizationAttributes()
{
	CcpAutoMutex mutex( m_mutex );

	if( m_playingEvents.empty() && m_eventsOnWake.empty() )
	{
		m_maxAttenuationRadiusSq = 0.0f;	
		m_playing2DSound = false;
		m_playingVitalSound = false;
	}
	else
	{
		bool playing2DSound = false;
		bool playingVitalSound = false;
		for ( auto it = m_playingEvents.begin(); it != m_playingEvents.end(); ++it )
		{
			UpdateMaxAttenuationRadiusForEvent( it->second );
			if( g_staticDataRepository->EventIs2D( it->second ) )
			{
				playing2DSound = true;
			}
			if( g_staticDataRepository->EventIsVital( it->second ) )
			{
				playingVitalSound = true;
			}
		}

		for ( auto it = m_eventsOnWake.begin(); it != m_eventsOnWake.end(); ++it )
		{
			UpdateMaxAttenuationRadiusForEvent( *it );
			if( g_staticDataRepository->EventIs2D( *it ) )
			{
				playing2DSound = true;
			}
			if( g_staticDataRepository->EventIsVital( *it ) )
			{
				playingVitalSound = true;
			}
		}

		m_playing2DSound = playing2DSound;
		m_playingVitalSound = playingVitalSound;
	}
}

//-----------------------------------------------------
// Description:
//	 Update the max attenuation radius of this game object if the given event's radius is larger than the current value. 
// Arguments:
//   eventName - The Wwise event to whose radius you want to update to if it's larger than the current value.
//-----------------------------------------------------
void AudGameObjResource::UpdateMaxAttenuationRadiusForEvent( const std::wstring& eventName )
{
	float eventMaxAttenuationRadiusSq = g_staticDataRepository->GetEventRadiusSq( eventName );
	m_maxAttenuationRadiusSq = std::max( m_maxAttenuationRadiusSq, eventMaxAttenuationRadiusSq );
}

//-----------------------------------------------------
// Description:
//	 Get the max attenuation radius. The scaling factor of this game object will also be taken into account.
//-----------------------------------------------------
float AudGameObjResource::GetMaxAttenuationRadius() const
{
	return m_maxAttenuationRadiusSq * m_scalingFactor;
}

const std::map<std::wstring, std::wstring>& AudGameObjResource::GetSwitches() const
{
	return m_switchValues;
}

std::map<unsigned int, std::wstring> AudGameObjResource::GetPlayingEvents()
{
	CcpAutoMutex mutex( m_mutex );
	return m_playingEvents;
}

//-----------------------------------------------------
// Description:
//   Force this game object to change its culling state and stay that way until ReleaseForcedCullingState is called.
//   This is meant for debug uses only so that the behavior of culling or waking up an object can be observed.
//-----------------------------------------------------
void AudGameObjResource::ForceCullingStateChange()
{
	// Temporarily release culling state so waking and culling work.
	m_forceCullingState = false;
	if ( m_culled )
	{
		Wake();
	}
	else
	{
		Cull();
	}
	m_forceCullingState = true;
}

//-----------------------------------------------------
// Description:
//   Release this game object from its forced culling state and give it back to the sound prioritzation system to decide.
//-----------------------------------------------------
void AudGameObjResource::ReleaseForcedCullingState()
{
	m_forceCullingState = false;
}

//-----------------------------------------------------
// Description:
//   Mute this game object so that it doesn't play any more sounds. In reality this method is actually just forcefully 
//   culling this game object so it can be in the correct state still when unmuted or woken up.
//-----------------------------------------------------
void AudGameObjResource::Mute()
{
	if ( m_muted )
	{
		return;
	}

	if ( !m_culled )
	{
		ForceCullingStateChange();
	}
	m_muted = true;
}

//-----------------------------------------------------
// Description:
//   Unmute this game object so that it can continue to play sounds as normal. In reality this method is actually just forcefully 
//   waking up this game object and returing control to the sound prioritization system.
//-----------------------------------------------------
void AudGameObjResource::Unmute()
{
	if ( !m_muted )
	{
		return;
	}

	if ( m_culled )
	{
		ForceCullingStateChange();
	}

	ReleaseForcedCullingState();
	m_muted = false;
}

bool AudGameObjResource::IsMuted()
{
	return m_muted;
}

bool AudGameObjResource::IsCulled() const 
{
    return m_culled;
}

float AudGameObjResource::GetCullingWeight() const 
{
    return m_cumulativeWeight;
}

AkGameObjectID AudGameObjResource::GetID() const 
{
    return m_ID;
}

Vector3 AudGameObjResource::GetPosition() const 
{
    return m_position;
}

std::wstring AudGameObjResource::GetEventName()
{
	return m_eventName;
}

void AudGameObjResource::SetEventName( const std::wstring& eventName )
{
	// Detect if the event name has changed
	bool eventNameChanged = (m_eventName != eventName);

	m_eventName = eventName;

	// If the event name has changed, trigger the new event
	if (eventNameChanged)
	{
		PostEvent( m_eventName );
	}
}