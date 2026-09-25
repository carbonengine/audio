# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout, WaitForEmitterToWake, WaitForSoundBanksToLoad


INSTANT_FADE_RATE = 0.0
SLOW_FADE_RATE = 0.01
FAST_FADE_RATE = 100.0


class TestObstructionOcclusionExposure(BaseAudio2TestClass):
    """Tests line-of-sight occlusion: when values snap, how they fade, and what gets clamped or rejected."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK, LOOP_BNK])

    def setUp(self):
        import audio2
        self.audioManager.Enable()
        self.manager = self.audioManager.manager

        self.emitter = audio2.AudEmitter("occlusionTestEmitter")
        self.emitter.SetPlacement((0, 0, 0), (0, 0, 0), (0, 0, 0))

        self.manager.obstructionOcclusionEnabled = True
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE

    def tearDown(self):
        self.emitter.StopAll()
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

    def MakeAudible(self):
        """Start a loop on the emitter. Silent emitters snap to new values, so the fade tests need something playing.
        """
        import audio2
        audio2.GetListener().SetPosition((0, 0, 0), (0, 0, 0), (0, 0, 0))
        self.assertTrue(WaitForSoundBanksToLoad([LOOP_EVENT]), "Timed out waiting for the test SoundBank to load.")
        self.assertTrue(WaitForEmitterToWake(self.emitter), "Timed out waiting for the emitter to be woken up.")
        self.assertGreater(self.emitter.SendEvent(LOOP_EVENT), 0, "The loop did not start playing.")

    def test_a_silent_emitter_snaps_to_new_values(self):
        """Nothing is playing so there is nothing to pop, every value applies at once even at a slow fade rate."""
        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0, "The first value did not apply at once.")

        self.assertTrue(self.SetBlockage(0.5))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.5, "A later value did not apply at once.")

    def test_a_playing_emitter_fades_in_and_out(self):
        self.MakeAudible()
        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        firstValue = self.GetOcclusion()
        self.Pump()
        secondValue = self.GetOcclusion()
        self.assertGreater(firstValue, 0.0, "Occlusion never started fading in.")
        self.assertGreater(secondValue, firstValue, "Occlusion stopped fading in.")
        self.assertLess(secondValue, 1.0, "Occlusion jumped to its target instead of fading in.")

        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE
        self.assertTrue(self.SetBlockage(0.0))
        self.Pump()
        firstValue = self.GetOcclusion()
        self.Pump()
        secondValue = self.GetOcclusion()
        self.assertLess(firstValue, 1.0, "Occlusion never started fading out.")
        self.assertLess(secondValue, firstValue, "Occlusion stopped fading out.")
        self.assertGreater(secondValue, 0.0, "Occlusion jumped to its target instead of fading out.")

    def test_a_fade_lands_exactly_on_its_target(self):
        """A tick longer than the whole fade stops on the target, and a zero fade rate applies at once."""
        self.MakeAudible()
        self.manager.obstructionOcclusionFadeRate = FAST_FADE_RATE

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0, "Fading in overshot the target.")

        self.assertTrue(self.SetBlockage(0.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0, "Fading out overshot the target.")

        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0, "A zero fade rate did not apply at once.")

        self.manager.obstructionOcclusionFadeRate = -5.0
        self.assertEqual(self.manager.obstructionOcclusionFadeRate, 0.0, "A negative fade rate was not clamped to zero.")

    def test_invalid_blockage_is_clamped_or_rejected(self):
        import audio2

        self.assertTrue(self.SetBlockage(5.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0, "Blockage above 1 was not clamped.")

        self.assertTrue(self.SetBlockage(-1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0, "Blockage below 0 was not clamped.")

        unknownEmitterID = self.emitter.ID + 1000000
        self.assertFalse(self.manager.SetEmitterLineOfSightBlockage(unknownEmitterID, 1.0), "Blockage was accepted for an emitter that does not exist.")

        # Occlusion is relative to the listener, so blocking the listener itself means nothing.
        listenerID = audio2.GetListener().ID
        self.assertFalse(self.manager.SetEmitterLineOfSightBlockage(listenerID, 1.0), "Blockage was accepted for the listener.")

    def test_clearing_and_disabling_return_emitters_to_clear(self):
        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.manager.ClearObstructionOcclusion()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0, "Clearing did not return the emitter to clear.")

        self.assertTrue(self.SetBlockage(1.0))
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 1.0)

        self.manager.obstructionOcclusionEnabled = False
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0, "Disabling did not return the emitter to clear.")
        self.assertFalse(self.SetBlockage(1.0), "Blockage was accepted while disabled.")

    def test_occlusion_is_suppressed_while_acoustics_are_enabled(self):
        """Acoustics transmission already attenuates, so occlusion must not stack on top of it."""
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        try:
            self.assertTrue(self.SetBlockage(1.0))
            self.Pump()

            self.assertEqual(self.GetOcclusion(), 0.0)
        finally:
            self.manager.spatialAudioGeometryEnabled = False
