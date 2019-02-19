////////////////////////////////////////////////////////////
//
//    Creator:   Eric Nielsen
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "stdafx.h"

#include "AudioEmitter.h"

#include "Vector3.h"


AudioEmitter::AudioEmitter( IRoot* lockobj ) : AudGameObjResource( lockobj )
{
}


AudioEmitter::~AudioEmitter()
{
}

void AudioEmitter::Py__init__()
{
	AudGameObjResource::Initialize( "", L"" );
}

void AudioEmitter::SendEvent()
{
	AudGameObjResource::SendEvent( m_eventName.c_str() );
}

void AudioEmitter::SetTransform( const Matrix& worldTransform )
{
	Vector3 front, up, position;
	position = worldTransform.GetTranslation();
	front = TransformNormal( Vector3( 0.0f, 0.0f, -1.0f ), worldTransform );
	up = TransformNormal( Vector3( 0.0f, 1.0f, 0.0f ), worldTransform );

	m_position.Set( MakeAkVector( position ), MakeAkVector( front ), MakeAkVector( up ) );
	SetPositionHelper( m_position );
}
