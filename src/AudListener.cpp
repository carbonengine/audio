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
	if( g_audioManager != nullptr && g_audioManager->IsSpatialAudioInitialized() )
	{
		AK::SpatialAudio::UnregisterListener( m_ID );
	}
	AK::SoundEngine::RemoveDefaultListener( m_ID );
	AK::SoundEngine::UnregisterGameObj( m_ID );
	if( g_audioManager != nullptr )
	{
		g_audioManager->GetRoomManager().ForgetGameObject( m_ID );
	}
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
			RegisterWithSpatialAudio();
			// Wwise puts a new game object at its origin, which is only the player while the offset is zero.
			ResendPlacement();
		}
	}
	else
	{
		CCP_LOGERR( "Audio listener was requested to be created before audio was enabled! Audio will be silent because of this. "
					"Try to change where audio is enabled or create the listener again." );
	}
}

void AudListener::RegisterWithSpatialAudio()
{
	if( m_gameObjRegistered && g_audioManager != nullptr && g_audioManager->IsSpatialAudioInitialized() )
	{
		AK::SpatialAudio::RegisterListener( m_ID );
	}
}

int AudListener::SetPlacementFromParent( const Vector3& front, const Vector3& top, const Vector3& position )
{
	if( g_audioManager != nullptr && g_audioManager->GetState() != AudioState::Uninitialized )
	{
		m_position = position;
		// Kept so the listener can be sent again when the scene-to-Wwise offset moves.
		m_effectiveOrientation = Orthonormalize( front, top );
		if( m_gameObjRegistered )
		{
			SendPlacementToWwise( m_effectiveOrientation.front, m_effectiveOrientation.top, position );
			g_audioManager->GetRoomManager().UpdateGameObjectPosition( m_ID, position );
		}
	}
	return AK_Success;
}

void AudListener::ConvertPlacementToWwise( AkSoundPosition& lh, const AkSoundPosition& rh ) const
{
	RH2LH::convertListener( &lh, &rh );
}
