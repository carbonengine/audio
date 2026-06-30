// Copyright © 2014 CCP ehf.

// audio2.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "AudManager.h"
#include "AudStaticDataRepository.h"

BLUE_DEFINE_INTERFACE( IBluePlacementObserver );
BLUE_DEFINE_INTERFACE( IBlueEventListener );

const char* g_moduleName = "audio2";
const std::string g_wwiseVersion = std::to_string(AK_WWISESDK_VERSION_MAJOR) + "." + \
                                   std::to_string(AK_WWISESDK_VERSION_MINOR) + "." + \
								   std::to_string( AK_WWISESDK_VERSION_SUBMINOR ) + "." +  \
								   std::to_string( AK_WWISESDK_VERSION_BUILD );

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
#ifdef _WIN32
HINSTANCE gInstance = NULL;
#endif // _WIN32

AudManager* g_audioManager = nullptr;
AudStaticDataRepository* g_staticDataRepository = nullptr;

bool g_audioEnabled = false;
bool g_audioInitialized = false;
bool g_shuttingDown = false;
bool g_debugDisplayAllEmitters = false;
bool g_wwiseCommunicationEnabled = false;

IBlueCallbackManPtr g_mainThreadQueue;

static PyObject* PyGetWwiseCommuncationEnabled( PyObject* self, PyObject* args )
{
	if ( g_wwiseCommunicationEnabled )
	{
		return Py_True;	
	}
	return Py_False;
}
MAP_FUNCTION( "GetWwiseCommunicationEnabled", PyGetWwiseCommuncationEnabled, "Whether audio2 is able to communicate with the Wwise profiler or not." );

static PyObject* PyGetWwiseVersion( PyObject* self, PyObject* args )
{
	return PyUnicode_FromString( g_wwiseVersion.c_str() );
}
MAP_FUNCTION( "GetWwiseVersion", PyGetWwiseVersion, "The version of Wwise being used by audio2." );

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

static PyObject* PyGetStaticDataRepository( PyObject* self, PyObject* args)
{
	if ( !g_staticDataRepository )
	{
		g_staticDataRepository = new OAudStaticDataRepository;
		if ( !g_staticDataRepository)
		{
			PyOS->PyError();
			return 0;
		}
	}

	return PyOS->WrapBlueObject( g_staticDataRepository );
}
MAP_FUNCTION( "GetStaticDataRepository", PyGetStaticDataRepository, "Create a global static data repository." );

static PyObject* PyGetRegisteredEnums( PyObject* self, PyObject* args )
{
	PyObject* l = PyList_New( 0 );
 
	EnumRegsMap& regs = BlueRegistration::GetEnumRegs();
	for( EnumRegsMap::const_iterator i = regs.begin(); i != regs.end(); ++i )
	{
		PyObject* enumName = PyUnicode_FromStringAndSize( i->first.c_str(), i->first.size() );
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
		PyObject* valueName = PyUnicode_FromString( i->mKey );
		PyObject* docString = PyUnicode_FromString( i->mDescription );
		PyObject* val = PyLong_FromLong( i->mValue.mLong );
 
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
static struct PyModuleDef ModuleDef =
{
	PyModuleDef_HEAD_INIT,
	CCP_STRINGIZE(CCP_CONCATENATE(_audio2, CCP_BUILD_FLAVOR)),
	"",
	-1,
	NULL
};

PyObject* StartDLL( /*HINSTANCE instance*/ )
{
	BeClasses->CreateInstanceFromName( "BlueCallbackMan", BlueInterfaceIID<IBlueCallbackMan>(), (void**)&g_mainThreadQueue );

	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );

	PyObject* module = PyModule_Create(&ModuleDef);

	BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs() );

	return module;
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
// PyInit__audio2 - python dll module entry function
//-----------------------------------------------------------------------------
extern "C"
#ifdef _WIN32
__declspec( dllexport )
#else
__attribute__((visibility ("default")))
#endif
PyObject* CCP_CONCATENATE( PyInit__audio2, CCP_BUILD_FLAVOR )()
{
	return StartDLL();
}