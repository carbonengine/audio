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
		const Orientation corrected = Orthonormalize( front, top );
		{
			CcpAutoMutex lock( m_placementMutex );
			m_position = position;
			m_effectiveOrientation = corrected;
		}
		if( m_gameObjRegistered )
		{
			SendPlacement( corrected.front, corrected.top, position );
			g_audioManager->GetRoomManager().UpdateGameObjectPosition( m_ID, position );
		}
	}
	return AK_Success;
}

void AudListener::SendPlacement( const Vector3& front, const Vector3& top, const Vector3& position )
{
	CcpAutoMutex lock( m_placementMutex );

	AkSoundPosition tmp;
	tmp.Set( ToFramePosition( position ), MakeAkVector( front ), MakeAkVector( top ) );

	// all vectors come in RH, but WWISE is LH, so convert
	AkSoundPosition soundPosLH;
	RH2LH::convertListener( &soundPosLH, &tmp );

	AK::SoundEngine::SetPosition( m_ID, soundPosLH );
	const AkVector64 sent = soundPosLH.Position();
	m_sentWwisePosition = Vector3( static_cast<float>( sent.X ), static_cast<float>( sent.Y ), static_cast<float>( sent.Z ) );
}
