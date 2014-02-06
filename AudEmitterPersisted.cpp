#include "stdafx.h"

#include "AudEmitterPersisted.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

AudEmitterPersisted::AudEmitterPersisted( IRoot* lockobj ) : AudEmitter( lockobj ), m_playEvent(L""), m_playOnLoad( true ) 
{}

AudEmitterPersisted::~AudEmitterPersisted()
{}


bool AudEmitterPersisted::Initialize()
{
	AudEmitter::Initialize();
	if( m_playOnLoad )
	{
		SendEvent( m_playEvent );	//SendEvent in AudEmitter checks for g_audioEnabled
	}
	return true;
}

bool AudEmitterPersisted::OnModified( Be::Var* value )
{
	if ( IsMatch( value, m_playEvent ) )
	{
		if( g_audioInitialized )
		{
			AK::SoundEngine::StopAll( m_ID );
			SendEvent( m_playEvent );
		}
	}
	return true;
}
