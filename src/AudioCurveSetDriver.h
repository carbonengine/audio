#pragma once

#include <ICurveSetDriver.h>


BLUE_DECLARE( AudioCurveSetDriver );
BLUE_DECLARE_INTERFACE( ITriScalarFunction );

BLUE_CLASS( AudioCurveSetDriver ) :
	public ICurveSetDriver,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();
	AudioCurveSetDriver( IRoot* lockob = NULL );
	~AudioCurveSetDriver();

	//IInitialize
	bool Initialize() override;

	// ICurveSetDriver
	double GetCurveSetTime( double time ) override;

	bool IsValid() const;
	const std::wstring& GetAudioParameterName();
	void SetAudioParameterName( const std::wstring& audioParameterName );
protected:
	bool m_audioParameterExists;
	float m_audioParameterValue;
	std::wstring m_audioParameterName;
	std::wstring m_name;

	ITriScalarFunctionPtr m_fallbackCurve;
};

TYPEDEF_BLUECLASS( AudioCurveSetDriver );
