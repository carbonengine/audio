#include <ITriFunction.h>

#include "Audio2.h"
#include "AudioCurveSetDriver.h"



AudioCurveSetDriver::AudioCurveSetDriver( IRoot* lockobj ) :
	m_name( L"" ),
	m_audioParameterName( L"" ),
	m_audioParameterValue( 0.0f ),
	m_audioParameterExists( false ),
	m_fallbackCurve()
{
}

AudioCurveSetDriver::~AudioCurveSetDriver()
{
}


bool AudioCurveSetDriver::OnModified( Be::Var* val )
{
	GetAudioParameterValue( m_audioParameterName );
	return true;
}

double AudioCurveSetDriver::GetCurveSetTime( double time )
{
	if( IsValid() )
	{
		return GetAudioParameterValue( m_audioParameterName );
	}
	else
	{
		if( m_fallbackCurve != nullptr )
		{
			return m_fallbackCurve->GetValueAt( time );
		}
	}
	return 0.0;
}

float AudioCurveSetDriver::GetAudioParameterValue( const std::wstring audioParameterName ) 
{
	if (g_audioEnabled)
	{
		AK::SoundEngine::Query::RTPCValue_type rtpcValueType = AK::SoundEngine::Query::RTPCValue_type::RTPCValue_Global;
		AKRESULT result = AK::SoundEngine::Query::GetRTPCValue(audioParameterName.c_str(), AK_INVALID_GAME_OBJECT, AK_INVALID_PLAYING_ID, m_audioParameterValue, rtpcValueType);
		if (result == AK_IDNotFound)
		{
			m_audioParameterExists = false;
			return 0.0f;
		}
		else
		{
			m_audioParameterExists = true;
		}
	}

	return m_audioParameterValue;
}

bool AudioCurveSetDriver::IsValid() const
{
	return g_audioEnabled && m_audioParameterExists;
}
