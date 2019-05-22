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

int AudEmitter::SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	return SetPositionHelper( front, top, pos );
}

//----------------------------------
// Blue Interfaces
//----------------------------------
void AudEmitter::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	SetPositionHelper( front, top, pos );	//g_audioInitialized is checked in SetPositionHelper
}