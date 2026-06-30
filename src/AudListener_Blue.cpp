// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudListener.h"

#include "AudGameObjResource.h"

BLUE_DEFINE_ABSTRACT( AudListener );

const Be::ClassInfo* AudListener::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudListener, "Representation of a listener in Wwise" )

		MAP_ATTRIBUTE( "ID", m_ID, "Listeners ID", Be::READ)

		MAP_METHOD_AND_WRAP
		( 
			"SetPosition", 
			SetPositionHelper, 
			"Updates the entities orientation and position of an entity.\n"
			":param front: Vector3 representing the orientation of the object.\n"
			":param top: Vector3 representing the up vector of the object.\n"
			":param position: Vector3 representing the world position."
		)
	EXPOSURE_CHAINTO( AudGameObjResource )
}

static AudListenerPtr s_listener = nullptr;
static PyObject* PyGetAudListener( PyObject* self, PyObject* args )
{
	if( s_listener == nullptr )
	{
		s_listener = new OAudListener;
		s_listener->Initialize();
	}
	else
	{
		s_listener->RegisterWwiseObject();
	}
	return PyOS->WrapBlueObject( s_listener->GetRawRoot() );
}
MAP_FUNCTION( "GetListener", PyGetAudListener, "Return a specific Wwise listener." );