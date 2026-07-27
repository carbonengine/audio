# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import COMMON_BNK
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout


INSTANT_FADE_RATE = 0.0
SLOW_FADE_RATE = 0.01  
FAST_FADE_RATE = 100.0 


class TestObstructionOcclusionExposure(BaseAudio2TestClass):
    """Tests line-of-sight occlusion, in particular the interpolation that produces the fade."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK])

    def setUp(self):
        import audio2
        self.audioManager.Enable()
        self.manager = self.audioManager.manager

        self.emitter = audio2.AudEmitter("occlusionTestEmitter")
        self.emitter.SetPlacement((0, 0, 0), (0, 0, 0), (0, 0, 0))

        self.manager.obstructionOcclusionEnabled = True
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE

    def tearDown(self):
        self.emitter = None
        self.audioManager.Disable()

    def Pump(self, times=3):
        """Tick the engine. Each tick runs AudManager::Process(), which advances the occlusion fades.
        """
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=times)

    def GetOcclusion(self):
        return self.manager.GetEmitterOcclusion(self.emitter.ID)

    def SetBlockage(self, blockage):
        return self.manager.SetEmitterLineOfSightBlockage(self.emitter.ID, blockage)

    def test_occlusion_is_applied_immediately_at_a_zero_fade_rate(self):
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 1.0)

    def test_occlusion_fades_in_gradually(self):
        """The value has to interpolate towards its target rather than jumping straight to it."""
        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        firstValue = self.GetOcclusion()
        self.Pump()
        secondValue = self.GetOcclusion()

        self.assertGreater(firstValue, 0.0, "Occlusion never started fading in.")
        self.assertGreater(secondValue, firstValue, "Occlusion stopped advancing towards its target.")
        self.assertLess(secondValue, 1.0, "Occlusion jumped to its target instead of fading.")

    def test_occlusion_fades_back_out_gradually(self):
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE
        self.assertTrue(self.SetBlockage(0.0))
        self.Pump()
        firstValue = self.GetOcclusion()
        self.Pump()
        secondValue = self.GetOcclusion()

        self.assertLess(firstValue, 1.0, "Occlusion never started fading out.")
        self.assertLess(secondValue, firstValue, "Occlusion stopped advancing towards its target.")
        self.assertGreater(secondValue, 0.0, "Occlusion jumped to its target instead of fading.")

    def test_a_fade_settles_on_its_target_without_overshooting(self):
        """A single tick large enough to cover the whole fade must stop exactly on the target."""
        self.manager.obstructionOcclusionFadeRate = FAST_FADE_RATE

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 1.0)

    def test_a_fade_out_settles_on_its_target_without_undershooting(self):
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()

        self.manager.obstructionOcclusionFadeRate = FAST_FADE_RATE
        self.assertTrue(self.SetBlockage(0.0))
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_blockage_is_clamped_to_a_valid_range(self):
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE

        self.assertTrue(self.SetBlockage(5.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.assertTrue(self.SetBlockage(-1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_negative_fade_rate_is_clamped_to_zero(self):
        self.manager.obstructionOcclusionFadeRate = -5.0

        self.assertEqual(self.manager.obstructionOcclusionFadeRate, 0.0)

    def test_clearing_returns_every_emitter_to_clear(self):
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.manager.ClearObstructionOcclusion()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_disabling_clears_existing_values_and_rejects_new_ones(self):
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.manager.obstructionOcclusionEnabled = False
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        self.assertFalse(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_blockage_is_rejected_for_an_emitter_that_does_not_exist(self):

        unknownEmitterID = self.emitter.ID + 1000000

        self.assertFalse(self.manager.SetEmitterLineOfSightBlockage(unknownEmitterID, 1.0))
        self.assertEqual(self.manager.GetEmitterOcclusion(unknownEmitterID), 0.0)

    def test_blockage_is_rejected_for_the_listener(self):
        """Occlusion is measured relative to the listener, so occluding the listener is meaningless."""
        import audio2
        listenerID = audio2.GetListener().ID

        self.assertFalse(self.manager.SetEmitterLineOfSightBlockage(listenerID, 1.0))
        self.assertEqual(self.manager.GetEmitterOcclusion(listenerID), 0.0)

    def test_occlusion_is_suppressed_while_acoustics_are_enabled(self):
        """Acoustics transmission already attenuates, so occlusion must not stack on top of it."""
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        try:
            self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE

            self.assertTrue(self.SetBlockage(1.0))
            self.Pump()

            self.assertEqual(self.GetOcclusion(), 0.0)
        finally:
            self.manager.spatialAudioGeometryEnabled = False
