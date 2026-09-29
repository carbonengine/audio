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

    def UseFakeQuery(self, blocked):
        """Have Carbon Audio check line of sight itself, against a stand-in for destiny that gives every emitter the same answer.
        """
        import blue
        query = blue.LoadExtension("_audiotests").FakeObstructionQuery()
        query.blocked = blocked
        self.manager.obstructionQuery = query
        return query

    def GetSightlineResult(self):
        """True or False from the last sightline pass, or None if the emitter was not checked."""
        return self.manager.GetLastSightlineResults().get(self.emitter.ID)

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

    def test_last_sightline_results_hold_what_was_checked_until_cleared(self):
        """Only emitters with a voice are checked, a recheck replaces the results and clearing drops them."""
        import audio2
        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE
        query = self.UseFakeQuery(blocked=True)

        self.MakeAudible()
        silentEmitter = audio2.AudEmitter("silentOcclusionTestEmitter")
        silentEmitter.SetPlacement((0, 0, 0), (0, 0, 0), (0, 0, 0))
        self.assertTrue(WaitForEmitterToWake(silentEmitter), "Timed out waiting for the silent emitter to be woken up.")
        self.Pump()
        results = self.manager.GetLastSightlineResults()
        self.assertIs(results.get(self.emitter.ID), True, "The playing emitter was not reported blocked.")
        self.assertNotIn(silentEmitter.ID, results, "An emitter with nothing playing was checked.")
        # A new voice is checked before it plays, so it starts blocked instead of fading in at this rate.
        self.assertEqual(self.GetOcclusion(), 1.0, "The new voice did not start blocked.")

        query.blocked = False
        self.Pump()
        self.assertIs(self.GetSightlineResult(), False, "A recheck did not replace the result.")
        # The voice was already playing, so it fades back out instead of snapping.
        occlusion = self.GetOcclusion()
        self.assertLess(occlusion, 1.0, "The occlusion did not start fading out once the sightline cleared.")
        self.assertGreater(occlusion, 0.0, "The occlusion snapped to clear instead of fading out.")

        self.manager.ClearObstructionOcclusion()
        self.assertEqual(self.manager.GetLastSightlineResults(), {}, "Clearing did not drop the last sightline results.")

    def test_a_sightline_query_without_an_answer_is_asked_again_next_tick(self):
        """No answer means ask again, not clear. The new voice keeps its onset and still starts blocked once the query answers."""
        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE
        query = self.UseFakeQuery(blocked=True)
        query.hasAnswer = False

        self.MakeAudible()
        self.Pump()
        self.assertIsNone(self.GetSightlineResult(), "A query without an answer produced a result.")
        self.assertEqual(self.GetOcclusion(), 0.0, "A query without an answer changed the occlusion.")

        query.hasAnswer = True
        self.Pump(times=1)
        self.assertIs(self.GetSightlineResult(), True, "The query was not asked again once it could answer.")
        # Only a new voice snaps, a periodic recheck would fade in at this rate.
        self.assertEqual(self.GetOcclusion(), 1.0, "The new voice lost its onset when the query had no answer.")

    def test_a_culled_emitter_is_skipped_and_checked_when_it_wakes(self):
        """Culled emitters are not checked. The loop that restarts on wake is checked before it plays and starts blocked."""
        self.manager.obstructionOcclusionFadeRate = SLOW_FADE_RATE
        query = self.UseFakeQuery(blocked=False)

        self.MakeAudible()
        self.Pump()
        self.assertIs(self.GetSightlineResult(), False, "The playing emitter was not checked.")
        self.assertEqual(self.GetOcclusion(), 0.0, "A clear sightline occluded the emitter.")

        self.emitter.ForceCullingStateChange()
        self.assertTrue(self.emitter.IsCulled())
        query.blocked = True
        self.Pump()
        self.assertIsNone(self.GetSightlineResult(), "A culled emitter was checked.")

        # Cull fades the loop out over three seconds. Let it end so the loop restarts from silence on wake.
        self.assertTrue(PumpOSWithTimeout(lambda: len(self.emitter.GetPlayingEvents()) > 0, maxTries=50), "The culled loop never finished.")

        self.emitter.ForceCullingStateChange()
        self.assertFalse(self.emitter.IsCulled())
        self.Pump(times=1)
        self.assertIs(self.GetSightlineResult(), True, "The woken emitter was not checked.")
        self.assertEqual(self.GetOcclusion(), 1.0, "The woken emitter did not start blocked.")
