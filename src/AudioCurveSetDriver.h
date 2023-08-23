#pragma once

#include <ICurveSetDriver.h>

BLUE_DECLARE( AudioCurveSetDriver );
BLUE_DECLARE_INTERFACE( ITriScalarFunction );

BLUE_CLASS( AudioCurveSetDriver ) :
	public ICurveSetDriver,
	public INotify
{
public:
	EXPOSE_TO_BLUE();
	AudioCurveSetDriver( IRoot* lockob = NULL );
	~AudioCurveSetDriver();

	// ICurveSetDriver
	double GetCurveSetTime( double time ) override;

	// INotify
	bool OnModified( Be::Var* val ) override;

	float GetAudioParameterValue( const std::wstring audioParameterName );
	bool IsValid() const;

protected:
	bool m_audioParameterExists;
	float m_audioParameterValue;
	std::wstring m_audioParameterName;
	std::wstring m_name;

	ITriScalarFunctionPtr m_fallbackCurve;
};

TYPEDEF_BLUECLASS( AudioCurveSetDriver );
