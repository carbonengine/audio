#include "stdafx.h"
#include "AudJukebox.h"
#include "AudPlaylistItem.h"

BLUE_DEFINE_ABSTRACT( AudJukebox );

static PyObject* PyGetTrackInfo( PyObject* self, PyObject* args )
{
    if ( ! g_audioInitialized )
        Py_RETURN_NONE;

    PyUnicodeObject* tmp = NULL;
    if ( 0 == PyArg_ParseTuple( args, "U", &tmp ) )
        Py_RETURN_NONE;
    
    Py_ssize_t size = PyUnicode_GET_SIZE( tmp );
    
    if (size > 65536)
    {
        Py_RETURN_NONE;
    }

    wchar_t wstr[65536] = {'\0'};
    if ( -1 == PyUnicode_AsWideChar( tmp, wstr, size ) )
    {
        Py_RETURN_NONE;
    }

    CCPMP3BaseInfo info;
    CCPMP3Tag tag;

    if (! CCPMP3InfoRead( wstr, info, &tag ) )
        Py_RETURN_NONE;

    OAudPlaylistItem* item = new OAudPlaylistItem;
    item->m_Artist = tag.wszArtist;
    item->m_Duration = info.msDuration;
    item->m_Title = tag.wszTrack;
    BluePythonObject* obj = PyOS->WrapBlueObject( item->GetRawRoot() );
    item->Unlock();

    return obj;
}


const Be::ClassInfo* AudJukebox::ExposeToBlue()
{
    EXPOSURE_BEGIN( AudJukebox, "Object that mimics the old jukebox behavior in EVE but using Wwise as its backend." )
        MAP_INTERFACE( AudJukebox )
		MAP_INTERFACE( AudGameObjResource )

		MAP_ATTRIBUTE( "eventListener", m_eventListener, "", Be::READWRITE )

		MAP_METHOD_AND_WRAP( "SendEvent", SendEvent, "" )
		MAP_METHOD_AND_WRAP( "Play", Play, "Plays a MP3 file, for which the absolute path has to be passed to this function." )
        MAP_METHOD( "GetTrackInfo", PyGetTrackInfo, "Retrieves ID3 tag and header info from specified MP3 file" )
    EXPOSURE_END()
}

static BluePythonObject* obj = NULL;
static PyObject* PyGetJukebox( PyObject* self, PyObject* args )
{
	AudJukebox* jb;
	if( !obj )
	{
		jb = new OAudJukebox;
		obj = PyOS->WrapBlueObject( jb->GetRawRoot() );
		jb->GetRawRoot()->Unlock();
	}
	return obj;
}
MAP_FUNCTION( "GetJukebox", PyGetJukebox,
			 "Description:\n"
			 "\tGets an instance of AudJukebox, if none exists it creates one(Singleton)."
			 "Signature:\n"
			 "\tGetJukebox() -> AudJukebox instance\n"
			 );
