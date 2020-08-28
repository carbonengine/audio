#include "stdafx.h"

#include "AudManager.h"
#include "AudEmitterMulti.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>				// Sound Engine

#include <vector>
#include <algorithm>

BLUE_DEFINE( AudManager );

const Be::ClassInfo* AudManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudManager, "Manages startup and shutdown of Wwise, don't use unless clever!" )
		MAP_INTERFACE( AudManager )

		MAP_ATTRIBUTE( "useDoppler", m_useDoppler, "A flag for deciding if we use the doppler calculations.", Be::READWRITE )
		MAP_METHOD_AND_WRAP( "UpdateSettings", 
							 UpdateSettings,
							 "Update settings to be used when starting Wwise. Needs to be called before SetEnabled in order to apply."
							 ":settings: An AudSettings instance defining Wwise specific settings."
							 ":type settings: AudSettings"
						   )
		MAP_METHOD_AND_WRAP( "SetEnabled", 
							 SetEnabled,
							 "Toggles Wwise on and off."
							 ":param onoff: whether Wwise should be turned on or off."
							 ":type onoff: bool"
						   )
		MAP_METHOD_AND_WRAP( "LoadBank", 
			                  LoadBank,
							 "Loads a sound bank from disk."
							 ":param name: Name of the soundbank to load"
							 ":type name: unicode"
							 ":return: True or False depending on if the call to LoadBank failed or not."
						   )
		MAP_METHOD_AND_WRAP( "UnloadBank", 
			                 UnloadBank,
							 "Unloads a sound bank."
							 ":param name: Name of the soundbank to unload."
							 ":type name: unicode"
						   )
		MAP_METHOD_AND_WRAP( "ClearBanks", 
							 ClearBanks,
							 "Unloads all currently loaded banks in Wwise."
						   )
		MAP_METHOD_AND_WRAP( "GetLoadedSoundBanks", 
							 GetLoadedSoundBanks,
							 "Return a list of loaded sound banks."
						   )
		MAP_METHOD_AND_WRAP( "GetEmitterForEventName",
							 GetEmitterForEventName,
							"Gets the AudEmitterMulti for a given event name if it exists."
							"If it does not exist it will create a new one for the given event name."
							":param eventName: name of the event you wish to get an emitter for."
						    ":type eventName: unicode"
							)
		MAP_METHOD_AND_WRAP( "StopAll",
							 StopAll,
							 "Stops all sounds currently playing."
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
		MAP_METHOD_AND_WRAP("EnableDebugDisplayAllEmitters",
							EnableDebugDisplayAllEmitters,
							"Forces all AudEmitters to render their debug info on screen."
						   )
		MAP_METHOD_AND_WRAP("DisableDebugDisplayAllEmitters",
							DisableDebugDisplayAllEmitters,
							"Stops the displaying of all AudEmitter debug info on screen."
						   )
		MAP_METHOD_AND_WRAP("GetDebugDisplayAllEmitters",
							GetDebugDisplayAllEmitters,
							"Return the value of debugDisplayAllEmitters."
						   )
	EXPOSURE_END()
}


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
