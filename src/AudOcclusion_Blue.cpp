// Copyright © 2026 CCP ehf.

#include "stdafx.h"
#include "AudOcclusion.h"

BLUE_DEFINE( AudOcclusion );

const Be::ClassInfo* AudOcclusion::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudOcclusion, "Game-fed line-of-sight audio occlusion. The game runs its own line-of-sight "
	                              "queries against physics collision data and feeds the per-emitter blocked state "
	                              "here; the engine fades each emitter's occlusion towards its target and forwards "
	                              "the values to Wwise." )
		MAP_INTERFACE( AudOcclusion )

		MAP_PROPERTY( "enabled", GetEnabled, SetEnabled, "Enable or disable occlusion processing. Disabling fades all values back to clear and ignores feeds until re-enabled; the game's regular feed then re-establishes the blocked states.")
		MAP_PROPERTY( "fadeRate", GetFadeRate, SetFadeRate, "How fast occlusion values fade towards their targets, in units per second. 0 = instantaneous.")
		MAP_PROPERTY( "blockedOcclusion", GetBlockedOcclusion, SetBlockedOcclusion, "Occlusion [0.0-1.0] a blocked emitter fades towards. Kept below 1.0 so blocked sounds stay muffled instead of silenced. Changing it re-targets every currently blocked emitter.")

		MAP_METHOD_AND_WRAP
		(
			"SetEmitterBlocked",
			SetEmitterBlocked,
			"Set whether the game considers an emitter's line of sight to the listener blocked.\n"
			"True fades the emitter's occlusion towards blockedOcclusion, False back towards 0, at\n"
			"fadeRate. The emitter keeps this state until the next feed, ClearAllBlocked(), or its\n"
			"destruction; the engine never times it out or clears it on its own. Safe for emitter\n"
			"IDs the engine does not know (yet); the game's next feed re-establishes the state once\n"
			"the emitter exists.\n"
			":param emitterID: The emitter whose blocked state the game determined.\n"
			":param blocked:   True if the emitter's line of sight is blocked."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearAllBlocked",
			ClearAllBlocked,
			"Fade every emitter back to clear, for a session change or system jump."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetEmitterOcclusion",
			GetEmitterOcclusion,
			"Get the occlusion value currently applied to an emitter. This is the live, mid-fade value "
			"rather than the target that was set, so it can be used to observe a fade in progress. "
			"Returns 0.0 if the emitter is clear or is not being tracked."
		)
	EXPOSURE_END()
}

static AudOcclusionPtr s_occlusion = nullptr;
static PyObject* PyGetOcclusion( PyObject* self, PyObject* args )
{
	if( s_occlusion == nullptr )
	{
		s_occlusion = new OAudOcclusion;
	}
	return PyOS->WrapBlueObject( s_occlusion->GetRawRoot() );
}
MAP_FUNCTION
(
	"GetOcclusion",
	PyGetOcclusion,
	"Gets the game-fed line-of-sight occlusion system. A view onto the audio manager's "
	"occlusion subsystem; calls made before the audio manager exists are safe no-ops."
);
