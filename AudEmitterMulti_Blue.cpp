#include "stdafx.h"

#include "AudEmitterMulti.h"
#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

BLUE_DEFINE( AudEmitterMulti );

void AudEmitterMulti::Py__init__( const std::string& name )
{
	m_name = name;
	Initialize();
}
const Be::ClassInfo* AudEmitterMulti::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitterMulti, "RAII wrapper for wwise gameobjects. Supports multiple locations." )

	MAP_METHOD_AND_WRAP( "__init__", Py__init__, "Ble")

		MAP_INTERFACE( AudGameObjResource )
		MAP_INTERFACE( AudEmitterMulti )

	EXPOSURE_CHAINTO( AudGameObjResource )
}