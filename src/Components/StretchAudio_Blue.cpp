// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "StretchAudio.h"

BLUE_DEFINE( StretchAudio );
BLUE_DEFINE_INTERFACE( IStretchAudio );
BLUE_DEFINE_INTERFACE( ITr2DebugRenderable );

const Be::ClassInfo* StretchAudio::ExposeToBlue()
{
	EXPOSURE_BEGIN( StretchAudio, ":jessica-help-url: https://wiki.ccpgames.com/display/CAudio/Tr2AudioStretchBase" )
		MAP_INTERFACE( IInitialize )
	    MAP_INTERFACE( ITr2DebugRenderable)
		MAP_INTERFACE( IStretchAudio )

		MAP_ATTRIBUTE(
			"sourceEmitter",
			m_sourceEmitter,
			"Source audio emitter, Follows the source position. The outburst event will come from this emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"destinationEmitter",
			m_destEmitter,
			"Destination audio emitter, follows the destination position. The impact event will come from this emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"stretchEmitter",
			m_stretchEmitter,
			"Audio emitter for the looping part of the stretch effect. It follows the camera as long as it is between the "
			"source and destination. The stretch event will come from this emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"outburstEvent",
			m_outburstEvent,
			"The event to play on the outburst of the weapon firing. Comes from the source audio emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"impactEvent",
			m_impactEvent,
			"The event to play on the impact of the weapon hitting the destination. Comes from the destination audio emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"stretchEvent",
			m_stretchEvent,
			"The event to play on the stretch effect between the source and destination. Comes from the stretch emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE(
			"shotMissedEvent",
			m_shotMissedEvent,
			"The event to play when the shot is missed. Comes from the stretch emitter.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_METHOD_AND_WRAP( 
			"SetShotMissed", 
			SetShotMissed, 
			"Sets whether the next shot is a miss. If true, the shotMissedEvent will be played on the stretch emitter when the stretch effect starts."
		)
	EXPOSURE_END()
}