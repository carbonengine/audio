// Copyright © 2026 CCP ehf.

#include "stdafx.h"
#include "AudOcclusion.h"

BLUE_DEFINE( AudOcclusion );

const Be::ClassInfo* AudOcclusion::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudOcclusion, "Sphere-based line-of-sight audio occlusion. The game registers the bounding "
	                              "spheres that can block sound plus the audio-space origin, and the engine computes "
	                              "each emitter's occlusion from the listener-emitter sightline." )
		MAP_INTERFACE( AudOcclusion )

		MAP_PROPERTY( "enabled", GetEnabled, SetEnabled, "Enable or disable occlusion processing. Disabling fades all values back to clear.")
		MAP_PROPERTY( "fadeRate", GetFadeRate, SetFadeRate, "How fast occlusion values fade towards their targets, in units per second. 0 = instantaneous.")
		MAP_PROPERTY( "blockedOcclusion", GetBlockedOcclusion, SetBlockedOcclusion, "Occlusion [0.0-1.0] applied to an emitter whose line of sight an occluder sphere blocks. Kept below 1.0 so blocked sounds stay muffled instead of silenced.")
		MAP_PROPERTY( "radiusScale", GetRadiusScale, SetRadiusScale, "What fraction [0.0-1.0] of an occluder's radius counts as solid. The game registers bounding spheres, which are much larger than the object inside them, so a sightline can clip one while passing well clear of the object itself. Lower this to require real overlap instead of a graze. 1.0 uses the radii as given.")
		MAP_PROPERTY( "losRecomputeInterval", GetLosRecomputeInterval, SetLosRecomputeInterval, "How often sphere line of sight is recomputed, in seconds. Fades still advance every update. 0 = every update.")

		MAP_METHOD_AND_WRAP
		(
			"SetOccluderSphere",
			SetOccluderSphere,
			"Add or move a sphere that blocks the line of sight to emitters behind it. While any occluder\n"
			"spheres are registered, occlusion is computed per emitter against them every update.\n"
			":param occluderID: Stable identifier of the occluder (e.g. a destiny ball ID).\n"
			":param x, y, z:   Centre of the sphere in game world space (e.g. raw destiny ball\n"
			"                  coordinates). Kept in double precision and translated into audio\n"
			"                  space via SetOrigin, so occluders never need re-sending as the\n"
			"                  player moves.\n"
			":param radius:    Radius of the sphere. Values <= 0 remove the occluder."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetOrigin",
			SetOrigin,
			"Report the game world point that audio space is centred on. Emitter and listener positions\n"
			"reach audio as floats relative to this point, while occluder centres are given in game world\n"
			"space, so this is what relates the two. Send it every tick; it costs one call no matter how\n"
			"many occluders are registered.\n"
			":param x, y, z: The game world position corresponding to the audio space origin\n"
			"                (in EVE, the ego ball's position)."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetSightlineSource",
			SetSightlineSource,
			"Trace sightlines from this game world point instead of from the listener. For testing\n"
			"where occlusion should be judged from - the camera or the player's ship. Feed the ship's\n"
			"world position every tick while the override is wanted; ClearSightlineSource() flips back\n"
			"to the listener.\n"
			":param x, y, z: The point sightlines start from, in game world space (e.g. the\n"
			"                ship's raw destiny ball position)."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearSightlineSource",
			ClearSightlineSource,
			"Return sightlines to starting at the listener."
		)
		MAP_METHOD_AND_WRAP
		(
			"HasSightlineSource",
			HasSightlineSource,
			"Whether a sightline source override is currently set."
		)
		MAP_METHOD_AND_WRAP
		(
			"RemoveOccluderSphere",
			RemoveOccluderSphere,
			"Remove a single occluder sphere. Emitters it was blocking fade back to clear.\n"
			":param occluderID: The identifier the occluder was registered with."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearOccluderSpheres",
			ClearOccluderSpheres,
			"Remove every occluder sphere and fade all emitters back to clear."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetEmitterOcclusion",
			GetEmitterOcclusion,
			"Get the occlusion value currently applied to an emitter. This is the live, mid-fade value "
			"rather than the target that was set, so it can be used to observe a fade in progress. "
			"Returns 0.0 if the emitter is clear or is not being tracked."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetOccluderSphereCount",
			GetOccluderSphereCount,
			"How many occluder spheres are currently registered."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetBlockingOccluderIDs",
			GetBlockingOccluderIDs,
			"The IDs of the occluders blocking at least one emitter, as of the last sightline pass.\n"
			"A snapshot recorded while the pass runs, so reading it costs nothing per emitter; built\n"
			"for overlays that highlight the spheres currently doing something. Empty while sphere\n"
			"occlusion is inactive (no spheres, disabled, or acoustics on)."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetBlockingOccluder",
			GetBlockingOccluder,
			"The occluder currently blocking an emitter's sightline, for diagnosing occlusion that\n"
			"looks wrong. Uses the same translation, radius scaling and segment test the engine uses,\n"
			"regardless of whether occlusion is enabled.\n"
			":param emitterID: The emitter whose sightline to test.\n"
			":return: The blocking occluder's ID, or 0 if the sightline is clear or cannot be\n"
			"         computed (missing or unplaced listener or emitter)."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetOriginX",
			GetOriginX,
			"X of the game world point audio space is centred on, as last reported via SetOrigin."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetOriginY",
			GetOriginY,
			"Y of the game world point audio space is centred on, as last reported via SetOrigin."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetOriginZ",
			GetOriginZ,
			"Z of the game world point audio space is centred on, as last reported via SetOrigin."
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
	"Gets the sphere-based line-of-sight occlusion system. A view onto the audio manager's "
	"occlusion subsystem; calls made before the audio manager exists are safe no-ops."
);
