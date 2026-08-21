# Copyright © 2026 CCP ehf.


class OcclusionManager(object):
    """Wrapper for Carbon Audio's line-of-sight obstruction/occlusion API.

    The game runs its own sightline queries and feeds per-emitter blockage values in here.
    Carbon Audio fades the applied occlusion towards each emitter's latest value and never
    times values out, so keeping feeds fresh is the caller's responsibility.

    Reached through AudioManager as audioManager.occlusion rather than constructed directly.
    """

    def __init__(self, manager):
        """
        :param manager: The raw Carbon Audio manager whose occlusion API this wraps.
        """
        self._manager = manager

    @property
    def enabled(self):
        """Whether game-driven obstruction/occlusion processing is enabled. Disabling fades all values back to clear."""
        return self._manager.obstructionOcclusionEnabled

    @enabled.setter
    def enabled(self, value):
        self._manager.obstructionOcclusionEnabled = value

    @property
    def fadeRate(self):
        """How fast obstruction/occlusion values fade towards their targets, in units per second. 0 = instantaneous."""
        return self._manager.obstructionOcclusionFadeRate

    @fadeRate.setter
    def fadeRate(self, value):
        self._manager.obstructionOcclusionFadeRate = value

    def ClearAll(self):
        """Fade the occlusion values of all tracked emitters back to clear."""
        self._manager.ClearObstructionOcclusion()

    def GetEmitterOcclusion(self, emitterID):
        """Get the occlusion value currently applied to an emitter.

        :param emitterID: The ID of the emitter to query.
        :type emitterID: int
        :return: The emitter's current occlusion value, or 0.0 if the emitter is clear or is not being tracked.
        """
        return self._manager.GetEmitterOcclusion(emitterID)

    def SetEmitterLineOfSightBlockage(self, emitterID, blockage):
        """Set a line-of-sight blockage ratio for an emitter.

        The applied occlusion fades towards this value at fadeRate.

        :param emitterID: The ID of the emitter the blockage applies to.
        :type emitterID: int
        :param blockage: How blocked the line of sight between the listener and the emitter is,
                         from 0.0 (clear) to 1.0 (fully blocked).
        :type blockage: float
        :return: True if the emitter exists, otherwise False.
        """
        return self._manager.SetEmitterLineOfSightBlockage(emitterID, blockage)
