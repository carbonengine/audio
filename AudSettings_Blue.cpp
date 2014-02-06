#include "StdAfx.h"
#include "AudSettings.h"

BLUE_DEFINE_ABSTRACT( AudSettings );

std::string AudSettings::GetSettingReprString( const AudSettings::Setting* s )
{
	char buff[ 512 ];
	int len = _countof(buff);
	switch( s->m_type )
	{
	case Be::LONG:
		sprintf_s( buff, len, "%d", *(int*)s->m_var );
		break;
	case Be::BOOL:
		sprintf_s( buff, len, *(bool*)s->m_var ? "True" : "False" );
		break;
	case Be::FLOAT:
		{
			double d = *(float*)s->m_var;
			sprintf_s( buff, len, "%f", d );
		}
		break;
	case Be::DOUBLE:
		sprintf_s( buff, len, "%f", *(double*)s->m_var );
		break;
	case Be::SHORT:
		{
			int d = *(short*)s->m_var;
			sprintf_s( buff, len, "%d", d );
		}
		break;
	default:
		sprintf_s( buff, len, "NOT IMPLEMENTED YET! DO IT!" );
		break;
	}

	return buff;
}

PyObject* PyRepr( PyObject* self, PyObject* args )
{
	AudSettings* pThis = BluePythonCast<AudSettings*>(self);
	// the cast above will always succeed

	std::string repr = pThis->GetReprString();
	return PyString_FromString( repr.c_str() );
}

static PyObject* PyGetValue( PyObject* self, PyObject* args )
{
	AudSettings* pThis = BluePythonCast<AudSettings*>(self);
	// the cast above will always succeed

    const char* key;
	if( !PyArg_ParseTuple( args, "s", &key ) )
	{
		PyErr_SetString( PyExc_TypeError, "Function accepts one string argument!" );
		return NULL;
	}

	AudSettings::Setting* s = pThis->FindSetting( key );
	if( !s )
	{
		PyErr_SetString( PyExc_LookupError, "Setting name not registered!" );
		return NULL;
	}

	// Construct mostly bogus Be::VarEntry to use the BluePython converters.
	Be::VarEntry entry;
	entry.mType = s->m_type;
	entry.mSize = s->m_size;

	return BlueConvertValueToPython( &entry, s->m_var );
}

static PyObject* PySetValue( PyObject* self, PyObject* args )
{
	AudSettings* pThis = BluePythonCast<AudSettings*>(self);
	// the cast above will always succeed

    const char* key;
	PyObject* value;
	if( !PyArg_ParseTuple( args, "sO", &key, &value ) )
	{
		PyErr_SetString( PyExc_TypeError, "Function accepts key, value where key is a string!" );
		return NULL;
	}

	AudSettings::Setting* s = pThis->FindSetting( key );
	if( !s )
	{
		PyErr_SetString( PyExc_LookupError, "Setting name not registered!" );
		return NULL;
	}

	// Construct mostly bogus Be::VarEntry to use the BluePython converters.
	Be::VarEntry entry;
	entry.mType = s->m_type;
	entry.mSize = s->m_size;
	entry.mName = key;

	bool ok = BlueConvertValueFromPython( &entry, s->m_var, value );
	if( !ok )
	{
		return NULL;
	}

	Py_RETURN_NONE;
}


const Be::ClassInfo* AudSettings::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudSettings, "Encapsulates settings for Audio" )
		MAP_INTERFACE(AudSettings)

		MAP_METHOD( 
			"GetValue", 
			PyGetValue, 
			"Returns a copy of the value assigned to the string key passed in" 
			)
		MAP_METHOD( 
			"SetValue", 
			PySetValue, 
			"Sets the string key to the value passed in" 
			)
		MAP_METHOD(
			"__repr__",
			PyRepr,
			"Returns a string representation for the object"
			)
	EXPOSURE_END()
}
