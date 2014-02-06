#include "StdAfx.h"
#include "AudPlaylistItem.h"

BLUE_DEFINE_NONEXPOSED( AudPlaylistItem );

const Be::ClassInfo* AudPlaylistItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( AudPlaylistItem, "Stores basic information about a song in the jukebox." )
		MAP_INTERFACE( AudPlaylistItem )
		MAP_INTERFACE( IRoot )

        MAP_ATTRIBUTE("Duration", m_Duration, "Duration of the playlist entry in milliseconds", Be::READ)
        MAP_ATTRIBUTE("Artist", m_Artist, "Duration of the playlist entry in milliseconds", Be::READ)
        MAP_ATTRIBUTE("Title", m_Title, "Duration of the playlist entry in milliseconds", Be::READ)
    EXPOSURE_END()
}