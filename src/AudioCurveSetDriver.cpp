// Copyright © 2023 CCP ehf.

#include <ITriFunction.h>

#include "Audio2.h"
#include "AudioCurveSetDriver.h"
#include "AudManager.h"


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
	g_audioManager->UnregisterParameter(m_audioParameterName);
}

bool AudioCurveSetDriver::Initialize()
{
	if( m_audioParameterName != L"" )
	{
		g_audioManager->RegisterParameter( m_audioParameterName );
	}
	
	return true;
}

double AudioCurveSetDriver::GetCurveSetTime( double time )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	const MonitoredParameterInfo* parameterInfo = g_audioManager->GetParameterInfo( m_audioParameterName );
	if( parameterInfo != nullptr )
	{
		m_audioParameterValue = parameterInfo->parameterValue;
		m_audioParameterExists = parameterInfo->parameterExists;
	}

	if( !IsValid() )
	{
		if( m_fallbackCurve != nullptr )
		{
			return m_fallbackCurve->GetValueAt( time );
		}
	}
	return m_audioParameterValue;
}

bool AudioCurveSetDriver::IsValid() const
{
	return g_audioEnabled && m_audioParameterName != L"" && m_audioParameterExists;
}

const std::wstring& AudioCurveSetDriver::GetAudioParameterName()
{
	return m_audioParameterName;
}

void AudioCurveSetDriver::SetAudioParameterName( const std::wstring& audioParameterName )
{
	g_audioManager->UnregisterParameter( m_audioParameterName );
	g_audioManager->RegisterParameter( audioParameterName );
	m_audioParameterName = audioParameterName;
}
