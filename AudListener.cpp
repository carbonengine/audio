#include "stdafx.h"

#include "AudListener.h"
#include "Vector3.h"
#include "Utilities.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>


AudListener::AudListener( IRoot* lockobj ) :
	m_ID( 0 )
{
}

AudListener::~AudListener()
{
	AK::SoundEngine::RemoveDefaultListener(m_ID);
}

void AudListener::Initialize()
{
	CreateWwiseObject();
}

//-----------------------------------------------------------------------------
// IBluePlacementObserver
//-----------------------------------------------------------------------------
void AudListener::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	SetPosition( front, top, pos );
}

int AudListener::SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	if( g_audioInitialized )
	{
		AkListenerPosition listenerRH, listenerLH;
		listenerRH.Set(pos, front, top);
		// all vectors come in RH, but WWISE is LH, so convert
		RH2LH::convertListener( &listenerLH, &listenerRH );

		return AK::SoundEngine::SetPosition( m_ID, listenerLH );
	}
	return AK_Success;
}

void AudListener::CreateWwiseObject()
{
	if (g_audioInitialized)
	{
		// Register the main listener.
		AK::SoundEngine::RegisterGameObj( m_ID, "Listener" );

		// Set one listener as the default.
		AK::SoundEngine::SetDefaultListeners( &m_ID, 1 );
	}
}

void AudListener::LogInfo()
{
	//if( g_audioInitialized )
	//{
	//Currently does nothing
	//AK::SoundEngine::Query::GetListenerPosition( m_ID, tmpListenerPosition );
	//}
}