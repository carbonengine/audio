# Copyright © 2026 CCP ehf.

import audio2


class OcclusionManager(object):
    """Wrapper for Carbon Audio's sphere-based line-of-sight occlusion.

    The game registers the bounding spheres that can block sound (SetOccluderSphere) plus the
    game world position of the audio-space origin every tick (SetOrigin), and the engine computes
    each positioned emitter's occlusion from the listener-emitter sightline. Blocked emitters
    fade towards blockedOcclusion; cleared ones fade back to 0.

    This is a view onto the audio manager's occlusion subsystem, not a second instance. It is
    safe to construct at any time; calls made before the audio manager exists are no-ops."""

    def __init__(self):
        self.occlusion = audio2.GetOcclusion()

    def ClearOccluderSpheres(self):
        """Remove every occluder sphere and fade all emitters back to clear."""
        self.occlusion.ClearOccluderSpheres()

    def ClearSightlineSource(self):
        """Return sightlines to starting at the listener."""
        self.occlusion.ClearSightlineSource()

    def GetBlockedOcclusion(self):
        """Return the occlusion applied to an emitter whose line of sight an occluder sphere blocks."""
        return self.occlusion.blockedOcclusion

    def GetBlockingOccluderIDs(self):
        """Return the IDs of the occluders blocking at least one emitter, as of the last
        sightline pass. A snapshot recorded while the pass runs, so reading it costs
        nothing per emitter; built for overlays that highlight the spheres currently
        doing something. Empty while sphere occlusion is inactive (no spheres, disabled,
        or acoustics on)."""
        return self.occlusion.GetBlockingOccluderIDs()

    def GetBlockingOccluder(self, emitterID):
        """Return the ID of the occluder currently blocking an emitter's sightline, for
        diagnosing occlusion that looks wrong. Uses the same translation, radius scaling
        and segment test the engine uses, regardless of whether occlusion is enabled.

        :param emitterID: The emitter whose sightline to test.
        :type emitterID: int
        :return: The blocking occluder's ID, or 0 if the sightline is clear or cannot be
                 computed (missing or unplaced listener or emitter).
        """
        return self.occlusion.GetBlockingOccluder(emitterID)

    def GetEmitterOcclusion(self, emitterID):
        """Return the occlusion currently applied to an emitter. This is the live, mid-fade
        value rather than the target, so it can be used to observe a fade in progress.
        Returns 0.0 if the emitter is clear or is not being tracked.

        :param emitterID: The emitter to query.
        :type emitterID: int
        """
        return self.occlusion.GetEmitterOcclusion(emitterID)

    def GetEnabled(self):
        """Return whether occlusion processing is enabled."""
        return self.occlusion.enabled

    def GetFadeRate(self):
        """Return how fast occlusion values fade towards their targets, in units per second."""
        return self.occlusion.fadeRate

    def GetLosRecomputeInterval(self):
        """Return how often sphere line of sight is recomputed, in seconds."""
        return self.occlusion.losRecomputeInterval

    def GetOccluderSphereCount(self):
        """Return how many occluder spheres are currently registered."""
        return self.occlusion.GetOccluderSphereCount()

    def GetOrigin(self):
        """Return the game world point audio space is centred on, as last reported via
        SetOrigin, as an (x, y, z) tuple. For diagnosing occlusion that looks wrong: a
        stale or wrong origin makes every sightline test use the wrong geometry."""
        return (self.occlusion.GetOriginX(),
                self.occlusion.GetOriginY(),
                self.occlusion.GetOriginZ())

    def GetRadiusScale(self):
        """Return what fraction of an occluder's radius counts as solid."""
        return self.occlusion.radiusScale

    def HasSightlineSource(self):
        """Return whether a sightline source override is currently set."""
        return self.occlusion.HasSightlineSource()

    def RemoveOccluderSphere(self, occluderID):
        """Remove a single occluder sphere. Emitters it was blocking fade back to clear.

        :param occluderID: The identifier the occluder was registered with.
        :type occluderID: int
        """
        self.occlusion.RemoveOccluderSphere(occluderID)

    def SetBlockedOcclusion(self, value):
        """Set the occlusion applied to an emitter whose line of sight an occluder sphere blocks.

        :param value: Occlusion between 0.0 and 1.0. Keep below 1.0 so blocked sounds stay
                      muffled instead of silenced.
        :type value: float
        """
        self.occlusion.blockedOcclusion = value

    def SetEnabled(self, enabled):
        """Enable or disable occlusion processing. Disabling fades all values back to clear.

        :type enabled: bool
        """
        self.occlusion.enabled = enabled

    def SetFadeRate(self, fadeRate):
        """Set how fast occlusion values fade towards their targets, in units per second.

        :param fadeRate: Units per second. 0 applies new values instantaneously.
        :type fadeRate: float
        """
        self.occlusion.fadeRate = fadeRate

    def SetLosRecomputeInterval(self, seconds):
        """Set how often sphere line of sight is recomputed, in seconds.

        Fades still advance every update; only the ray tests are throttled.

        :param seconds: The interval in seconds. 0 recomputes every update.
        :type seconds: float
        """
        self.occlusion.losRecomputeInterval = seconds

    def SetOccluderSphere(self, occluderID, x, y, z, radius):
        """Add or move a sphere that blocks the line of sight to emitters behind it.

        While any occluder spheres are registered, occlusion is computed per emitter against
        them every update.

        :param occluderID: Stable identifier of the occluder (e.g. a destiny ball ID).
        :type occluderID: int
        :param x, y, z: Centre of the sphere in game world space (e.g. raw destiny ball
                        coordinates). Kept in double precision and translated into audio space
                        via SetOrigin, so occluders never need re-sending as the player moves.
        :type x, y, z: float
        :param radius: Radius of the sphere. Values <= 0 remove the occluder.
        :type radius: float
        """
        self.occlusion.SetOccluderSphere(occluderID, x, y, z, radius)

    def SetOrigin(self, x, y, z):
        """Report the game world point that audio space is centred on. Send this every tick.

        Emitter and listener positions reach audio as floats relative to this point, while
        occluder centres are given in game world space, so this is what relates the two.

        :param x, y, z: The game world position corresponding to the audio space origin
                        (in EVE, the ego ball's position).
        :type x, y, z: float
        """
        self.occlusion.SetOrigin(x, y, z)

    def SetRadiusScale(self, scale):
        """Set what fraction of an occluder's radius counts as solid.

        The game registers bounding spheres, which are much larger than the objects inside
        them, so a sightline can clip a sphere while passing visibly clear of the object.
        Lower this to require real overlap instead of a graze.

        :param scale: A fraction between 0.0 and 1.0. 1.0 uses the radii as given.
        :type scale: float
        """
        self.occlusion.radiusScale = scale

    def SetSightlineSource(self, x, y, z):
        """Trace sightlines from this game world point instead of from the listener.

        For testing where occlusion should be judged from - the camera or the player's
        ship. Feed the ship's world position every tick while the override is wanted;
        ClearSightlineSource() flips back to the listener, so switching perspective
        mid-flight is one call either way.

        :param x, y, z: The point sightlines start from, in game world space (e.g. the
                        ship's raw destiny ball position).
        :type x, y, z: float
        """
        self.occlusion.SetSightlineSource(x, y, z)
