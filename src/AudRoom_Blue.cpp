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
	EXPOSURE_BEGIN( AudRoom, "A Wwise Spatial Audio room, placed as a unit box through ITr2VolumeObject" )
		MAP_INTERFACE( ITr2VolumeObject )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Name of the room, shown in the Wwise profiler.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "reverbAuxBus", m_reverbAuxBus, "Wwise auxiliary bus with the reverb of this room. Empty for no reverb.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "reverbLevel", m_reverbLevel, "Send level [0, 1] from emitters in the room to the reverb bus.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "transmissionLoss", m_transmissionLoss, "Transmission loss [0, 1] through the walls of this room. 1 blocks everything.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "priority", m_priority, "The room with the highest priority wins when rooms overlap.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "auxSendLevelToSelf", m_auxSendLevelToSelf, "Send level [0, 1] from the room tone to the reverb bus.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "keepRegistered", m_keepRegistered, "Keep the room game object registered in Wwise, needed for events and RTPCs on the room. Always on with a room tone.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "roomToneEvent", m_roomToneEvent, "Wwise event played on the room while it is in Wwise, e.g. an ambience bed.\n:jessica-group: Acoustics", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Whether the room is active. It is only sent to Wwise when its volume object is enabled too.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "roomID", m_roomID, "Wwise room identifier.\n:jessica-hidden: True", Be::READ )
		MAP_ATTRIBUTE( "isRegistered", m_sentToWwise, "Whether the room is in Wwise.\n:jessica-hidden: True", Be::READ )
	EXPOSURE_END()
}
