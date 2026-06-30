// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "StretchAudio.h"
#include "AudManager.h"

StretchAudio::StretchAudio( IRoot* lockobj ) : 
	m_impactEvent( L"" ),
	m_outburstEvent( L"" ),
	m_stretchEvent( L"" ),
	m_shotMissedEvent( L"" ),
	m_shotMissed( false )
{
	Initialize();
}

StretchAudio::~StretchAudio()
{
	Stop();
}

bool StretchAudio::Initialize()
{
	// If emitters don't already exist, create and set default names
	if ( nullptr == m_sourceEmitter )
	{
        m_sourceEmitter.CreateInstance();
		m_sourceEmitter->Initialize( "stretch_source_sfx", L"", Vector3(0,0,0) );
	}
	if ( nullptr == m_destEmitter)
	{
		m_destEmitter.CreateInstance();
		m_destEmitter->Initialize( "stretch_dest_sfx", L"", Vector3(0,0,0) );
	}
	if ( nullptr == m_stretchEmitter)
	{
        m_stretchEmitter.CreateInstance();
		m_stretchEmitter->Initialize( "stretch_mid_sfx", L"", Vector3(0,0,0) );
	}

	return true;
}

void StretchAudio::Update( Vector3& sourcePosition, Vector3& destPosition )
{
	if ( !g_audioEnabled )
	{
		return;
	}

	Vector3 front(0,1,0), top(0,0,1);
	if ( nullptr != m_sourceEmitter )
	{
		m_sourceEmitter->SetPosition( front, top, sourcePosition );
	}
	if ( nullptr != m_destEmitter )
	{
		m_destEmitter->SetPosition( front, top, destPosition );
	}
	if ( nullptr != m_stretchEmitter)
	{
		Vector3 projectedPos = ProjectListenerOntoSegment( sourcePosition, destPosition );
		m_stretchEmitter->SetPosition( front, top, projectedPos );
	}
}

Vector3 StretchAudio::ProjectListenerOntoSegment( const Vector3& sourcePos, const Vector3& destPos )
{
	if ( nullptr == m_listener )
	{
		m_listener = g_audioManager->GetListener();
		if ( nullptr == m_listener )
		{
			CCP_LOGERR( "No listener found for StretchAudio, cannot position stretch emitter." );
			return Vector3( 0, 0, 0 );
		}
	}
    Vector3 listenerPos = m_listener->GetPosition();
    Vector3 toListener = listenerPos - sourcePos;
    Vector3 segment = destPos - sourcePos;
    
    float segmentLengthSquared = Dot(segment, segment);
    if (segmentLengthSquared < 1e-6f) {
        return sourcePos;  // Can't project onto a point, so use source position
    }
    
    float t = Dot(toListener, segment) / segmentLengthSquared;
    t = std::max(0.0f, std::min(t, 1.0f));
    return sourcePos + t * segment;
}

ITr2AudEmitterPtr StretchAudio::FindEmitterByName( const char* name )
{	
    auto checkEmitter = [&](const AudEmitterPtr& emitter) -> ITr2AudEmitterPtr {
        if (emitter && emitter->GetName() == name) {
            return dynamic_cast<ITr2AudEmitter*>(emitter.p);
        }
        return nullptr;
    };

    if (auto result = checkEmitter(m_sourceEmitter)) return result;
    if (auto result = checkEmitter(m_destEmitter)) return result;
    if (auto result = checkEmitter(m_stretchEmitter)) return result;
    
    return nullptr;
}

void StretchAudio::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_sourceEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_destEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_stretchEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void StretchAudio::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_sourceEmitter.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_destEmitter.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_stretchEmitter.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
}

void StretchAudio::Start()
{
	if ( nullptr != m_sourceEmitter )
	{
		m_sourceEmitter->SendEvent( m_outburstEvent );
	}
	if ( nullptr != m_destEmitter)
	{
		m_destEmitter->SendEvent( m_impactEvent );
	}
	if ( nullptr != m_stretchEmitter)
	{
		if ( m_shotMissed )
		{
			m_stretchEmitter->SendEvent( m_shotMissedEvent );
		}

		m_stretchEmitter->SendEvent( m_stretchEvent );
	}
}

void StretchAudio::Stop()
{
	if ( nullptr != m_sourceEmitter )
	{
		m_sourceEmitter->StopAll();
	}
	if ( nullptr != m_destEmitter)
	{
		m_destEmitter->StopAll();
	}
	if ( nullptr != m_stretchEmitter)
	{
		m_stretchEmitter->StopAll();
	}
}

void StretchAudio::SetShotMissed( bool missed )
{
	m_shotMissed = missed;
}