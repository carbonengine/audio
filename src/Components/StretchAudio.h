////////////////////////////////////////////////////////////
//
//    Creator:   Eric Nielsen
//    Created:   April 2020
//    Copyright: CCP 2020
//
//    Description:
// 		Provides audio functionality for stretch effects that span between two points.
// 		Manages source, destination, and interpolated audio emitters that follow
// 		the listener's position along the stretch path. Audio event timing is controlled
// 		by the stretch effect system. However, it can be used purely for the emitter's
// 		positioning functionality in tandem with a controller or other system

#pragma once

#include <IStretchAudio.h>
#include <ITr2DebugRenderer2.h>
#include <IInitialize.h>

#include "AudEmitter.h"
#include "AudListener.h"

BLUE_CLASS( StretchAudio ) :
	public IStretchAudio,
	public IInitialize,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	StretchAudio( IRoot* lockobj = NULL );
    virtual ~StretchAudio();

	// IInitialize
	bool Initialize() override;

	// Places the source and destination audio emitters at the source and
	// destination positions of the stretch effect. Places the stretch
	// emitter between those two points relative to the listener.
	void Update( Vector3& sourcePosition, Vector3& destPosition );
	ITr2AudEmitterPtr FindEmitterByName( const char* name ) override;

	// IStretchAudioAuto
	void Start() override;
	void Stop() override;

	void SetShotMissed( bool missed ); 

	// debug
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );
protected:
	std::wstring m_outburstEvent;
	std::wstring m_impactEvent;
	std::wstring m_stretchEvent;
	std::wstring m_shotMissedEvent;
	AudEmitterPtr m_sourceEmitter;
	AudEmitterPtr m_destEmitter;
	AudEmitterPtr m_stretchEmitter;
private:
	bool m_shotMissed;
    AudListenerPtr m_listener;

	// Projects the listener's position onto the line segment defined by the source and destination positions.
	Vector3 ProjectListenerOntoSegment( const Vector3& sourcePos, const Vector3& destPos );
};

TYPEDEF_BLUECLASS( StretchAudio );
