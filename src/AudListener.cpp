#include "stdafx.h"
#include "AudListener.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "Vector3.h"
#include "Utilities.h"


AudListener::AudListener( IRoot* lockobj ) : AudGameObjResource( LISTENER_GAME_OBJ_ID, lockobj )
{
	m_name = "Listener";
	m_additionalCullingWeight = std::numeric_limits<float>::max();
}

AudListener::~AudListener()
{
	AK::SoundEngine::RemoveDefaultListener(m_ID);
	AK::SoundEngine::UnregisterGameObj(m_ID);
}

void AudListener::RegisterWwiseObject()
{
	if( g_audioInitialized && m_gameObjRegistered == false )
	{
		AK::SoundEngine::RegisterGameObj(m_ID, m_name.c_str());
		AK::SoundEngine::AddDefaultListener(m_ID);
		m_gameObjRegistered = true;
	}
}