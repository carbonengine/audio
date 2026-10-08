////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#include "stdafx.h"
#include "AudRoom.h"

BLUE_DEFINE( AudRoom );
BLUE_DEFINE_INTERFACE( ITr2VolumeObject );

const Be::ClassInfo* AudRoom::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudRoom, "A Wwise Spatial Audio room. Trinity places it as a unit box through ITr2VolumeObject; the acoustic parameters live here." )
		MAP_INTERFACE( ITr2VolumeObject )

		MAP_ATTRIBUTE( "name", m_name, "Name of the room, shown in the Wwise profiler.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reverbAuxBus", m_reverbAuxBus, "Name of the Wwise auxiliary bus carrying the reverb of this room. Empty for no reverb.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reverbLevel", m_reverbLevel, "Send level [0, 1] from emitters inside the room to the reverb aux bus.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transmissionLoss", m_transmissionLoss, "Transmission loss [0, 1] applied to sound passing through the walls of this room. 1 blocks everything.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "priority", m_priority, "When a game object is inside several rooms the room with the highest priority wins.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "auxSendLevelToSelf", m_auxSendLevelToSelf, "Send level [0, 1] of the room game object to its own reverb bus, for room tones posted on the room.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "keepRegistered", m_keepRegistered, "Keep the room game object registered in Wwise at all times, needed when posting events or RTPCs on the room.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Whether the room is acoustically active. The room is only sent to Wwise when this and the owning volume object are both enabled.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "roomID", m_roomID, "Wwise room identifier.\n:jessica-hidden: True", Be::READ )
	EXPOSURE_END()
}
