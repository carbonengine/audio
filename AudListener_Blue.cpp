#include "stdafx.h"

#include "AudListener.h"
#include <vector>
#include "Vector3.h"

BLUE_DEFINE_ABSTRACT( AudListener );

typedef std::vector< AudListenerPtr > ListenerVector;

const Be::ClassInfo* AudListener::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudListener, "Representation of a listener in Wwise" )
		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( AudListener )

		MAP_ATTRIBUTE( "ID", m_ID, "Listeners ID", Be::READ)

		MAP_METHOD_AND_WRAP( "SetPosition", SetPosition, 
							 "\tUpdates the entities orientation and position of an entity.\n"
							 "\tTakes 3 arguments but is not using the \"top\" vector as is.\n"
							 "Arguments:\n"
							 "\tfront -- Vector3 representing the orientation of the object.\n"
							 "\ttop -- Vector3 representing the up vector of the object\n"
							 "\tposition -- Vector3 representing the world position .\n"
						   )
	EXPOSURE_END()
}

//-----------------------------------------------------------------------------
// Listener resource handling
//-----------------------------------------------------------------------------
static ListenerVector& GetListeners()
{
	static ListenerVector s_listeners( 8 );
	return s_listeners;
}

void DestroyListenerVector()
{
	GetListeners().clear();
}

static PyObject* PyGetAudListener( PyObject* self, PyObject* args )
{
	int id;
	AudListenerPtr alp = NULL;
	// Arg parsing
	PyArg_ParseTuple( args, "i", &id );
	if( id < 0 || id > 7 )
	{
		PyErr_SetString( PyExc_ValueError, "Wise has only 8 listeners. Please specify a number between 0 and 7." ); 
		Py_RETURN_NONE;
	}
	
	ListenerVector& listeners = GetListeners();
	if( listeners[id] == NULL )
	{
		alp.p = new OAudListener();
		alp->m_ID = id + START_LISTENER_GAME_OBJ_COUNT;
		alp->Initialize();
		listeners[id] = alp;
	}
	else
	{
		alp = listeners[id];
	}
	BluePythonObject* obj = PyOS->WrapBlueObject( alp );
	
	return obj;
}
MAP_FUNCTION( "GetListener", PyGetAudListener, "Return a specific Wwise listener." );