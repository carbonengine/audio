# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import COMMON_BNK
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout


INSTANT_FADE_RATE = 0.0
SLOW_FADE_RATE = 0.01
FAST_FADE_RATE = 100.0

FRONT = (0, 0, 1)
TOP = (0, 1, 0)

LISTENER_POSITION = (0, 0, 0)
EMITTER_POSITION = (0, 0, 100)

# On the listener-emitter segment.
BLOCKING_SPHERE = (0, 0, 50)
# Beside the segment. Moving the occluder here returns the emitter's target to
# clear while keeping it tracked, so the fade towards clear can be observed.
OFF_AXIS_SPHERE = (0, 200, 50)

SPHERE_RADIUS = 10.0
OCCLUDER_ID = 1


class TestObstructionOcclusionExposure(BaseAudio2TestClass):
    """Tests the occlusion fade machinery: the interpolation that moves every
    emitter's value towards the target the occluder spheres set for it."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK])

    def setUp(self):
        import audio2
        self.audioManager.Enable()
        self.occlusion = audio2.GetOcclusion()

        self.listener = audio2.GetListener()
        self.listener.SetPosition(FRONT, TOP, LISTENER_POSITION)

        self.emitter = audio2.AudEmitter("occlusionTestEmitter")
        self.emitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.occlusion.enabled = True
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        # These tests place spheres exactly, so they test the radii they ask for
        # rather than the shipped scale.
        self.occlusion.radiusScale = 1.0
        # Recompute line of sight on every update so tests do not depend on timing.
        self.occlusion.losRecomputeInterval = 0.0
        # The value a blocked emitter fades towards.
        self.blocked = self.occlusion.blockedOcclusion

    def tearDown(self):
        self.occlusion.SetOrigin(0.0, 0.0, 0.0)
        self.occlusion.ClearOccluderSpheres()
        self.emitter = None
        self.listener = None
        self.occlusion = None
        self.audioManager.Disable()

    def Pump(self, times=3):
        """Tick the engine. Each tick runs AudManager::Process(), which recomputes
        sphere line of sight and advances the occlusion fades."""
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=times)

    def GetOcclusion(self):
        return self.occlusion.GetEmitterOcclusion(self.emitter.ID)

    def Block(self):
        """Place the occluder on the listener-emitter segment."""
        self.occlusion.SetOccluderSphere(OCCLUDER_ID, *BLOCKING_SPHERE, SPHERE_RADIUS)

    def Unblock(self):
        """Move the occluder beside the segment, clearing the emitter's target
        without ending sphere occlusion."""
        self.occlusion.SetOccluderSphere(OCCLUDER_ID, *OFF_AXIS_SPHERE, SPHERE_RADIUS)

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

    def test_disabling_clears_existing_values_and_stops_the_spheres_driving(self):
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        self.Block()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.blocked)

        self.occlusion.enabled = False
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        # The occluder is still registered, but with processing disabled it must not
        # re-occlude the emitter.
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_the_listener_itself_is_never_occluded(self):
        """Occlusion is measured relative to the listener, so occluding the listener is meaningless."""
        import audio2
        listenerID = audio2.GetListener().ID

        self.Block()
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
