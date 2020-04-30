#include "stdafx.h"
#include "AudEmitter.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>

AudEmitter::AudEmitter( IRoot* lockobj ) :
	AudGameObjResource( lockobj ),
	PARENTLOCK( m_position ),
	m_debugPosition(0, 0, 0),
	m_debugColor()
{
	Initialize();
}

AudEmitter::~AudEmitter()
{
}

bool AudEmitter::Initialize()
{
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
	return SetPositionHelper( front, top, pos );
}

unsigned int AudEmitter::SendEvent( const std::wstring& name )
{
	return AudGameObjResource::SendEvent( name );
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

//----------------------------------
// Needed for IBluePlacementObserver interface.
//----------------------------------
void AudEmitter::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	SetPosition( front, top, pos );
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
			float rand1 = static_cast<float>(minRange + (rand() / double( RAND_MAX )) * (maxRange - minRange));
			float rand2 = static_cast<float>(minRange + (rand() / double( RAND_MAX )) * (maxRange - minRange));
			float rand3 = static_cast<float>(minRange + (rand() / double( RAND_MAX )) * (maxRange - minRange));
			m_debugColor = Color( rand1, rand2, rand3, 1.0f );
		}

		const float emitterRange = AK::SoundEngine::Query::GetMaxRadius( m_ID );

		uint32_t debugSphereSegments = static_cast<uint32_t>(8.f + emitterRange / 5000.f);
		debugSphereSegments = (debugSphereSegments < 25) ? debugSphereSegments : 25; // limit segment growth to 25

		renderer.DrawSphere( this, m_debugPosition, emitterRange, debugSphereSegments, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( m_debugColor ) );
		renderer.DrawText( TRI_DBG_FONT_SMALL, m_debugPosition, m_debugColor, m_name.c_str() );
	}
}