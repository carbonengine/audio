#include "StdAfx.h"
#include "AudConfig.h"

BLUE_DEFINE( AudConfig );
/*
static PyObject* PyGetAttribute( PyObject* self, PyObject* args )
{
	Py_RETURN_NONE;
}

static PyObject* PySetAttribute( PyObject* self, PyObject* args )
{
	Py_RETURN_NONE;
}
*/

const Be::ClassInfo* AudConfig::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudConfig, "Encapsulates initialisation configuration for audio2" )
		MAP_INTERFACE(AudConfig)

		MAP_ATTRIBUTE( "lowLevelIO", m_lowLevelIO, "Handle to Wwises' low level IO object.", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "numRefillsInVoice", m_platformInitSettings.uNumRefillsInVoice, "Number of refill buffers in the voice buffer.", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "dirty", m_dirty, "Determines if configuration has changed since audio2 was last inititalized", Be::READ )
        MAP_ATTRIBUTE( "asyncFileOpen", m_asyncFileOpen, "Determines whether to use blocking or non-blocking file opening. Must be set before the AudManager is created.", Be::READWRITE | Be::NOTIFY )
	EXPOSURE_END()
}