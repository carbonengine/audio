// audio2.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "LogBridge.h"

BLUE_DEFINE_INTERFACE( IBluePlacementObserver );
BLUE_DEFINE_INTERFACE( IBlueEventListener );

#include "AudManager.h"
#include "AudResource.h"

const char* g_moduleName = "_audio2";

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
#ifdef _WIN32
HINSTANCE gInstance = NULL;
#endif // _WIN32

AudManager* g_audioManager = nullptr;

bool g_audioEnabled = false;
bool g_audioInitialized = false;
bool g_debugDisplayAllEmitters = false;

IBlueCallbackManPtr g_mainThreadQueue;

static PyObject* PyGetManager( PyObject* self, PyObject* args)
{
	if ( !g_audioManager )
	{
		g_audioManager = new OAudManager;
		if ( !g_audioManager )
		{
			PyOS->PyError();
			return 0;
		}
	}

	return PyOS->WrapBlueObject( g_audioManager );
}
MAP_FUNCTION( "GetOrCreateManager", PyGetManager, "Create a global audio manager instance if needed, otherwise return the existing one." );

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
static void StartDLL( /*HINSTANCE instance*/ )
{	
	WwiseLogServerBridgeInit( AK::Monitor::ErrorLevel_All );

	BeClasses->CreateInstanceFromName( "BlueCallbackMan", BlueInterfaceIID<IBlueCallbackMan>(), (void**)&g_mainThreadQueue );

	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
	
	PyObject* module = Py_InitModule( CCP_STRINGIZE( CCP_CONCATENATE( _audio2, CCP_BUILD_FLAVOR ) ), NULL );
	
	BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs() );
}

#ifdef _WIN32
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
#endif // _WIN32

//-----------------------------------------------------------------------------
// init_audio2 - python dll module entry function
//-----------------------------------------------------------------------------
extern "C" void
#ifdef _MSC_VER
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
CCP_CONCATENATE( init_audio2, CCP_BUILD_FLAVOR )()
{
    StartDLL();
}