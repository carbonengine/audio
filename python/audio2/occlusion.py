# Copyright © 2026 CCP ehf.

import audio2


class OcclusionManager(object):
    """Wrapper for Carbon Audio's game-fed line-of-sight occlusion.

    The game runs its own line-of-sight queries against physics collision data and feeds
    the per-emitter result here (SetEmitterBlocked), typically at 5-10 Hz. The engine keeps
    the per-emitter occlusion state machine: a blocked emitter's occlusion fades towards
    blockedOcclusion and a cleared one back towards 0 at fadeRate every audio update, and
    the live values are forwarded to Wwise.

    The engine never times out or clears a blocked emitter on its own; an emitter keeps its
    last fed state until the next feed, ClearAllBlocked(), or its destruction. Stale-feed
    protection is the caller's job (clear on session change via ClearAllBlocked).

    This is a view onto the audio manager's occlusion subsystem, not a second instance. It is
    safe to construct at any time; calls made before the audio manager exists are no-ops."""

    def __init__(self):
        self.occlusion = audio2.GetOcclusion()

    def ClearAllBlocked(self):
        """Fade every emitter back to clear, for a session change or system jump."""
        self.occlusion.ClearAllBlocked()

    def GetBlockedOcclusion(self):
        """Return the occlusion a blocked emitter fades towards."""
        return self.occlusion.blockedOcclusion

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

    def SetBlockedOcclusion(self, value):
        """Set the occlusion a blocked emitter fades towards.

        Changing it re-targets every currently blocked emitter.

        :param value: Occlusion between 0.0 and 1.0. Keep below 1.0 so blocked sounds stay
                      muffled instead of silenced.
        :type value: float
        """
        self.occlusion.blockedOcclusion = value

    def SetEmitterBlocked(self, emitterID, blocked):
        """Set whether an emitter's line of sight to the listener is blocked.

        True fades the emitter's occlusion towards blockedOcclusion, False back towards 0,
        at fadeRate. The emitter keeps this state until the next feed, ClearAllBlocked(),
        or its destruction. Safe for emitter IDs the engine does not know (yet); the next
        feed re-establishes the state once the emitter exists.

        :param emitterID: The emitter whose blocked state was determined.
        :type emitterID: int
        :param blocked: True if the emitter's line of sight is blocked.
        :type blocked: bool
        """
        self.occlusion.SetEmitterBlocked(emitterID, blocked)

    def SetEmittersBlocked(self, blockedStates):
        """Set the blocked state of several emitters at once, one query batch per call.

        :param blockedStates: An iterable of (emitterID, blocked) pairs, as accepted by
                              SetEmitterBlocked.
        """
        for emitterID, blocked in blockedStates:
            self.occlusion.SetEmitterBlocked(emitterID, blocked)

    def SetEnabled(self, enabled):
        """Enable or disable occlusion processing.

        Disabling fades all values back to clear and ignores feeds until re-enabled;
        the regular feed then re-establishes the blocked states.

        :type enabled: bool
        """
        self.occlusion.enabled = enabled

    def SetFadeRate(self, fadeRate):
        """Set how fast occlusion values fade towards their targets, in units per second.

        :param fadeRate: Units per second. 0 applies new values instantaneously.
        :type fadeRate: float
        """
        self.occlusion.fadeRate = fadeRate
