#include "stdafx.h"

#include "AudManager.h"
#include "AudEmitterMulti.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>				// Sound Engine
#include "CCPAudioStream/include/CCPAudioStreamSourceFactory.h"

#include <vector>
#include <algorithm>

BLUE_DEFINE_ABSTRACT( AudManager );

const Be::ClassInfo* AudManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudManager, "Manages startup and shutdown of Wwise, don't use unless clever!" )
		MAP_INTERFACE( AudManager )

		MAP_ATTRIBUTE( "config", m_initConfig, "Configuration struct used in the initialization of audio2.", Be::READWRITE )
		MAP_ATTRIBUTE( "useDoppler", m_useDoppler, "A flag for deciding if we use the doppler calculations.", Be::READWRITE )

		MAP_METHOD_AND_WRAP( "SetEnabled", SetEnabled,
							 "Description:\n"
							 "\tToggles Wwise on and off.\n"
							 "Signature:\n"
							 "\tSetEnabled( newStatus ) -> None\n"
							 "Parameters:\n"
							 "\tnewStatus -- Boolean that dictates the status of Wwise."
						   )
		MAP_METHOD_AND_WRAP( "LoadBank", LoadBank,
							 "Description:\n"
							 "\tLoads a sound bank from disk.\n"
							 "Signature:\n"
							 "\tLoadBank( name ) -> None\n"
							 "Parameters:\n"
							 "\tname -- Name of the soundbank to load. UNICODE!"
						   )
		MAP_METHOD_AND_WRAP( "UnloadBank", UnloadBank, 
							 "Description:\n"
							 "\tUnloads a sound bank.\n"
							 "Signature:\n"
							 "\tUnloadBank( name ) -> None\n"
							 "Parameters:\n"
							 "\tname -- Name of the soundbank to unload. UNICODE!"
						   )
		MAP_METHOD_AND_WRAP( "ClearBanks", ClearBanks, 
							 "Description:\n"
							 "\tUnloads all currently loaded banks in Wwise.\n"
							 "Signature:\n"
							 "\tClearBanks() -> None\n"
						   )
		MAP_METHOD_AND_WRAP( "GetLoadedSoundBanks", GetLoadedSoundBanks, 
							 "Description:\n"
							 "\tGets a list of loaded sound banks.\n"
							 "Signature:\n"
							 "\tGetLoadedSoundBanks(  ) -> List\n"
							 "Parameters:\n"
							 "\tNone"
						   )
		MAP_METHOD_AND_WRAP( "GetEmitterForEventName", 
							GetEmitterForEventName, 
							"Description:\n"
							"\tGets the AudEmitterMulti for a given event name if it exists.\n"
							"\tIf it does not exist it will create a new one for the given event name.\n"
							"Signature:\n"
							"\tGetEmitterForEventName( eventName ) -> AudEmitterMulti or Raise a RuntimeError\n"
							"Parameters:\n"
							"\teventName -- Name (unicode) of the event you wish to get an emitter for."
							)
		MAP_METHOD_AND_WRAP( "RegisterDebugEventCallback",
							RegisterDebugEventCallback,
							"Registers a python function be called when an audio event is sent to the sound engine\n"
							":param callback: An instance of a python function to be executed"
						   )		
		MAP_METHOD_AND_WRAP( "RegisterDebugSwitchCallback",
							RegisterDebugSwitchCallback,
							"Registers a python function be called when an audio switch is set in the sound engine\n"
							":param callback: An instance of a python function to be executed"
						   )		
		MAP_METHOD_AND_WRAP( "SetApplicationName",
							SetApplicationName,
							"Sets the application name to be used when remote debugging with Wwise."
							":param applicationName: A string defining the name of the application audio2 is running inside."
						   )
	EXPOSURE_END()
}

void AudManager::RemovePythonReference()
{
	PyObject* tmp = g_audioManagerWrapper;
	g_audioManagerWrapper = NULL;
	Py_XDECREF( tmp );
}

// NOTE: This is NOT a memory leak, we explicitly delete them in BlueClientStop in audio2.cpp
AudManager* g_audioManager = NULL;
BluePythonObject* g_audioManagerWrapper = NULL;

static PyObject* PyGetAudManager( PyObject* self, PyObject* args )
{
	if( !g_audioManagerWrapper )
	{
		//on-demand creation of audiomanager
		if (!g_audioManager)
		{
			//this is how you manually created RootRefLock<> objects without going through pyos
			g_audioManager = new OAudManager;
			if (!g_audioManager)
			{
				PyOS->PyError();
				return 0;
			}
		}
		g_audioManagerWrapper = PyOS->WrapBlueObject( g_audioManager );
	}
	
	Py_INCREF( g_audioManagerWrapper );
	return g_audioManagerWrapper;
}
MAP_FUNCTION( "GetManager", PyGetAudManager, "Return an AudManager instance." );

static PyObject* PySetGlobalRTPC( PyObject* module, PyObject* args )
{
	if( g_audioInitialized )
	{
		//local declarations and arg parsing
		float value;
		const wchar_t* buffer;
		PyArg_ParseTuple( args, "uf", &buffer, &value );
		std::wstring name( buffer );

		AK::SoundEngine::SetRTPCValue( name.c_str(), value );
	}

	//Return
	Py_RETURN_NONE;
}
MAP_FUNCTION( "SetGlobalRTPC", PySetGlobalRTPC, "Sets RTPC values that are not associated with a specific AudEmitter." );

static PyObject* PySetState( PyObject* module, PyObject* args )
{
	if( g_audioInitialized )
	{
		//local declarations and arg parsing
		const wchar_t* groupBuf, *stateBuf;
		PyArg_ParseTuple( args, "uu", &groupBuf, &stateBuf );
		std::wstring groupName( groupBuf );
		std::wstring stateName( stateBuf );

		AK::SoundEngine::SetState( groupName.c_str(), stateName.c_str() );
	}

	//Return
	Py_RETURN_NONE;
}
MAP_FUNCTION( "SetState", PySetState, "Sets the given state in WWise runtime, which is applied globally." );

static PyObject* PyGetDirectSoundPtr( PyObject* module, PyObject* args )
{
	return PyLong_FromVoidPtr( (void *) &SetAudioStreamData );
}
MAP_FUNCTION( "GetDirectSoundPtr", PyGetDirectSoundPtr, "Get a pointer to the direct sound interface." );

static PyObject* PyGetStreamPositionPtr( PyObject* module, PyObject* args )
{
	return PyLong_FromVoidPtr( (void *) &GetAudioStreamPosition );
}
MAP_FUNCTION( 
	"GetStreamPositionPtr", 
	PyGetStreamPositionPtr, 
	"Get a pointer to the function returning the number of samples consumed by WWise for the given stream" );


static PyObject* PyStopAll( PyObject* self, PyObject* args )
{
	AK::SoundEngine::StopAll();
	Py_RETURN_NONE;
}
MAP_FUNCTION( "StopAll", PyStopAll, "Stops all sounds currently playing.");