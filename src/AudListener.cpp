// Copyright © 2014 CCP ehf.

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
	if( g_audioManager != nullptr && g_audioManager->GetSpatialAudioGeometryEnabled() )
	{
		AK::SpatialAudio::UnregisterListener( m_ID );
	}
	AK::SoundEngine::RemoveDefaultListener( m_ID );
	AK::SoundEngine::UnregisterGameObj( m_ID );
}

void AudListener::RegisterWwiseObject()
{
	if( g_audioManager != nullptr && g_audioManager->GetState() == AudioState::Enabled )
	{
		if( m_gameObjRegistered == false )
		{
			AK::SoundEngine::RegisterGameObj(m_ID, m_name.c_str());
			AK::SoundEngine::AddDefaultListener(m_ID);

			// Register listener for occlusion/diffraction processing
			if( g_audioManager != nullptr && g_audioManager->GetSpatialAudioGeometryEnabled() )
			{
				AK::SpatialAudio::RegisterListener( m_ID );
			}

			m_gameObjRegistered = true;
		}
	}
	else
	{
		CCP_LOGERR( "Audio listener was requested to be created before audio was enabled! Audio will be silent because of this. "
					"Try to change where audio is enabled or create the listener again." );
	}
}

int AudListener::SetPlacementFromParent( const Vector3& front, const Vector3& top, const Vector3& position )
{
	if( g_audioManager != nullptr && g_audioManager->GetState() != AudioState::Uninitialized )
	{
		m_position = position;
		if( m_gameObjRegistered )
		{
			const Orientation corrected = Orthonormalize( front, top );

			AkSoundPosition tmp;
			tmp.Set( MakeAkVector( position ), MakeAkVector( corrected.front ), MakeAkVector( corrected.top ) );

			// all vectors come in RH, but WWISE is LH, so convert
			AkSoundPosition soundPosLH;
			RH2LH::convertListener( &soundPosLH, &tmp );

			AK::SoundEngine::SetPosition( m_ID, soundPosLH );
		}
	}
	return AK_Success;
}
