#include "StdAfx.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>

#include "AudGameObjResource.h"
#include "Vector3.h"
#include "Utilities.h"
#include "AudManager.h"
#include "AudPosition.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "Events" );

//-----------------------------------------------------------------------------
// Helper function to distribute ID's
//-----------------------------------------------------------------------------
static AkGameObjectID GenerateEntityID()
{
	static AkGameObjectID s_currentID = START_GAME_OBJ_COUNT;
	return s_currentID++;
}

AudGameObjResource::AudGameObjResource( IRoot* lockobj ) : PARENTLOCK( m_parameters )
														 , m_playID( 0 )
														 , m_playEvent(L"")
														 , m_playOnLoad( false )
														 , m_eventPrefix(L"")
														 , m_scalingFactor( 1.0 )
{
	m_ID = GenerateEntityID();
	m_parameters.SetNotify( this );
}

AudGameObjResource::~AudGameObjResource()
{
	if( g_audioInitialized )
        {
		// Silence the game object and put it on death row!
		AK::SoundEngine::PostEvent( L"fade_out", m_ID );
		
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

unsigned int AudGameObjResource::SendEvent( const std::wstring& name )
{
	if( g_audioInitialized )
	{
		std::wstring eventName;
		if (m_eventPrefix != L"")
		{
			eventName = std::wstring(m_eventPrefix) + std::wstring(name);
		}
		else
		{
			eventName = name;
		}

		CCP_LOG_CH( s_ch, "Sending event: %S to game object %d (%s)", name.c_str(), m_ID, m_name.c_str());
		m_playID = AK::SoundEngine::PostEvent( eventName.c_str(), m_ID );
		if (m_playID == AK_INVALID_PLAYING_ID)
		{
			AkUniqueID eventID = AK::SoundEngine::GetIDFromString(eventName.c_str());
			g_audioManager->AddWaitingEvent(eventID, m_ID);
		}
		return m_playID;
	}
	return 0;
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

int AudGameObjResource::SetSwitch( const wchar_t* groupName, const wchar_t* switchName )
{
	if( g_audioInitialized )
	{
		AK::SoundEngine::SetSwitch( groupName, switchName, m_ID );
	}
	return AK_Success;
}

int AudGameObjResource::SetPositionHelper( const AkSoundPosition& position )
{
	if( g_audioInitialized )
	{
		// all vectors come in RH, but WWISE is LH, so convert
		AkSoundPosition soundPosLH;
		RH2LH::convertEmitter( &soundPosLH, &position );

		AK::SoundEngine::SetPosition( m_ID, soundPosLH );
	}
	return AK_Success;
}

//----------------------------------
// Blue Interfaces
//----------------------------------
void AudGameObjResource::HandleEvent( const wchar_t* evtName )
{
	SendEvent( evtName );	//g_audioInitialized is checked in SendEvent
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
	Vector3 initpos( WISE_INIT_POSITION, WISE_INIT_POSITION, WISE_INIT_POSITION );
	AkSoundPosition tmp;
	tmp.Set(initpos, initpos, initpos);
	SetPositionHelper( tmp );

	//Start playing on loading from a red file.
	if( m_playOnLoad )
	{
		SendEvent( m_playEvent );	//SendEvent checks for g_audioEnabled
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
	