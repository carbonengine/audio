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
#if PY_MAJOR_VERSION >= 3
	return PyUnicode_FromString( g_wwiseVersion.c_str() );
#else
    return PyString_FromString( g_wwiseVersion.c_str() );
#endif
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
		PyObject* enumName = NULL;
#if PY_MAJOR_VERSION >= 3
		enumName = PyUnicode_FromStringAndSize( i->first.c_str(), i->first.size() );
#else
        enumName = PyString_FromStringAndSize( i->first.c_str(), i->first.size() );
#endif
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
#if PY_MAJOR_VERSION >= 3
		PyObject* valueName = PyUnicode_FromString( i->mKey );
		PyObject* docString = PyUnicode_FromString( i->mDescription );
#else
        PyObject* valueName = PyString_FromString( i->mKey );
        PyObject* docString = PyString_FromString( i->mDescription );
#endif
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
#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef ModuleDef =
{
	PyModuleDef_HEAD_INIT,
	CCP_STRINGIZE(CCP_CONCATENATE(_audio2, CCP_BUILD_FLAVOR)),
	"",
	-1,
	NULL
};
#endif

#if PY_MAJOR_VERSION >= 3
PyObject* StartDLL( /*HINSTANCE instance*/ )
#else
void StartDLL( /*HINSTANCE instance*/ )
#endif
{	
	BeClasses->CreateInstanceFromName( "BlueCallbackMan", BlueInterfaceIID<IBlueCallbackMan>(), (void**)&g_mainThreadQueue );

	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
	
#if PY_MAJOR_VERSION >= 3
	PyObject* module = PyModule_Create(&ModuleDef);
#else
    PyObject* module = Py_InitModule( CCP_STRINGIZE( CCP_CONCATENATE( _audio2, CCP_BUILD_FLAVOR ) ), NULL );
#endif
	
	BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs() );

#if PY_MAJOR_VERSION >= 3
	return module;
#endif
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
extern "C"
#ifdef _WIN32
__declspec( dllexport )
#else
__attribute__((visibility ("default")))
#endif
#if PY_MAJOR_VERSION >= 3
PyObject* CCP_CONCATENATE( PyInit__audio2, CCP_BUILD_FLAVOR )()
{
	return StartDLL();
}
#else
void CCP_CONCATENATE( init_audio2, CCP_BUILD_FLAVOR )()
{
    StartDLL();
}
#endif