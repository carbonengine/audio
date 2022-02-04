#include "stdafx.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>

#include "AudGameObjResource.h"
#include "Vector3.h"
#include "Utilities.h"
#include "AudManager.h"
#include "AudPosition.h"

//-----------------------------------------------------------------------------
// Helper function to distribute ID's
//-----------------------------------------------------------------------------
static AkGameObjectID GenerateEntityID()
{
	static AkGameObjectID s_currentID = START_GAME_OBJ_COUNT;
	return s_currentID++;
}

AudGameObjResource::AudGameObjResource( IRoot* lockobj ) : PARENTLOCK( m_parameters ),
														 m_playID( 0 ),
														 m_playEvent(L""),
														 m_playOnLoad( false ),
														 m_eventPrefix(L""),
														 m_scalingFactor( 1.0 ),
														 m_position( 1.0e+7F, 1.0e+7F, 1.0e+7F ) // WWISE INIT POSITION
{
	m_ID = GenerateEntityID();

	if (g_audioManager != nullptr)
	{
		g_audioManager->RegisterAudEmitter( m_ID, this );
	}

	m_parameters.SetNotify( this );
}

AudGameObjResource::AudGameObjResource( AkGameObjectID gameObjID, IRoot* lockobj ) : PARENTLOCK( m_parameters ),
																				   m_playID( 0 ),
																				   m_playEvent(L""),
																				   m_playOnLoad( false ),
																				   m_eventPrefix(L""),
																				   m_scalingFactor( 1.0 ),
																				   m_position( 1.0e+7F, 1.0e+7F, 1.0e+7F ) // WWISE INIT POSITION
{
	m_ID = gameObjID;

	if (g_audioManager != nullptr)
	{
		g_audioManager->RegisterAudEmitter( m_ID, this );
	}

	m_parameters.SetNotify( this );
}

AudGameObjResource::~AudGameObjResource()
{
	if ( g_audioManager != nullptr )
	{
		g_audioManager->UnregisterAudEmitter( m_ID );
	}

	if( g_audioInitialized )
    {
		// Make sure end of event callbacks do not happen after this object is destroyed.
		AK::SoundEngine::CancelEventCallbackGameObject( m_ID );
		StopAll();
		g_audioManager->AddToDestructionVector( m_ID );
	}
}

void AudGameObjResource::CreateWwiseObject()
{
	if( g_audioInitialized )
	{
		AK::SoundEngine::RegisterGameObj( m_ID, m_name.c_str() );
	}
}

void AudGameObjResource::LogInfo()
{
	//if( g_audioInitialized )
	//{
	//Currently does nothing
	//AK::SoundEngine::Query::GetPosition( m_ID, tmpSoundPosition );
	//}
}

unsigned int AudGameObjResource::PostEvent( const std::wstring& name, bool bypassPrefix, AkUInt32 additionalFlags )
{
	if( g_audioInitialized )
	{

		std::wstring eventName = PrepareEvent( name, bypassPrefix );

		// Set up callback info so that we get a callback when every event is finished playing.
		AkUInt32 inFlags = AK_EndOfEvent | additionalFlags;
		AkCallbackFunc callback = &AudGameObjResource::PropagateWwiseCallback;

		m_playID = AK::SoundEngine::PostEvent( eventName.c_str(), m_ID, inFlags, callback, this );
		g_audioManager->LogPostEvent( m_ID, m_playID, AK_INVALID_UNIQUE_ID, eventName );

		if ( m_playID != AK_INVALID_PLAYING_ID )
		{
			m_playingEvents.insert({m_playID, eventName});
		}

		if (m_playID == AK_INVALID_PLAYING_ID)
		{
			CCP_LOGERR( "Failed to send event to Wwise: %S", eventName.c_str() );
		}
		return m_playID;
	}
	return 0;
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
	m_playingEvents.erase(cbInfo->playingID);
}

unsigned int AudGameObjResource::PySendEvent( const std::wstring& event, bool bypassPrefix )
{
	return PostEvent( event, bypassPrefix );
}

void AudGameObjResource::StopSound( AkPlayingID playingID, uint32_t fadeOutDuration )
{
	if ( g_audioInitialized )
	{
		AK::SoundEngine::ExecuteActionOnPlayingID( AK::SoundEngine::AkActionOnEventType_Stop, playingID, fadeOutDuration );
		g_audioManager->LogStopPlayingID( m_ID, playingID );
	}
}

void AudGameObjResource::StopAll()
{
	if( g_audioInitialized )
	{
		for ( auto it = begin( m_playingEvents ); it != end( m_playingEvents ); ++it)
		{
			StopSound( it->first );
		}
	}
}

int AudGameObjResource::SetAttenuationScalingFactor( float value )
{
	if( g_audioInitialized )
	{
		m_scalingFactor = value;
		return AK::SoundEngine::SetScalingFactor( m_ID, value );
	}
	return AK_InvalidParameter;
}

int AudGameObjResource::SetObstructionAndOcclusion( unsigned int listenerID, float obstruction, float occlusion )
{
	if( g_audioInitialized )
	{
		AK::SoundEngine::SetObjectObstructionAndOcclusion( m_ID, listenerID, obstruction, occlusion );
	}
	return AK_Success;
}

int AudGameObjResource::SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position )
{
	if( g_audioInitialized )
	{
		AkSoundPosition tmp;
		Vector3 correctFront = Normalize( front );
		Vector3 correctUp = Normalize( top );
		correctUp = Normalize( Cross( Cross( correctFront, correctUp ), correctFront ) );
		tmp.Set( MakeAkVector(position), MakeAkVector(correctFront), MakeAkVector(correctUp) );

		// all vectors come in RH, but WWISE is LH, so convert
		AkSoundPosition soundPosLH;
		RH2LH::convertEmitter( &soundPosLH, &tmp);

		AK::SoundEngine::SetPosition( m_ID, soundPosLH );
	}
	return AK_Success;
}

//----------------------------------
// Blue Interfaces
//----------------------------------
void AudGameObjResource::HandleEvent( const wchar_t* evtName )
{
	PostEvent( evtName );	//g_audioInitialized is checked in PostEvent
}


bool AudGameObjResource::Initialize()
{
	// Hack for the scenario that comes up when loading stations for instance
	// what happens is that an AudEmitterMultiAuto gets created at 0,0,0 and immediatly
	// starts playing, an update loop occurs and it gets put to it's correct place
	// in world space.
	// What this does it that it takes the entity and places it as far away as it
	// can during this initial phase and then it get placed to its correct place
	// after the first update loop

	CreateWwiseObject();
	SetPositionHelper( Vector3(1,0,0), Vector3(0,1,0), m_position );

	//Start playing on loading from a red file.
	if( m_playOnLoad )
	{
		PostEvent( m_playEvent );	//PostEvent checks for g_audioEnabled
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

void AudGameObjResource::Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position )
{
	m_name = name;
	m_eventPrefix = prefix;
	m_position = position;
	Initialize();
}

void AudGameObjResource::SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState )
{
	if ( g_audioInitialized )
	{
		AK::SoundEngine::SetSwitch( switchGroup.c_str(), switchState.c_str(), m_ID );
		g_audioManager->LogSetSwitch( m_ID, switchGroup, switchState );
	}
}

void AudGameObjResource::SetRTPC( const std::wstring& rtpcName, float rtpcValue )
{
	if ( g_audioInitialized )
	{
		AK::SoundEngine::SetRTPCValue( rtpcName.c_str(), AkRtpcValue(rtpcValue), m_ID );
		g_audioManager->LogSetRTPC( m_ID, rtpcName, rtpcValue );
	}
}

//-----------------------------------------------------
// Description:
//   Seek on an event by using a percentage of its duration. 
// Arguments:
//   playingID - The playing ID of the event to seek. Returned from PostEvent().
//   percentToSeek - The desired position, in percentage, where you want playback of this event to restart.
//    				 Expressed in a percentage of the audio file's total duration between 0 and 1.
//-----------------------------------------------------
void AudGameObjResource::SeekOnEventPercent( const unsigned int playingID, float percentToSeek )
{
	if ( g_audioInitialized )
	{

		auto it = m_playingEvents.find( playingID );
		if ( it != m_playingEvents.end() )
		{
			AKRESULT result = AK::SoundEngine::SeekOnEvent( it->second.c_str(), m_ID, percentToSeek, false, it->first);
			if ( result != AK_Success )
			{
				CCP_LOGERR( "Failed to seek on event %S", it->second.c_str() );
			}
		}
	}
}

//-----------------------------------------------------
// Description:
//   Seek on an event using milliseconds. 
// Arguments:
//   playingID - The playing ID of the event to seek. Returned from PostEvent().
//   msToSeek - Desired position where playback should restart, in milliseconds.
//-----------------------------------------------------
void AudGameObjResource::SeekOnEventMs( const unsigned int playingID, const unsigned int msToSeek)
{
	if ( g_audioInitialized )
	{

		auto it = m_playingEvents.find( playingID );
		if ( it != m_playingEvents.end() )
		{
			AKRESULT result = AK::SoundEngine::SeekOnEvent( it->second.c_str(), m_ID, AkTimeMs(msToSeek), false, it->first);
			if ( result != AK_Success )
			{
				CCP_LOGERR( "Failed to seek on event %S", it->second.c_str() );
			}
		}
	}
}


// Formats events to be sent to Wwise. bypassPrefix determines whether or not to apply the prefix
// defined on the emitter to the event.
std::wstring AudGameObjResource::PrepareEvent( const std::wstring& event, bool bypassPrefix )
{
	std::wstring eventName;
	if ( m_eventPrefix != L"" && bypassPrefix == false )
	{
		eventName = std::wstring(m_eventPrefix) + std::wstring(event);
	}
	else
	{
		eventName = event;
	}

	return eventName;
}
