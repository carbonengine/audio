////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "AudPortal.h"

BLUE_DEFINE( AudPortal );

const Be::ClassInfo* AudPortal::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudPortal, "A Wwise Spatial Audio portal, placed as a unit box through ITr2BoxVolume" )
		MAP_INTERFACE( ITr2BoxVolume )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Name of the portal, shown in the Wwise profiler.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "open", m_open, "Whether sound passes through the portal, e.g. an open door.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "adjacentRoomBleed", m_adjacentRoomBleed, "Scales how much reverb of the room on the other side is heard through the portal.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Whether the portal is active. It is only sent to Wwise when its volume object is enabled too.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "portalID", m_portalID, "Wwise portal identifier.\n:jessica-hidden: True", Be::READ )
	EXPOSURE_END()
}
