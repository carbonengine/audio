#include "stdafx.h"

#include <sstream>

#include "AudListener.h"
#include "Vector3.h"
#include "Utilities.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>


AudListener::AudListener( IRoot* lockobj ) :
	m_ID(0)
{
}

AudListener::~AudListener()
{
	AK::SoundEngine::RemoveDefaultListener(m_ID);
	AK::SoundEngine::UnregisterGameObj(m_ID);
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
		listenerRH.Set( MakeAkVector(pos), MakeAkVector(front), MakeAkVector(top) );
		// all vectors come in RH, but WWISE is LH, so convert
		RH2LH::convertListener( &listenerLH, &listenerRH );

		AK::SoundEngine::SetPosition(m_ID, listenerLH);
	}
	return AK_Success;
}

void AudListener::CreateWwiseObject()
{
	if( g_audioInitialized )
	{
		auto userFacingID = m_ID - START_LISTENER_GAME_OBJ_COUNT;

		std::ostringstream listenerName;
		listenerName << "Listener_" << userFacingID;

		AK::SoundEngine::RegisterGameObj(m_ID, listenerName.str().c_str());
		AK::SoundEngine::AddDefaultListener(m_ID);
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