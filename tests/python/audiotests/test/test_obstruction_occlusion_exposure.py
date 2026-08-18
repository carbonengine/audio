# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import COMMON_BNK
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout


INSTANT_FADE_RATE = 0.0
SLOW_FADE_RATE = 0.01
FAST_FADE_RATE = 100.0

FRONT = (0, 0, 1)
TOP = (0, 1, 0)
EMITTER_POSITION = (0, 0, 100)


class TestObstructionOcclusionExposure(BaseAudio2TestClass):
    """Tests the game-fed occlusion state machine: the game feeds per-emitter blocked
    states and the engine fades each emitter's occlusion towards the target that sets."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK])

    def setUp(self):
        import audio2
        self.audioManager.Enable()
        self.occlusion = audio2.GetOcclusion()

        self.emitter = audio2.AudEmitter("occlusionTestEmitter")
        self.emitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.defaults = {
            "enabled": self.occlusion.enabled,
            "fadeRate": self.occlusion.fadeRate,
            "blockedOcclusion": self.occlusion.blockedOcclusion,
        }

        self.occlusion.enabled = True
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        # The value a blocked emitter fades towards.
        self.blocked = self.occlusion.blockedOcclusion

    def tearDown(self):
        # These settings are global, so hand them back as they were found rather than
        # leaking this class's values into whatever runs next.
        self.occlusion.ClearAllBlocked()
        self.occlusion.enabled = self.defaults["enabled"]
        self.occlusion.fadeRate = self.defaults["fadeRate"]
        self.occlusion.blockedOcclusion = self.defaults["blockedOcclusion"]
        self.emitter = None
        self.occlusion = None
        self.audioManager.Disable()

    def Pump(self, times=3):
        """Tick the engine. Each tick runs AudManager::Process(), which advances the
        occlusion fades."""
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=times)

    def GetOcclusion(self):
        return self.occlusion.GetEmitterOcclusion(self.emitter.ID)

    def Block(self):
        self.occlusion.SetEmitterBlocked(self.emitter.ID, True)

    def Unblock(self):
        self.occlusion.SetEmitterBlocked(self.emitter.ID, False)

    def EstablishTrackedAndClear(self):
        """Block and clear the emitter at an instant fade rate, so later fades start
        from a tracked emitter at 0. An emitter's very first value snaps instead of
        fading, so gradual-fade tests need this first."""
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        self.Block()
        self.Pump()
        self.Unblock()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_first_value_is_applied_at_once(self):
        """An emitter that starts out behind cover is muffled immediately instead of
        fading in from clear, even at a slow fade rate."""
        self.occlusion.fadeRate = SLOW_FADE_RATE

        self.Block()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.blocked)

    def test_an_emitter_that_is_back_to_clear_still_fades(self):
        self.EstablishTrackedAndClear()

        self.occlusion.fadeRate = SLOW_FADE_RATE
        self.Block()
        self.Pump()

        occlusion = self.GetOcclusion()
        self.assertGreater(occlusion, 0.0, "Occlusion never started fading back in.")
        self.assertLess(occlusion, self.blocked, "Occlusion jumped to its target instead of fading.")

    def test_occlusion_is_applied_immediately_at_a_zero_fade_rate(self):
        self.EstablishTrackedAndClear()
        self.occlusion.fadeRate = INSTANT_FADE_RATE

        self.Block()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.blocked)

    def test_occlusion_fades_in_gradually(self):
        """The value has to interpolate towards its target rather than jumping straight to it."""
        self.EstablishTrackedAndClear()
        self.occlusion.fadeRate = SLOW_FADE_RATE

        self.Block()
        self.Pump()
        firstValue = self.GetOcclusion()
        self.Pump()
        secondValue = self.GetOcclusion()

        self.assertGreater(firstValue, 0.0, "Occlusion never started fading in.")
        self.assertGreater(secondValue, firstValue, "Occlusion stopped advancing towards its target.")
        self.assertLess(secondValue, self.blocked, "Occlusion jumped to its target instead of fading.")

    def test_occlusion_fades_back_out_gradually(self):
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.blocked)

        self.occlusion.fadeRate = SLOW_FADE_RATE
        self.Unblock()
        self.Pump()
        firstValue = self.GetOcclusion()
        self.Pump()
        secondValue = self.GetOcclusion()

        self.assertLess(firstValue, self.blocked, "Occlusion never started fading out.")
        self.assertLess(secondValue, firstValue, "Occlusion stopped advancing towards its target.")
        self.assertGreater(secondValue, 0.0, "Occlusion jumped to its target instead of fading.")

    def test_a_fade_settles_on_its_target_without_overshooting(self):
        """A single tick large enough to cover the whole fade must stop exactly on the target."""
        self.EstablishTrackedAndClear()
        self.occlusion.fadeRate = FAST_FADE_RATE

        self.Block()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.blocked)

    def test_a_fade_out_settles_on_its_target_without_undershooting(self):
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        self.Block()
        self.Pump()

        self.occlusion.fadeRate = FAST_FADE_RATE
        self.Unblock()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_blocked_emitter_keeps_its_state_between_feeds(self):
        """The engine must never time out or auto-clear a blocked emitter on its own;
        the blocked/clear decision belongs to the game."""
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.blocked)

        # Many updates with no feed at all: the state has to hold.
        self.Pump(times=10)

        self.assertEqual(self.GetOcclusion(), self.blocked)

    def test_changing_blocked_occlusion_retargets_a_blocked_emitter(self):
        """QA tooling tunes blockedOcclusion live, so an already-blocked emitter has to
        follow the new value without waiting for the next feed."""
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.blocked)

        self.occlusion.blockedOcclusion = 0.75
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.75)

    def test_blocked_occlusion_is_clamped_to_a_valid_range(self):
        try:
            self.occlusion.blockedOcclusion = 5.0
            self.assertEqual(self.occlusion.blockedOcclusion, 1.0)

            self.occlusion.blockedOcclusion = -1.0
            self.assertEqual(self.occlusion.blockedOcclusion, 0.0)
        finally:
            self.occlusion.blockedOcclusion = self.blocked

    def test_a_negative_fade_rate_is_clamped_to_zero(self):
        self.occlusion.fadeRate = -5.0

        self.assertEqual(self.occlusion.fadeRate, 0.0)

    def test_clearing_returns_every_emitter_to_clear(self):
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.blocked)

        self.occlusion.ClearAllBlocked()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_disabling_clears_existing_values_and_ignores_feeds(self):
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.blocked)

        self.occlusion.enabled = False
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        # Feeds arriving while disabled must not re-occlude the emitter.
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_feed_for_an_unknown_emitter_is_safe(self):
        """The game queries at 5-10 Hz, so an emitter can start playing (or stop
        existing) between updates. Feeding an ID the engine does not know must not
        throw or leak; the state is dropped and the next feed re-establishes it."""
        unknownEmitterID = self.emitter.ID + 1000000

        self.occlusion.SetEmitterBlocked(unknownEmitterID, True)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(unknownEmitterID), 0.0)

    def test_a_clear_feed_for_an_unknown_emitter_is_safe(self):
        unknownEmitterID = self.emitter.ID + 1000000

        self.occlusion.SetEmitterBlocked(unknownEmitterID, False)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(unknownEmitterID), 0.0)

    def test_the_listener_itself_is_never_occluded(self):
        """Occlusion is measured relative to the listener, so occluding the listener is meaningless."""
        import audio2
        listenerID = audio2.GetListener().ID

        self.occlusion.SetEmitterBlocked(listenerID, True)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(listenerID), 0.0)

    def test_occlusion_is_suppressed_while_acoustics_are_enabled(self):
        """Acoustics transmission already attenuates, so occlusion must not stack on top of it."""
        manager = self.audioManager.manager
        manager.spatialAudioGeometryEnabled = True
        if not manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        try:
            self.occlusion.fadeRate = INSTANT_FADE_RATE

            self.Block()
            self.Pump()

            self.assertEqual(self.GetOcclusion(), 0.0)
        finally:
            manager.spatialAudioGeometryEnabled = False
