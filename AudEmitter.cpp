#include "stdafx.h"
#include "AudEmitter.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>

AudEmitter::AudEmitter( IRoot* lockobj ) : 
	AudGameObjResource( lockobj ), 
	PARENTLOCK( m_position ), 
	m_debugPosition(0, 0, 0), 
	m_TEMP_testShitVariable( 0.f ), 
	m_TEMP_testShitVariable2( true )
{
}

AudEmitter::~AudEmitter()
{
}

int AudEmitter::SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	m_debugPosition = pos;
	return SetPositionHelper( front, top, pos );
}

//----------------------------------
// Blue Interfaces
//----------------------------------
void AudEmitter::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	m_debugPosition = pos;
	SetPositionHelper( front, top, pos );	//g_audioInitialized is checked in SetPositionHelper
}

// Debug
void AudEmitter::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "AudioEmitters" );
}

void AudEmitter::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if ( !renderer.HasOption( GetRawRoot(), "AudioEmitters" ) )
	{
		return;
	}

	// ----------------- temp ------------------
	// @Eric :	this was just for developing the bubbles, When you remove this make
	//			sure to remove the "m_temp_test.." from the header file as well :)
	if ( m_TEMP_testShitVariable2 )
	{
		m_TEMP_testShitVariable += 0.01f;
		if ( m_TEMP_testShitVariable > 1.f )
		{
			m_TEMP_testShitVariable2 = !m_TEMP_testShitVariable2;
		}
	}
	else
	{
		m_TEMP_testShitVariable -= 0.01f;
		if ( m_TEMP_testShitVariable < 0.f )
		{
			m_TEMP_testShitVariable2 = !m_TEMP_testShitVariable2;
		}
	}
	//-------------------- not temp ------------

	const float emitterRange = AK::SoundEngine::Query::GetMaxRadius(m_ID);							// @Eric : replace this with the emitter range
	const float VolumeNormalized = m_TEMP_testShitVariable;		// @Eric :replace this with a normalize of the volume ( loudness scale from 0 to 1 )


	const float volumeValue = ( VolumeNormalized < 0.f ) ? 0.f : ( 1.0f < VolumeNormalized ) ? 1.0f : VolumeNormalized; // clamp between 0 and 1
	const float reversedVV = 1.f - VolumeNormalized;

	uint32_t debugSphereSegments = static_cast< uint32_t >( 4.f + emitterRange / 5000.f );
	debugSphereSegments = ( debugSphereSegments < 25 ) ? debugSphereSegments : 25; // limit segment growth to 25
	const float varianceVectorRange = emitterRange / 35.f; // modify the last float here to increase bubble vibrancy ( lower float -> more vibration )

	float numSpheresController = -0.1f; // counter

	while ( numSpheresController < volumeValue )
	{
		float rand1 = 0.1f + static_cast< float >( rand() / double( RAND_MAX ) ) * 0.2f * volumeValue; // random float between 0.1 and 0.3 multiplied by vV
		float rand2 = static_cast< float >( rand() / double( RAND_MAX ) ) * 0.15f; // random float between 0.0 and 0.15 multiplied by vV

		Tr2DebugColor debugColor = Color(volumeValue, 0.5f * reversedVV * reversedVV + rand1,
		                                 volumeValue * (numSpheresController * 0.5f + rand2), 0.6f + 0.4f * reversedVV);

		// a random offset based on volume intensity
		Vector3 varianceVector = Vector3(
			volumeValue * (varianceVectorRange * ( static_cast< float >( rand() / double( RAND_MAX ) ) - 0.5f )),
			volumeValue * (varianceVectorRange * ( static_cast< float >( rand() / double( RAND_MAX ) ) - 0.5f )),
			volumeValue * (varianceVectorRange * ( static_cast< float >( rand() / double( RAND_MAX ) ) - 0.5f ))
		);

		varianceVector *= volumeValue * volumeValue;

		renderer.DrawSphere( this, m_debugPosition + varianceVector, emitterRange, debugSphereSegments, ITr2DebugRenderer2::Wireframe, debugColor );

		// the float here is controlling the max number of spheres  ( 1 / [number] ~> max number of spheres. so 0.25 -> 4 spheres at max volume (1) )
		numSpheresController += 0.23f;
	}

	renderer.DrawText( TRI_DBG_FONT_SMALL, m_debugPosition, 0x88ffffff, m_name.c_str() );
}