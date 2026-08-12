# Copyright © 2026 CCP ehf.

import unittest

from audiotests.base_test_class import COMMON_BNK
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout


INSTANT_FADE_RATE = 0.0

FRONT = (0, 0, 1)
TOP = (0, 1, 0)

LISTENER_POSITION = (0, 0, 0)
EMITTER_POSITION = (0, 0, 100)

# On the listener-emitter segment.
BLOCKING_SPHERE = (0, 0, 50)
# Beside the segment.
OFF_AXIS_SPHERE = (0, 200, 50)

SPHERE_RADIUS = 10.0
OCCLUDER_ID = 1


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

        self.listener = audio2.GetListener()
        self.listener.SetPosition(FRONT, TOP, LISTENER_POSITION)

        self.emitter = audio2.AudEmitter("pythonOcclusionTestEmitter")
        self.emitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.defaults = {
            "enabled": self.occlusion.GetEnabled(),
            "fadeRate": self.occlusion.GetFadeRate(),
            "blockedOcclusion": self.occlusion.GetBlockedOcclusion(),
            "radiusScale": self.occlusion.GetRadiusScale(),
            "losRecomputeInterval": self.occlusion.GetLosRecomputeInterval(),
        }

        self.occlusion.SetEnabled(True)
        self.occlusion.SetFadeRate(INSTANT_FADE_RATE)
        self.occlusion.SetRadiusScale(1.0)
        self.occlusion.SetLosRecomputeInterval(0.0)

    def tearDown(self):
        # These settings are global, so hand them back as they were found rather than
        # leaking this class's values into whatever runs next.
        self.occlusion.SetOrigin(0.0, 0.0, 0.0)
        self.occlusion.ClearSightlineSource()
        self.occlusion.ClearOccluderSpheres()
        self.occlusion.SetEnabled(self.defaults["enabled"])
        self.occlusion.SetFadeRate(self.defaults["fadeRate"])
        self.occlusion.SetBlockedOcclusion(self.defaults["blockedOcclusion"])
        self.occlusion.SetRadiusScale(self.defaults["radiusScale"])
        self.occlusion.SetLosRecomputeInterval(self.defaults["losRecomputeInterval"])
        self.emitter = None
        self.listener = None
        self.occlusion = None
        self.audioManager.Disable()

    def Pump(self, times=3):
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=times)

    def SetSphere(self, center, radius=SPHERE_RADIUS, occluderID=OCCLUDER_ID):
        self.occlusion.SetOccluderSphere(occluderID, center[0], center[1], center[2], radius)

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

    def test_radius_scale_round_trips(self):
        self.occlusion.SetRadiusScale(0.25)

        self.assertEqual(self.occlusion.GetRadiusScale(), 0.25)

    def test_los_recompute_interval_round_trips(self):
        self.occlusion.SetLosRecomputeInterval(0.5)

        self.assertEqual(self.occlusion.GetLosRecomputeInterval(), 0.5)

    def test_occluders_can_be_registered_and_removed(self):
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 0)

        self.SetSphere(BLOCKING_SPHERE, occluderID=1)
        self.SetSphere(OFF_AXIS_SPHERE, occluderID=2)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 2)

        self.occlusion.RemoveOccluderSphere(1)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 1)

        self.occlusion.ClearOccluderSpheres()
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 0)

    def test_a_sphere_registered_through_the_wrapper_occludes_an_emitter(self):
        """End to end through the wrapper: register a blocker, tick, read the emitter."""
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID),
                         self.occlusion.GetBlockedOcclusion())

    def test_the_origin_reaches_the_engine_through_the_wrapper(self):
        """Occluder centres are game world coordinates, so a sphere registered far out
        only lines up with the emitter once the matching origin is reported."""
        origin = (5524469439110.0, 15879459987.0, 1989687970679.0)
        worldSphere = (origin[0] + BLOCKING_SPHERE[0],
                       origin[1] + BLOCKING_SPHERE[1],
                       origin[2] + BLOCKING_SPHERE[2])

        self.occlusion.SetOrigin(*origin)
        self.SetSphere(worldSphere)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID),
                         self.occlusion.GetBlockedOcclusion())

    def test_the_blocking_occluder_is_reachable_through_the_wrapper(self):
        self.SetSphere(BLOCKING_SPHERE, occluderID=4242)
        self.Pump()

        self.assertEqual(self.occlusion.GetBlockingOccluder(self.emitter.ID), 4242)
        self.assertEqual(list(self.occlusion.GetBlockingOccluderIDs()), [4242])

    def test_the_origin_can_be_read_back_through_the_wrapper(self):
        origin = (5524469439110.0, 15879459987.0, 1989687970679.0)

        self.occlusion.SetOrigin(*origin)

        self.assertEqual(self.occlusion.GetOrigin(), origin)

    def test_the_sightline_source_flips_through_the_wrapper(self):
        """End to end: a sphere beyond the emitter blocks nothing from the listener, but
        does from a source past it, and clearing the source flips straight back."""
        # On the segment's line but beyond the emitter, then a source past it again.
        beyondSphere = (0, 0, 150)
        sourceBeyond = (0, 0, 200)

        self.assertFalse(self.occlusion.HasSightlineSource())

        self.SetSphere(beyondSphere)
        self.occlusion.SetSightlineSource(*sourceBeyond)
        self.Pump()

        self.assertTrue(self.occlusion.HasSightlineSource())
        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID),
                         self.occlusion.GetBlockedOcclusion())

        self.occlusion.ClearSightlineSource()
        self.Pump()

        self.assertFalse(self.occlusion.HasSightlineSource())
        self.assertEqual(self.occlusion.GetEmitterOcclusion(self.emitter.ID), 0.0)

    def test_the_wrapper_is_a_view_onto_the_one_occlusion_subsystem(self):
        """The wrapper must not stand up state of its own: a second wrapper, and the
        Blue object itself, have to see what this one did."""
        import audio2
        from audio2.occlusion import OcclusionManager

        self.SetSphere(BLOCKING_SPHERE)
        self.occlusion.SetBlockedOcclusion(0.5)

        self.assertEqual(audio2.GetOcclusion().GetOccluderSphereCount(), 1)
        self.assertEqual(audio2.GetOcclusion().blockedOcclusion, 0.5)
        self.assertEqual(OcclusionManager().GetOccluderSphereCount(), 1)
        self.assertEqual(OcclusionManager().GetBlockedOcclusion(), 0.5)


if __name__ == "__main__":
    unittest.main()
