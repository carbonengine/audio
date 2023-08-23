#include "AudioCurveSetDriver.h"


BLUE_DEFINE( AudioCurveSetDriver );
BLUE_DEFINE_INTERFACE( ICurveSetDriver );
BLUE_DEFINE_INTERFACE( ITriScalarFunction );

const Be::ClassInfo* AudioCurveSetDriver::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudioCurveSetDriver, "" )
		MAP_INTERFACE( ICurveSetDriver )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"Name of this curve set driver.",
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		(
			"audioParameterName",
			m_audioParameterName,
			"The audio parameter whose value you want to drive the curve set. NOTE: Only works with global audio parameters!",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY 
		)
		MAP_ATTRIBUTE
		(
			"audioParameterValue",
			m_audioParameterValue,
			"The current value of the given audio parameter.",
			Be::READ 
		)
		MAP_PROPERTY_READONLY
		(
			"isValid",
			IsValid,
			"If not valid the fallback curve will be used if it is defined. This driver is considered valid if audio is enabled and the given audio parameter exists." 
		)
		MAP_ATTRIBUTE
		(
			"fallbackCurve",
			m_fallbackCurve,
			"A fallback curve to use for when audio is disabled or the given audio parameter does not exist.",
			Be::READWRITE | Be::PERSIST 
		)

	EXPOSURE_END()
}