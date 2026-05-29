#include "stdafx.h"
#include "AudListener.h"

#include "AudManager.h"
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
	if( g_audioManager != nullptr && g_audioManager->GetState() == AudioState::Enabled )
	{
		if( m_gameObjRegistered == false )
		{
			AK::SoundEngine::RegisterGameObj(m_ID, m_name.c_str());
			AK::SoundEngine::AddDefaultListener(m_ID);
			m_gameObjRegistered = true;
		}
	}
	else
	{
		CCP_LOGERR( "Audio listener was requested to be created before audio was enabled! Audio will be silent because of this. "
					"Try to change where audio is enabled or create the listener again." );
	}
}

int AudListener::SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position )
{
	if( g_audioManager != nullptr && g_audioManager->GetState() != AudioState::Uninitialized )
	{
		m_position = position;
		if( m_gameObjRegistered )
		{
			AkSoundPosition tmp;
			Vector3 correctFront = Normalize( front );
			Vector3 correctUp = Normalize( top );
			correctUp = Normalize( Cross( Cross( correctFront, correctUp ), correctFront ) );
			tmp.Set( MakeAkVector(position), MakeAkVector(correctFront), MakeAkVector(correctUp) );

			// all vectors come in RH, but WWISE is LH, so convert
			AkSoundPosition soundPosLH;
			RH2LH::convertListener( &soundPosLH, &tmp);

			AKRESULT result = AK::SoundEngine::SetPosition( m_ID, soundPosLH );
		}
	}
	return AK_Success;
}