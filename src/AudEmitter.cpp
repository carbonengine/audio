#include "stdafx.h"
#include "AudEmitter.h"

#include "AudManager.h"
#include "AudStaticDataRepository.h"
#include "DebugUtilities.h"
#include "Vector3.h"

namespace
{
	constexpr const char* AUDIO_ATTENUATION_SPHERE_DEBUG_OPTION = "Audio Attenuation Sphere";
	constexpr const char* AUDIO_EMITTER_DIRECTION_DEBUG_OPTION = "Audio Emitter Direction";
}

AudEmitter::AudEmitter( IRoot* lockobj ) :
	AudGameObjResource( lockobj ),
	m_normalizeAttenuationScaling( false ),
	m_minNormalizedValue( 30.f ), // This default comes from the perspective that normalizing is most commonly used for bounding sphere radii of ships 
	m_maxNormalizedValue( 9000.f), // ^
	m_minNormalizedScalingFactor( 0.4f ),
	m_maxNormalizedScalingFactor( 3.5f ),
	m_debugColor( DebugUtilities::GenerateDebugColor( 0.4f, 0.9f ) ),
	m_simulationColor(0xff00ff00), // Green
	m_visualizationRadius(0.f),
	m_listenerDistanceScaleFactor(0.003f),
	m_radiusToTextWidthRatio(1.85f),
	m_debugFontCharWidth(0.8f)
{
}

AudEmitter::AudEmitter( AkGameObjectID gameObjID, IRoot* lockobj ) :
	AudGameObjResource( gameObjID, lockobj ),
	m_normalizeAttenuationScaling( false ),
	m_minNormalizedValue( 30.f ), // This default comes from the perspective that normalizing is most commonly used for bounding sphere radii of ships 
	m_maxNormalizedValue( 9000.f), // ^
	m_minNormalizedScalingFactor( 0.4f ),
	m_maxNormalizedScalingFactor( 3.5f ),
	m_debugColor( DebugUtilities::GenerateDebugColor( 0.4f, 0.9f ) ),
	m_simulationColor(0xff00ff00), // Green
	m_visualizationRadius(0.f),
	m_listenerDistanceScaleFactor(0.003f),
	m_radiusToTextWidthRatio(1.85f),
	m_debugFontCharWidth(0.8f)
{
}

AudEmitter::~AudEmitter()
{
}

void AudEmitter::Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position )
{
	AudGameObjResource::Initialize( name, prefix, position );
}

void AudEmitter::HandleEvent( const wchar_t* evtName )
{
	PostEvent( evtName );
}

void AudEmitter::SetName( const std::string& name )
{
	m_name = name;
}


void AudEmitter::SetPrefix( const std::wstring& prefix )
{
	m_eventPrefix = prefix;
}

int AudEmitter::SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	m_hasReceivedPosition = true;
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

bool AudEmitter::SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState )
{
	return AudGameObjResource::SetSwitch( switchGroup, switchState );
}


bool AudEmitter::SetRTPC( const std::wstring& rtpcName, float rtpcValue )
{
	return AudGameObjResource::SetRTPC( rtpcName, rtpcValue );
}

bool AudEmitter::SetAttenuationScalingFactor( const float scalingFactor )
{
	float finalScalingFactor = scalingFactor;
	if ( m_normalizeAttenuationScaling )
	{
		// Linearly normalize the scaling factor 
		finalScalingFactor = ( scalingFactor - m_minNormalizedValue ) * ( m_maxNormalizedScalingFactor - m_minNormalizedScalingFactor ) / ( m_maxNormalizedValue - m_minNormalizedValue ) + m_minNormalizedScalingFactor;
	}
	m_debugColor = 0xaaff0000; // Emitter's whose attenuation is affected by code will always show red in debug.
	return AudGameObjResource::SetAttenuationScalingFactor( finalScalingFactor );
}


void AudEmitter::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	SetPosition( front, top, pos );
}

void AudEmitter::SetVisibility( bool isVisible )
{
	m_isVisible = isVisible;
}

void AudEmitter::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( AUDIO_ATTENUATION_SPHERE_DEBUG_OPTION );
	options.insert( AUDIO_EMITTER_DIRECTION_DEBUG_OPTION );
}

void AudEmitter::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if ( m_culled || g_audioManager == nullptr || g_audioManager->GetState() != AudioState::Enabled )
	{
		return;
	}

	RenderDebugBoundingSphere( renderer );
	RenderDebugDirection( renderer );
	RenderDebugName( renderer );
}

void AudEmitter::RenderDebugBoundingSphere( ITr2DebugRenderer2& renderer )
{
	if ( !g_debugDisplayAllEmitters && !renderer.HasOption( GetRawRoot(), AUDIO_ATTENUATION_SPHERE_DEBUG_OPTION ) )
	{
		return; 
	}

	uint32_t debugSphereSegments = static_cast<uint32_t>(8.f + m_visualizationRadius / 5000.f);
	debugSphereSegments = (debugSphereSegments < 25) ? debugSphereSegments : 25; 

	if( m_visualizationRadius > 0.f )
	{
		float scaledRadius = m_visualizationRadius * m_scalingFactor;
		renderer.DrawSphere( this, m_position, scaledRadius, debugSphereSegments, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( m_simulationColor ) );
	}

	float emitterRange = m_visualizationRadius;
	emitterRange = std::max( emitterRange, AK::SoundEngine::Query::GetMaxRadius( m_ID ) );
	renderer.DrawSphere( this, m_position, emitterRange, debugSphereSegments, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( m_debugColor ) );
}

void AudEmitter::RenderDebugDirection( ITr2DebugRenderer2& renderer )
{
	if ( !renderer.HasOption( GetRawRoot(), AUDIO_EMITTER_DIRECTION_DEBUG_OPTION ) )
	{
		return;
	}

	AudListenerPtr listener = g_audioManager->GetListener();
	const float distanceToListener = Length( m_position - listener->GetPosition() );
	const float arrowLength = std::min( std::max( distanceToListener * 0.025f, 25.0f ), 250.0f );
	const float arrowRadius = std::max( arrowLength * 0.035f, 1.5f );
	Vector3 direction = Normalize( m_effectiveOrientation.front );

	renderer.DrawArrow(
		this,
		m_position,
		m_position + direction * arrowLength,
		arrowRadius,
		0.22f,
		12,
		ITr2DebugRenderer2::Solid,
		Tr2DebugColor( m_debugColor )
	);
}

void AudEmitter::RenderDebugName( ITr2DebugRenderer2& renderer )
{
	if ( !g_debugDisplayAllEmitters &&
		 !renderer.HasOption( GetRawRoot(), AUDIO_ATTENUATION_SPHERE_DEBUG_OPTION ) &&
		 !renderer.HasOption( GetRawRoot(), AUDIO_EMITTER_DIRECTION_DEBUG_OPTION ) )
	{
		return;
	}

	std::string debugName = m_name + "(" + std::to_string(m_ID) + ")";
	renderer.DrawText(TRI_DBG_FONT_SMALL, m_position, m_debugColor, debugName.c_str());
}

void AudEmitter::Mute()
{
	AudGameObjResource::Mute();
}

void AudEmitter::Unmute()
{
	AudGameObjResource::Unmute();
}


void AudEmitter::ForceCullingStateChange()
{
	AudGameObjResource::ForceCullingStateChange();
}

void AudEmitter::ReleaseForcedCullingState()
{
	AudGameObjResource::ReleaseForcedCullingState();
}
