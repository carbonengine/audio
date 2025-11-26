////////////////////////////////////////////////////////////
//
// Creator: Andri Mar
// Contributors: Eric Nielsen
// Creation Date: June 2010
// Copyright (c) 2010-2022, CCP Games
//

#pragma once
#ifndef _AUDEMITTER_H_
#define _AUDEMITTER_H_

#include <IBlueEventListener.h>
#include <ITr2DebugRenderer2.h>
#include <ITr2AudEmitter.h>

#include "AudGameObjResource.h"
#include "AudPosition.h"

struct Vector3;

// ------------------------------------------------------------------------
// Description:
//   This class encapsulates an audio emitter in EVE and is exposed both
//   through Blue and to Trinity through ITr2AudEmitter.
// SeeAlso:
//   ITr2AudEmitter, AudGameObjResource
// ------------------------------------------------------------------------
BLUE_CLASS( AudEmitter ) :
	public IBlueEventListener,
	public IBluePlacementObserver,
	public AudGameObjResource,
	public ITr2DebugRenderable,
	public ITr2AudEmitter
{
public:
	AudEmitter( IRoot* lockobj = NULL );
	virtual ~AudEmitter();

	EXPOSE_TO_BLUE();

	// IBlueEventListener
	void HandleEvent( const wchar_t* evtName ) override;

	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos ) override;

	void Py__init__( const std::string& name );

	//ITr2AudEmitter
	void ForceCullingStateChange() override;
	void Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position ) override;
	void Mute() override;
	void ReleaseForcedCullingState() override;
	unsigned int SendEvent( const std::wstring& name, bool bypassPrefix = false ) override;
	int SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos ) override;
	void SetName( const std::string& name ) override;
	void SetPrefix( const std::wstring& prefix ) override;
	bool SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState ) override;
	bool SetRTPC( const std::wstring& rtpcName, float rtpcValue ) override;
	bool SetAttenuationScalingFactor( const float scalingFactor ) override;
	
	void SetVisibility( bool isVisible ) override;
	std::string GetName() override;
	void Unmute() override;
	// Debug
	virtual	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	virtual	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;
protected:
	AudEmitter( AkGameObjectID gameObjID, IRoot* lockobj = NULL );
	// Properties used for normalizing attenuation scaling for this audio emitter.
	bool m_normalizeAttenuationScaling;
	float m_minNormalizedValue;
	float m_maxNormalizedValue;
	float m_minNormalizedScalingFactor;
	float m_maxNormalizedScalingFactor;

	// Debug properties.
	Vector3 m_debugPosition;
	Vector3 m_debugFront;
	Color m_debugColor;
	Color m_simulationColor;
	float m_simulationRadius;
};

TYPEDEF_BLUECLASS( AudEmitter );

#endif