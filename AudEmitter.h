/*
	*************************************************************************************

	AudEmitter.h

	Author:    Andri Mar
	Created:   November 2008
	OS:        Win32
	Project:   Audio2

	Description:

		An audio entity ingame. Wrapper for the GameObject concept in Wwise.


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUDEMITTER_H_
#define _AUDEMITTER_H_

#include "Audio2.h"
#include "AudGameObjResource.h"
#include "AudParameter.h"
#include "AudPosition.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>

#include "CcpMath/include/CcpMath.h"
#include "trinity/Include/ITr2DebugRenderer2.h"
#include "trinity/Audio/ITr2AudEmitter.h"

struct Vector3;

BLUE_CLASS( AudEmitter ) :
	public IBluePlacementObserver,
	public AudGameObjResource,
	public ITr2DebugRenderable,
	public ITr2AudEmitter
{
public:
	AudEmitter( IRoot* lockobj = NULL );
	virtual ~AudEmitter();

	EXPOSE_TO_BLUE();

	//--------------------------
	// Blue interfaces
	//--------------------------
	// IBluePlacementObserver
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos ) override;
	void Py__init__( const std::string& name );

	// IInitialized
	bool Initialize() override;

	//ITr2AudEmitter
	void Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position ) override;
	unsigned int SendEvent( const std::wstring& name, bool bypassPrefix = false ) override;
	int SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos ) override;
	void SetName( const std::string& name ) override;
	void SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState ) override;
	void SetRTPC( const std::wstring& rtpcName, float rtpcValue ) override;
	int SetAttenuationScalingFactor( const float scalingFactor ) override;
	std::string GetName() override;

	// Debug
	virtual	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	virtual	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	// AudEmitter
	bool StopEvent( const std::wstring& eventName ); // Stop all sounds associated with an event.

protected:
	Vector3 m_debugPosition;
	Vector3 m_debugFront;
	Color m_debugColor;
};

TYPEDEF_BLUECLASS( AudEmitter );

#endif