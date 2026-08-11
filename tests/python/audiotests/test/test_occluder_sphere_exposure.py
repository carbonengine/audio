# Copyright © 2026 CCP ehf.

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
# On the segment's line but beyond the emitter.
BEYOND_EMITTER_SPHERE = (0, 0, 150)

SPHERE_RADIUS = 10.0
OCCLUDER_ID = 1


class TestOccluderSphereExposure(BaseAudio2TestClass):
    """Tests occluder-sphere line-of-sight occlusion: the engine computing every
    positioned emitter's occlusion itself from spheres the game registers."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK])

    def setUp(self):
        import audio2
        self.audioManager.Enable()
        self.manager = self.audioManager.manager

        self.listener = audio2.GetListener()
        self.listener.SetPosition(FRONT, TOP, LISTENER_POSITION)

        self.emitter = audio2.AudEmitter("occluderSphereTestEmitter")
        self.emitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.manager.obstructionOcclusionEnabled = True
        self.manager.obstructionOcclusionFadeRate = INSTANT_FADE_RATE
        # These tests place spheres exactly, so they test the radii they ask for rather
        # than the shipped scale. The tests that care about scaling set it themselves.
        self.manager.occluderRadiusScale = 1.0
        # Recompute line of sight on every update so tests do not depend on timing.
        self.manager.occluderLosRecomputeInterval = 0.0

    def tearDown(self):
        self.manager.SetOccluderOrigin(0.0, 0.0, 0.0)
        self.manager.ClearOccluderSpheres()
        self.emitter = None
        self.listener = None
        self.audioManager.Disable()

    def Pump(self, times=3):
        """Tick the engine. Each tick runs AudManager::Process(), which recomputes
        sphere line of sight and advances the occlusion fades."""
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=times)

    def GetOcclusion(self):
        return self.manager.GetEmitterOcclusion(self.emitter.ID)

    def SetSphere(self, center, radius=SPHERE_RADIUS, occluderID=OCCLUDER_ID):
        self.manager.SetOccluderSphere(occluderID, center[0], center[1], center[2], radius)

    def test_sphere_between_listener_and_emitter_occludes(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.manager.occluderBlockedOcclusion)

    def test_sphere_beside_the_segment_leaves_the_emitter_clear(self):
        self.SetSphere(OFF_AXIS_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_sphere_beyond_the_emitter_leaves_the_emitter_clear(self):
        self.SetSphere(BEYOND_EMITTER_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_removing_the_sphere_clears_the_emitter(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.manager.RemoveOccluderSphere(OCCLUDER_ID)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_clearing_all_spheres_clears_the_emitter(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.manager.ClearOccluderSpheres()
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_moving_the_sphere_off_the_segment_clears_the_emitter(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.SetSphere(OFF_AXIS_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_zero_radius_removes_the_occluder(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.SetSphere(BLOCKING_SPHERE, radius=0.0)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_manual_blockage_is_rejected_while_spheres_drive_occlusion(self):
        self.assertTrue(self.manager.SetEmitterLineOfSightBlockage(self.emitter.ID, 1.0))

        self.SetSphere(OFF_AXIS_SPHERE)
        self.assertFalse(self.manager.SetEmitterLineOfSightBlockage(self.emitter.ID, 1.0))

        self.manager.ClearOccluderSpheres()
        self.assertTrue(self.manager.SetEmitterLineOfSightBlockage(self.emitter.ID, 1.0))

    def test_an_emitter_without_a_position_is_ignored(self):
        import audio2
        unpositioned = audio2.AudEmitter("occluderSphereUnpositionedEmitter")

        # A sphere over the origin covers the unpositioned emitter's default
        # placement, but an emitter that never received a position must not be
        # treated as world-space.
        self.SetSphere(LISTENER_POSITION, radius=SPHERE_RADIUS)
        self.Pump()

        self.assertEqual(self.manager.GetEmitterOcclusion(unpositioned.ID), 0.0)

    def test_an_occluder_centred_on_the_emitter_does_not_occlude_it(self):
        """Occluder radii are coarse bounding spheres, so an emitter attached to an
        object sits inside the object's own sphere. An emitter at the centre is not
        behind anything, and counting it would occlude turrets and beams permanently."""
        self.SetSphere(EMITTER_POSITION, radius=SPHERE_RADIUS)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_an_emitter_in_the_near_half_of_an_occluder_is_clear(self):
        """The near side of an object is in plain sight. This is the mounted-turret case:
        the emitter is inside its own object's bounding sphere, facing the listener."""
        # Centred past the emitter, so the emitter sits in front of the centre.
        self.SetSphere((0, 0, EMITTER_POSITION[2] + SPHERE_RADIUS * 0.5))
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_an_emitter_in_the_far_half_of_an_occluder_is_blocked(self):
        """The far side is behind the object. This is a beam terminating against the back
        of a wall: inside the wall's bounding sphere, but with the wall in between."""
        # Centred short of the emitter, so the emitter sits past the centre.
        self.SetSphere((0, 0, EMITTER_POSITION[2] - SPHERE_RADIUS * 0.5))
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.manager.occluderBlockedOcclusion)

    def test_an_occluder_around_the_listener_does_not_occlude_anything(self):
        """Flying inside a large occluder must not muffle the whole world."""
        self.SetSphere(LISTENER_POSITION, radius=SPHERE_RADIUS * 10.0)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_an_occluder_swallowing_the_whole_sightline_does_not_occlude(self):
        """A sphere containing both ends contains the sightline; there is nothing
        for it to block."""
        self.SetSphere(BLOCKING_SPHERE, radius=10000.0)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_occluders_are_given_in_game_world_space(self):
        """Occluder centres arrive in the game's own coordinates, which are far too
        large for the float positions emitters use, and are translated into audio
        space against the reported origin. Without that translation an occluder can
        never line up with an emitter."""
        # A solar system reaches this far out, where a float has ~500m granularity.
        origin = (5524469439110.0, 15879459987.0, 1989687970679.0)
        world_sphere = (origin[0] + BLOCKING_SPHERE[0],
                        origin[1] + BLOCKING_SPHERE[1],
                        origin[2] + BLOCKING_SPHERE[2])

        self.manager.SetOccluderOrigin(*origin)
        self.SetSphere(world_sphere)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.manager.occluderBlockedOcclusion)

    def test_a_stale_origin_does_not_strand_the_occluders(self):
        """The origin follows the player, so an occluder registered under one origin
        must be tested against the current one rather than the one it arrived with."""
        self.manager.SetOccluderOrigin(0.0, 0.0, 0.0)
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        # Shift audio space sideways by far more than the sphere's radius. The sphere
        # keeps its game world position, so it is no longer on the sightline.
        self.manager.SetOccluderOrigin(0.0, SPHERE_RADIUS * 100.0, 0.0)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_sphere_grazed_at_its_edge_does_not_occlude(self):
        """Registered radii are bounding spheres, far larger than the objects inside
        them, so clipping the outer edge of one is not cover."""
        # Offset sideways to 95% of the radius: inside the sphere as registered, outside
        # the fraction of it that counts as solid.
        graze = (0, SPHERE_RADIUS * 0.95, 50)

        self.manager.occluderRadiusScale = 0.75
        self.SetSphere(graze)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        # The same sphere at full radius does block, which is what makes this a graze
        # rather than a miss.
        self.manager.occluderRadiusScale = 1.0
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.manager.occluderBlockedOcclusion)

    def test_a_radius_scale_of_zero_stops_occluding(self):
        self.manager.occluderRadiusScale = 0.0
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_an_emitter_at_solar_system_range_is_blocked_by_a_near_sphere(self):
        """Emitters on stars and planets sit at around 1e12 metres, and a sightline that
        long still gets a straight geometric answer: a sphere sitting on it blocks it. Note
        what this means in a field - such a sightline crosses every occluder near the
        player, so distant emitters are blocked far more readily than close ones."""
        import audio2
        distant = audio2.AudEmitter("occluderSphereDistantEmitter")
        # Straight out past the sphere along the same axis, at solar system scale.
        distant.SetPlacement(FRONT, TOP, (0, 0, 1.0e12))

        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.manager.GetEmitterOcclusion(distant.ID),
                         self.manager.occluderBlockedOcclusion)

    def test_an_emitter_at_solar_system_range_off_to_one_side_stays_clear(self):
        """The counterpart: distance alone must not produce blockage. Float precision holds
        up here, since the closest-point arithmetic stays proportional to the sphere rather
        than to the length of the sightline."""
        import audio2
        distant = audio2.AudEmitter("occluderSphereDistantClearEmitter")
        distant.SetPlacement(FRONT, TOP, (1.0e12, 0, 1.0e12))

        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.manager.GetEmitterOcclusion(distant.ID), 0.0)

    def test_blocked_occlusion_value_is_configurable(self):
        self.manager.occluderBlockedOcclusion = 0.5
        try:
            self.SetSphere(BLOCKING_SPHERE)
            self.Pump()

            self.assertEqual(self.GetOcclusion(), 0.5)
        finally:
            self.manager.occluderBlockedOcclusion = 0.35
