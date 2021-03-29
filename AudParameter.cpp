#include "stdafx.h"

#include "AudParameter.h"
#include "AudManager.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

AudParameter::AudParameter( IRoot* lockobj ) :
	m_ID( 0 ),
	m_name( L"" ),
	m_value( 0.0f )
{}

AudParameter::~AudParameter()
{}

bool AudParameter::OnModified( Be::Var* value )
{
	if ( ( Be::Var* )&m_value == value )
	{
		if( g_audioInitialized && m_ID )
		{
			AK::SoundEngine::SetRTPCValue( m_name.c_str(), m_value, m_ID );
			g_audioManager->LogSetRTPC( m_ID, m_name, m_value );
		}
	}
	return true;
}
