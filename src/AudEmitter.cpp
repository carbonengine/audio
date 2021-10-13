#include "stdafx.h"
#include "AudEmitter.h"
#include "AudManager.h"
#include "DebugUtilities.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

AudEmitter::AudEmitter( IRoot* lockobj ) :
	AudGameObjResource( lockobj ),
	m_debugPosition(0, 0, 0),
	m_debugFront(0, 0, 0),
	m_debugColor()
{
	Initialize();
}

AudEmitter::~AudEmitter()
{
	g_audioManager->UnregisterAudEmitter( this );
}

bool AudEmitter::Initialize()
{
	g_audioManager->RegisterAudEmitter( this );
	return AudGameObjResource::Initialize();
}

void AudEmitter::Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position )
{
	AudGameObjResource::Initialize( name, prefix, position );
}

void AudEmitter::SetName( const std::string& name )
{
	m_name = name;
}

int AudEmitter::SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	m_debugPosition = pos;
	m_debugFront = front;
	return SetPositionHelper( front, top, pos );
}

unsigned int AudEmitter::SendEvent( const std::wstring& name, bool bypassPrefix )
{
	return PostEvent( name, bypassPrefix );
}

std::string AudEmitter::GetName()
{
	return m_name;
}

void AudEmitter::SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState )
{
	AudGameObjResource::SetSwitch( switchGroup, switchState );
}


void AudEmitter::SetRTPC( const std::wstring& rtpcName, float rtpcValue )
{
	AudGameObjResource::SetRTPC( rtpcName, rtpcValue );
}

int AudEmitter::SetAttenuationScalingFactor( const float scalingFactor )
{
	m_debugColor = 0xaaff0000; // Emitter's whose attenuation is affected by code will always show red in debug.
	return AudGameObjResource::SetAttenuationScalingFactor( scalingFactor );
}

//----------------------------------
// Needed for IBluePlacementObserver interface.
//----------------------------------
void AudEmitter::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	SetPosition( front, top, pos );
}

bool AudEmitter::StopEvent( const std::wstring& eventName )
{
	std::wstring fullEventName = PrepareEvent( eventName, false );
	if ( m_playedEvents.count(fullEventName) > 0 )
	{
		AkPlayingID playingID = m_playedEvents.find( fullEventName )->second;
		StopSound(playingID);
		m_playedEvents.erase( fullEventName );
		return true;
	}
	return false;
}

// Debug
void AudEmitter::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "AudioEmitters" );
}

void AudEmitter::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if ( g_audioInitialized )
	{
		if ( !g_debugDisplayAllEmitters )
		{
			if ( !renderer.HasOption( GetRawRoot(), "AudioEmitters" ) )
			{
				return;
			}
		}

		if ( !m_debugColor )
		{
			float minRange = 0.4f;
			float maxRange = 0.9f;
			m_debugColor = DebugUtilities::GenerateDebugColor( minRange, maxRange );
		}

		const float emitterRange = AK::SoundEngine::Query::GetMaxRadius( m_ID );
		uint32_t debugSphereSegments = static_cast<uint32_t>(8.f + emitterRange / 5000.f);
		debugSphereSegments = (debugSphereSegments < 25) ? debugSphereSegments : 25; // limit segment growth to 25

		renderer.DrawSphere( this, m_debugPosition, emitterRange, debugSphereSegments, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( m_debugColor ) );
		renderer.DrawText( TRI_DBG_FONT_SMALL, m_debugPosition, m_debugColor, m_name.c_str() );
	}
}