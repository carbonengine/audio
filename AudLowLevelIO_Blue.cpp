#include "stdafx.h"

#include "AudLowLevelIO.h"

BLUE_DEFINE( AudLowLevelIO );

static PyObject* Py__init__( PyObject* self, PyObject* args )
{
	const wchar_t* pathBuff = NULL;
	const wchar_t* langBuff = NULL;
	AudLowLevelIO* pThis = BluePythonCast<AudLowLevelIO*>( self );

	PyArg_ParseTuple( args, "u|u", &pathBuff, &langBuff );

	std::wstring path( pathBuff );
	std::wstring lang;
	if( langBuff )
	{
		lang = langBuff;
	}
	else
	{
		lang = L"";
	}
	pThis->Initialize( path, lang );

	Py_RETURN_NONE;
}

const Be::ClassInfo* AudLowLevelIO::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudLowLevelIO, "Wrapper for Wwises' low level io object." )
		MAP_INTERFACE( IAudLowLevelIO )
		MAP_INTERFACE( AudLowLevelIO )

		MAP_METHOD( "__init__", Py__init__, 
					"Description:\n"
					"\tConstructor. Takes 2 unicode strings as an arguments, a base path and language to use.\n"
					"Signature:\n"
					"\t__init__( path, language = u\"English(US)/\") - new instance\n"
					"Parameters:\n"
					"\tpath -- Base path from where all SoundBanks are loaded from. UNICODE!\n"
					"\tlang -- Addition to the basepath for language specific voices. UNICODE!\n"
				  )
		MAP_METHOD_AND_WRAP( "GetSelectedLangugePath", GetSelectedLangugePath,
							"Description:\n"
							"\tGets the selected language path.\n"
							"Signature:\n"
							"\tGetSelectedLangugePath(  ) -> String\n"
							"Parameters:\n"
							"\tNone"
							)
	EXPOSURE_END()	
}