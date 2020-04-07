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
	virtual void UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos );
	void Py__init__( const std::string& name );

	//ITr2AudEmitter
	void SetName( const std::string name );
	virtual int SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos );
	unsigned int SendEvent( const std::wstring& name );

	// Debug
	virtual	void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

protected:
	PAudPosition m_position;
	Vector3 m_debugPosition;
	Color m_debugColor;
};

TYPEDEF_BLUECLASS( AudEmitter );

#endif