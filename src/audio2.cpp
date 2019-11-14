// audio2.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include <Blue/Include/Blue.cxx>
#include <BlueExposure/include/InterfaceDefinitions.cxx>

#include "LogBridge.h"

BLUE_DEFINE_INTERFACE( IBluePlacementObserver );
BLUE_DEFINE_INTERFACE( IBlueEventListener );

#include "AudManager.h"
#include "AudResource.h"
#include "AudSettings.h"

#ifndef BLUE_DLL_NAME
#error Please add BLUE_DLL_NAME=<PythonModuleName> to compiler preprocessor definitions (/D)
#endif

const char* BLUE_DLL_NAME_STR = CCP_STRINGIZE( BLUE_DLL_NAME );
const char* g_moduleName = "audio2";

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
HINSTANCE gInstance = NULL;
bool g_audioEnabled = false;
bool g_audioInitialized = false;
bool g_debugDisplayAllEmitters = false;

IBlueCallbackManPtr g_mainThreadQueue;

static PyObject* PyGetRegisteredEnums( PyObject* self, PyObject* args )
{
	PyObject* l = PyList_New( 0 );
 
	EnumRegsMap& regs = BlueRegistration::GetEnumRegs();
	for( EnumRegsMap::const_iterator i = regs.begin(); i != regs.end(); ++i )
	{
		PyObject* enumName = NULL;
		enumName = PyString_FromStringAndSize( i->first.c_str(), i->first.size() );
		PyList_Append( l, enumName );
		Py_XDECREF( enumName );
	}
 
	return l;
}
MAP_FUNCTION( "GetRegisteredEnums", PyGetRegisteredEnums, "GetRegisteredEnums()\nGet a list of registered enum values for the module" );
 
static PyObject* PyGetRegisteredEnumValues( PyObject* self, PyObject* args )
{
	const char* enumName;
 
	if (!PyArg_ParseTuple(args, "s", &enumName))
		return NULL;
 
	const std::string enumNameStr = std::string( enumName );
 
	GetEnumValuesFunctionTypePtr fn = EnumTypeRegistration::GetTypeValuesGetter( enumNameStr );
 
	// There was no registered enum of this name
	if( fn == NULL )
	{
		Py_RETURN_NONE;
	}
 
	EnumValues& vals = fn();
 
	PyObject* l = PyList_New( 0 );
	for( EnumValues::const_iterator i = vals.begin(); i != vals.end(); ++i )
	{
		PyObject* valueName = PyString_FromString( i->mKey );
		PyObject* val = PyLong_FromLong( i->mValue.mLong );
		PyObject* docString = PyString_FromString( i->mDescription );
 
		PyObject* tupleObject = Py_BuildValue("(OOO)", valueName, val, docString );
 
		Py_XDECREF( valueName );
		Py_XDECREF( val );
		Py_XDECREF( docString );
 
		PyList_Append( l, tupleObject );
 
		Py_XDECREF( tupleObject );
 
	}
	return l;
 
}
MAP_FUNCTION( "GetRegisteredEnumValues", PyGetRegisteredEnumValues, "GetRegisteredEnumValues( enumName )\nGet a list of tuples containing the value, name and docstring of the values of the enum " );

//-----------------------------------------------------------------------------
// Global DLL init function.
//-----------------------------------------------------------------------------
static void StartDLL( HINSTANCE instance )
{	
	WwiseLogServerBridgeInit( AK::Monitor::ErrorLevel_All );

	BeClasses->CreateInstanceFromName( "BlueCallbackMan", BlueInterfaceIID<IBlueCallbackMan>(), (void**)&g_mainThreadQueue );

	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
	
	PyObject* module = Py_InitModule( ( char* )BLUE_DLL_NAME_STR, NULL );
	
	BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs() );

	PyModule_AddObject( module, "settings", PyOS->WrapBlueObject(&AudManager::GetSettings()) );
}

//-----------------------------------------------------------------------------
// DLL entry(main) function
//-----------------------------------------------------------------------------
BOOL APIENTRY DllMain( HINSTANCE instance, DWORD  reason, LPVOID )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{	
		gInstance = instance;
		DisableThreadLibraryCalls( instance) ;
	}
	else if ( reason == DLL_PROCESS_DETACH )
	{
	}

    return TRUE;
}

//-----------------------------------------------------------------------------
// init_audio2 - python dll module entry function
//-----------------------------------------------------------------------------
#define CONCAT( a, b ) MY_CONCAT( a, b )
#define MY_CONCAT( a, b ) a##b
//---------------------------------
extern "C" void __declspec(dllexport) CONCAT( init, BLUE_DLL_NAME )()
{
	StartDLL( gInstance );
}