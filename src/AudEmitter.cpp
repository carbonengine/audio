#include "stdafx.h"
#include "AudEmitter.h"

#include "AudManager.h"
#include "AudStaticDataRepository.h"
#include "DebugUtilities.h"
#include "Vector3.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
#ifndef AK_OPTIMIZED
	constexpr float DEBUG_CONE_PI = 3.14159265358979323846f;
	constexpr uint32_t DEBUG_CONE_SEGMENTS = 32;
	constexpr uint32_t DEBUG_CONE_RAY_STEP = 4;
	constexpr uint32_t DEBUG_CONE_FALLOFF_CONNECTOR_STEP = 8;
	const Tr2DebugColor DEBUG_CONE_INNER_COLORS[] = {
		Tr2DebugColor( 0xaa00ffff ),
		Tr2DebugColor( 0xaa00ff80 ),
		Tr2DebugColor( 0xaaff00ff ),
		Tr2DebugColor( 0xaaffff00 )
	};

	Vector3 NormalizeDebugVector( const Vector3& vector, const Vector3& fallback )
	{
		const float length = Length( vector );
		if( length <= 0.0001f )
		{
			return fallback;
		}
		return Vector3( vector.x / length, vector.y / length, vector.z / length );
	}

	Vector3 GetPerpendicularDebugVector( const Vector3& forward )
	{
		Vector3 right = Cross( forward, Vector3( 0.0f, 1.0f, 0.0f ) );
		if( Length( right ) <= 0.0001f )
		{
			right = Cross( forward, Vector3( 0.0f, 0.0f, 1.0f ) );
		}
		return NormalizeDebugVector( right, Vector3( 1.0f, 0.0f, 0.0f ) );
	}

	Tr2DebugColor WithDebugAlpha( Tr2DebugColor color, uint32_t alpha )
	{
		return Tr2DebugColor( ( color.m_color & 0x00ffffff ) | ( ( alpha & 0xff ) << 24 ) );
	}

	bool GetClippedConePoint( const Vector3& origin, const Vector3& forward, float radius, float angleDegrees, uint32_t segmentIndex, Vector3& outPoint )
	{
		const float halfAngleDegrees = std::max( 0.0f, std::min( angleDegrees * 0.5f, 180.0f ) );
		if( radius <= 0.0f || halfAngleDegrees <= 0.0f )
		{
			return false;
		}

		const float drawableHalfAngleDegrees = std::min( halfAngleDegrees, 179.0f );
		const float halfAngleRadians = drawableHalfAngleDegrees * DEBUG_CONE_PI / 180.0f;
		const float coneForward = std::cos( halfAngleRadians ) * radius;
		const float coneSide = std::sin( halfAngleRadians ) * radius;
		const Vector3 right = GetPerpendicularDebugVector( forward );
		const Vector3 up = NormalizeDebugVector( Cross( right, forward ), Vector3( 0.0f, 1.0f, 0.0f ) );
		const float ringRadians = ( static_cast<float>( segmentIndex % DEBUG_CONE_SEGMENTS ) / static_cast<float>( DEBUG_CONE_SEGMENTS ) ) * 2.0f * DEBUG_CONE_PI;
		const Vector3 radial = right * std::cos( ringRadians ) + up * std::sin( ringRadians );

		outPoint = origin + forward * coneForward + radial * coneSide;
		return true;
	}

	void DrawClippedConeAngle( ITr2DebugRenderer2& renderer, AudEmitter* owner, const Vector3& origin, const Vector3& forward, float radius, float angleDegrees, Tr2DebugColor color, uint32_t boundaryRayStep )
	{
		Vector3 previousPoint;
		for( uint32_t i = 0; i <= DEBUG_CONE_SEGMENTS; ++i )
		{
			Vector3 point;
			if( !GetClippedConePoint( origin, forward, radius, angleDegrees, i, point ) )
			{
				return;
			}

			if( i > 0 )
			{
				renderer.DrawLine( owner, previousPoint, point, color );
			}
			if( boundaryRayStep > 0 && i < DEBUG_CONE_SEGMENTS && i % boundaryRayStep == 0 )
			{
				renderer.DrawLine( owner, origin, point, color );
			}

			previousPoint = point;
		}
	}

	void DrawConeFalloffBand( ITr2DebugRenderer2& renderer, AudEmitter* owner, const Vector3& origin, const Vector3& forward, float radius, float innerAngleDegrees, float outerAngleDegrees, Tr2DebugColor color )
	{
		if( outerAngleDegrees <= innerAngleDegrees + 0.5f )
		{
			return;
		}

		const Tr2DebugColor falloffColor = WithDebugAlpha( color, 0x55 );
		DrawClippedConeAngle( renderer, owner, origin, forward, radius, outerAngleDegrees, falloffColor, DEBUG_CONE_FALLOFF_CONNECTOR_STEP );

		for( uint32_t i = 0; i < DEBUG_CONE_SEGMENTS; i += DEBUG_CONE_FALLOFF_CONNECTOR_STEP )
		{
			Vector3 innerPoint;
			Vector3 outerPoint;
			if( GetClippedConePoint( origin, forward, radius, innerAngleDegrees, i, innerPoint ) &&
				GetClippedConePoint( origin, forward, radius, outerAngleDegrees, i, outerPoint ) )
			{
				renderer.DrawLine( owner, innerPoint, outerPoint, falloffColor );
			}
		}
	}
#endif
}

AudEmitter::AudEmitter( IRoot* lockobj ) :
	AudGameObjResource( lockobj ),
	m_normalizeAttenuationScaling( false ),
	m_minNormalizedValue( 30.f ), // This default comes from the perspective that normalizing is most commonly used for bounding sphere radii of ships 
	m_maxNormalizedValue( 9000.f), // ^
	m_minNormalizedScalingFactor( 0.4f ),
	m_maxNormalizedScalingFactor( 3.5f ),
	m_debugColor(0, 0, 0, 0),
	m_simulationColor(0xff00ff00), // Green
	m_visualizationRadius(0.f),
#ifndef AK_OPTIMIZED
	m_debugFront( 1.0f, 0.0f, 0.0f ),
#endif
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
	m_debugColor(0, 0, 0, 0),
	m_simulationColor(0xff00ff00), // Green
	m_visualizationRadius(0.f),
#ifndef AK_OPTIMIZED
	m_debugFront( 1.0f, 0.0f, 0.0f ),
#endif
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
#ifndef AK_OPTIMIZED
	m_debugFront = NormalizeDebugVector( front, Vector3( 1.0f, 0.0f, 0.0f ) );
#endif
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

// Debug
void AudEmitter::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Audio Attenuation Sphere" );
}

void AudEmitter::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if ( g_audioManager != nullptr && g_audioManager->GetState() == AudioState::Enabled )
	{
		if ( !g_debugDisplayAllEmitters )
		{
			if ( !renderer.HasOption( GetRawRoot(), "Audio Attenuation Sphere" ) )
			{
				return;
			}
		}
		else if( !g_audioManager->ShouldDebugDisplayEmitter( m_ID ) )
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

		if ( !m_culled )
		{
			if ( Dot(m_debugColor, m_debugColor) == 0 )
			{
				float minRange = 0.4f;
				float maxRange = 0.9f;
				m_debugColor = DebugUtilities::GenerateDebugColor( minRange, maxRange );
			}

			const float emitterRange = AK::SoundEngine::Query::GetMaxRadius( m_ID );
			renderer.DrawSphere( this, m_position, emitterRange, debugSphereSegments, ITr2DebugRenderer2::Wireframe, Tr2DebugColor( m_debugColor ) );

#ifndef AK_OPTIMIZED
			DrawDebugAttenuationCone( renderer );
#endif

			DrawClickableRadius(renderer);

			std::string debugName = m_name + "(" + std::to_string(m_ID) + ")";
			renderer.DrawText(TRI_DBG_FONT_SMALL, m_position, m_debugColor, debugName.c_str());
		}
	}
}

#ifndef AK_OPTIMIZED
void AudEmitter::DrawDebugAttenuationCone(ITr2DebugRenderer2& renderer)
{
	if( !g_debugDisplayAllEmitters )
	{
		return;
	}

	g_audioManager->RequestDebugAttenuationConeData( m_ID, GetPlayingEvents() );

	std::vector<DebugAttenuationConeData> coneDataList;
	if( !g_audioManager->GetDebugAttenuationConeDataList( m_ID, coneDataList ) )
	{
		return;
	}

	const Vector3 forward = NormalizeDebugVector( m_debugFront, Vector3( 1.0f, 0.0f, 0.0f ) );
	constexpr size_t debugConeColorCount = sizeof( DEBUG_CONE_INNER_COLORS ) / sizeof( DEBUG_CONE_INNER_COLORS[0] );
	for( size_t index = 0; index < coneDataList.size(); ++index )
	{
		const DebugAttenuationConeData& coneData = coneDataList[index];
		const float height = coneData.maxRadius * m_scalingFactor;
		if( height <= 0.0f || coneData.innerAngle <= 0.0f )
		{
			continue;
		}

		const Tr2DebugColor innerColor = DEBUG_CONE_INNER_COLORS[index % debugConeColorCount];
		DrawClippedConeAngle( renderer, this, m_position, forward, height, coneData.innerAngle, innerColor, DEBUG_CONE_RAY_STEP );
		DrawConeFalloffBand( renderer, this, m_position, forward, height, coneData.innerAngle, coneData.outerAngle, innerColor );

		char coneLabel[256];
		std::snprintf( coneLabel, sizeof( coneLabel ), "%s inner %.1f outer %.1f cone %.1fdB", coneData.attenuationName.c_str(), coneData.innerAngle, coneData.outerAngle, coneData.coneAttenuation );
		renderer.DrawText( TRI_DBG_FONT_SMALL, m_position + forward * std::min( height * 0.25f, 1000.0f ), Color( innerColor.m_color ), coneLabel );
	}
}
#endif

void AudEmitter::DrawClickableRadius(ITr2DebugRenderer2& renderer)
{
	// In order to scale the clickable radius we need to know the distance to the camera. This is not so 
	// easy to get from our library so we use the listener position because the listener follows the camera.
	AudListenerPtr listener = g_audioManager->GetListener(); 
	Vector3 listenerPos = listener->GetPosition();
	float distanceToListener = Length( m_position - listenerPos );
	float scaleFactor = distanceToListener * m_listenerDistanceScaleFactor; 

	float textWidth = m_name.length() * m_debugFontCharWidth * scaleFactor;
	float radius = textWidth * m_radiusToTextWidthRatio; 

	// Draw barely visible transparent sphere
	renderer.DrawSphere( this, m_position, radius, 8, ITr2DebugRenderer2::Solid, Tr2DebugColor( 0x08000000 ) );
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
