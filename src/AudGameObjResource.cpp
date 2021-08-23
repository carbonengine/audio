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
	m_parameters.SetNotify( this );
}

AudGameObjResource::~AudGameObjResource()
{
	if( g_audioInitialized )
    {
		// Silence the game object and put it on death row!
		PostEvent( L"fade_out" );
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

unsigned int AudGameObjResource::PostEvent( const std::wstring& name, bool bypassPrefix, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void * in_pCookie ) 
{
	if( g_audioInitialized )
	{
		std::wstring eventName = PrepareEvent( name, bypassPrefix );

		m_playID = AK::SoundEngine::PostEvent( eventName.c_str(), m_ID, in_uFlags, in_pfnCallback, in_pCookie);
		g_audioManager->LogPostEvent( m_ID, m_playID, AK_INVALID_UNIQUE_ID, eventName );

		if (m_playID == AK_INVALID_PLAYING_ID)
		{
			CCP_LOGERR( "Failed to send event to Wwise: %S", eventName.c_str() );
		}
		return m_playID;
	}
	return 0;
}

unsigned int AudGameObjResource::PySendEvent( const std::wstring& event, bool bypassPrefix )
{
	return PostEvent( event, bypassPrefix );
}

void AudGameObjResource::StopSound( AkPlayingID playingID )
{
	if ( g_audioInitialized )
	{
		AK::SoundEngine::ExecuteActionOnPlayingID( AK::SoundEngine::AkActionOnEventType_Stop, playingID );
		g_audioManager->LogStopPlayingID( m_ID, playingID );
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
