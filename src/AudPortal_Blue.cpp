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
		MAP_ATTRIBUTE( "obstruction", m_obstruction, "Obstruction [0, 1] of the direct sound through the portal.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "occlusion", m_occlusion, "Occlusion [0, 1] of all sound through the portal, e.g. a door that is partly closed.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "obstructionOcclusionTransition", m_obstructionOcclusionTransition, "Fade obstruction and occlusion over the depth of the portal.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Whether the portal is active. It is only sent to Wwise when its volume object is enabled too.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "portalID", m_portalID, "Wwise portal identifier.\n:jessica-hidden: True", Be::READ )
		MAP_ATTRIBUTE( "isRegistered", m_sentToWwise, "Whether the portal is in Wwise.\n:jessica-hidden: True", Be::READ )
		MAP_ATTRIBUTE( "frontRoomID", m_frontRoomID, "Room on the front side of the portal in Wwise.\n:jessica-hidden: True", Be::READ )
		MAP_ATTRIBUTE( "backRoomID", m_backRoomID, "Room on the back side of the portal in Wwise.\n:jessica-hidden: True", Be::READ )
	EXPOSURE_END()
}
