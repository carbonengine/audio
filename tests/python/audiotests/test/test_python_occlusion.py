# Copyright © 2026 CCP ehf.

import unittest

from audiotests.base_test_class import COMMON_BNK
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout


INSTANT_FADE_RATE = 0.0

FRONT = (0, 0, 1)
TOP = (0, 1, 0)
EMITTER_POSITION = (0, 0, 100)


class TestPythonOcclusion(BaseAudio2TestClass):
    """Tests the OcclusionManager Python wrapper. Its methods are pass-throughs onto
    the Blue object, so a wrong property or method name fails silently until called;
    these exercise every one of them."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK])

    def setUp(self):
        import audio2
        from audio2.occlusion import OcclusionManager

        self.audioManager.Enable()
        self.occlusion = OcclusionManager()

        self.emitter = audio2.AudEmitter("pythonOcclusionTestEmitter")
        self.emitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.defaults = {
            "enabled": self.occlusion.GetEnabled(),
            "fadeRate": self.occlusion.GetFadeRate(),
            "blockedOcclusion": self.occlusion.GetBlockedOcclusion(),
        }

        self.occlusion.SetEnabled(True)
        self.occlusion.SetFadeRate(INSTANT_FADE_RATE)

    def tearDown(self):
        # These settings are global, so hand them back as they were found rather than
        # leaking this class's values into whatever runs next.
        self.occlusion.ClearAllBlocked()
        self.occlusion.SetEnabled(self.defaults["enabled"])
        self.occlusion.SetFadeRate(self.defaults["fadeRate"])
        self.occlusion.SetBlockedOcclusion(self.defaults["blockedOcclusion"])
        self.emitter = None
        self.occlusion = None
        self.audioManager.Disable()

    def Pump(self, times=3):
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=times)

    def test_enabled_round_trips(self):
        self.occlusion.SetEnabled(False)
        self.assertFalse(self.occlusion.GetEnabled())

        self.occlusion.SetEnabled(True)
        self.assertTrue(self.occlusion.GetEnabled())

    def test_fade_rate_round_trips(self):
        self.occlusion.SetFadeRate(2.0)

        self.assertEqual(self.occlusion.GetFadeRate(), 2.0)

    def test_blocked_occlusion_round_trips(self):
        self.occlusion.SetBlockedOcclusion(0.5)

        self.assertEqual(self.occlusion.GetBlockedOcclusion(), 0.5)

    def test_a_blocked_feed_through_the_wrapper_occludes_an_emitter(self):
        """End to end through the wrapper: feed a blocked state, tick, read the emitter."""
        self.occlusion.SetEmitterBlocked(self.emitter.ID, True)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID),
                         self.occlusion.GetBlockedOcclusion())

        self.occlusion.SetEmitterBlocked(self.emitter.ID, False)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID), 0.0)

    def test_a_batched_feed_reaches_every_emitter(self):
        import audio2
        secondEmitter = audio2.AudEmitter("pythonOcclusionTestEmitter2")
        secondEmitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.occlusion.SetEmittersBlocked([(self.emitter.ID, True),
                                           (secondEmitter.ID, True)])
        self.Pump()

        blocked = self.occlusion.GetBlockedOcclusion()
        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID), blocked)
        self.assertEqual(self.occlusion.GetEmitterOcclusion(secondEmitter.ID), blocked)

    def test_clearing_through_the_wrapper_returns_emitters_to_clear(self):
        self.occlusion.SetEmitterBlocked(self.emitter.ID, True)
        self.Pump()
        self.assertGreater(self.occlusion.GetEmitterOcclusion(self.emitter.ID), 0.0)

        self.occlusion.ClearAllBlocked()
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID), 0.0)

    def test_the_wrapper_is_a_view_onto_the_one_occlusion_subsystem(self):
        """The wrapper must not stand up state of its own: a second wrapper, and the
        Blue object itself, have to see what this one did."""
        import audio2
        from audio2.occlusion import OcclusionManager

        self.occlusion.SetBlockedOcclusion(0.5)
        self.occlusion.SetEmitterBlocked(self.emitter.ID, True)
        self.Pump()

        self.assertEqual(audio2.GetOcclusion().blockedOcclusion, 0.5)
        self.assertEqual(audio2.GetOcclusion().GetEmitterOcclusion(self.emitter.ID), 0.5)
        self.assertEqual(OcclusionManager().GetBlockedOcclusion(), 0.5)
        self.assertEqual(OcclusionManager().GetEmitterOcclusion(self.emitter.ID), 0.5)


if __name__ == "__main__":
    unittest.main()
