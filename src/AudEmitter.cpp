#include "stdafx.h"
#include "AudEmitter.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

AudEmitter::AudEmitter( IRoot* lockobj ) : AudGameObjResource( lockobj ), PARENTLOCK( m_position )
{
}

AudEmitter::~AudEmitter()
{
}

int AudEmitter::SetPosition( const Vector3& front, const Vector3& pos )
{
	m_position.m_value.Orientation = front;
	m_position.m_value.Position = pos;
	return SetPositionHelper( m_position.m_value );
}

//----------------------------------
// Blue Interfaces
//----------------------------------
void AudEmitter::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	AkSoundPosition tmp;
	tmp.Orientation = front;
	tmp.Position = pos;
	SetPositionHelper( tmp );	//g_audioInitialized is checked in SetPositionHelper
}