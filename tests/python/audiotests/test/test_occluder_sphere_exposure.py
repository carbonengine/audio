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

# A sightline source past BEYOND_EMITTER_SPHERE, looking back through it at the emitter.
SOURCE_BEYOND_SPHERE = (0, 0, 200)
# A sightline source with a clear line to the emitter.
SOURCE_BESIDE_EMITTER = (0, 200, 100)

SPHERE_RADIUS = 10.0
OCCLUDER_ID = 1

# Three occluders of which only the farthest sits on the sightline, so the scan has to
# keep looking past the two nearer ones instead of stopping at the first miss.
NEAR_MISS_SPHERE = (0, 30, 10)
MID_MISS_SPHERE = (0, 40, 20)
FAR_BLOCKING_SPHERE = (0, 0, 60)

# Long enough that no test wall-clock will reach it, so line of sight is only
# recomputed when something explicitly invalidates it.
LONG_RECOMPUTE_INTERVAL = 3600.0


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
        self.occlusion = audio2.GetOcclusion()

        self.listener = audio2.GetListener()
        self.listener.SetPosition(FRONT, TOP, LISTENER_POSITION)

        self.emitter = audio2.AudEmitter("occluderSphereTestEmitter")
        self.emitter.SetPlacement(FRONT, TOP, EMITTER_POSITION)

        self.occlusion.enabled = True
        self.occlusion.fadeRate = INSTANT_FADE_RATE
        # These tests place spheres exactly, so they test the radii they ask for rather
        # than the shipped scale. The tests that care about scaling set it themselves.
        self.occlusion.radiusScale = 1.0
        # Recompute line of sight on every update so tests do not depend on timing.
        self.occlusion.losRecomputeInterval = 0.0

    def tearDown(self):
        self.occlusion.SetOrigin(0.0, 0.0, 0.0)
        self.occlusion.ClearSightlineSource()
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

    def SetSphere(self, center, radius=SPHERE_RADIUS, occluderID=OCCLUDER_ID):
        self.occlusion.SetOccluderSphere(occluderID, center[0], center[1], center[2], radius)

    def test_sphere_between_listener_and_emitter_occludes(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

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

        self.occlusion.RemoveOccluderSphere(OCCLUDER_ID)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_clearing_all_spheres_clears_the_emitter(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.occlusion.ClearOccluderSpheres()
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

    def test_an_emitter_without_a_position_is_ignored(self):
        import audio2
        unpositioned = audio2.AudEmitter("occluderSphereUnpositionedEmitter")

        # A sphere over the origin covers the unpositioned emitter's default
        # placement, but an emitter that never received a position must not be
        # treated as world-space.
        self.SetSphere(LISTENER_POSITION, radius=SPHERE_RADIUS)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(unpositioned.ID), 0.0)

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

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

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

        self.occlusion.SetOrigin(*origin)
        self.SetSphere(world_sphere)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

    def test_a_stale_origin_does_not_strand_the_occluders(self):
        """The origin follows the player, so an occluder registered under one origin
        must be tested against the current one rather than the one it arrived with."""
        self.occlusion.SetOrigin(0.0, 0.0, 0.0)
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        # Shift audio space sideways by far more than the sphere's radius. The sphere
        # keeps its game world position, so it is no longer on the sightline.
        self.occlusion.SetOrigin(0.0, SPHERE_RADIUS * 100.0, 0.0)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_sphere_grazed_at_its_edge_does_not_occlude(self):
        """Registered radii are bounding spheres, far larger than the objects inside
        them, so clipping the outer edge of one is not cover."""
        # Offset sideways to 95% of the radius: inside the sphere as registered, outside
        # the fraction of it that counts as solid.
        graze = (0, SPHERE_RADIUS * 0.95, 50)

        self.occlusion.radiusScale = 0.75
        self.SetSphere(graze)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        # The same sphere at full radius does block, which is what makes this a graze
        # rather than a miss.
        self.occlusion.radiusScale = 1.0
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

    def test_a_radius_scale_of_zero_stops_occluding(self):
        self.occlusion.radiusScale = 0.0
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

        self.assertEqual(self.occlusion.GetEmitterOcclusion(distant.ID),
                         self.occlusion.blockedOcclusion)

    def test_an_emitter_at_solar_system_range_off_to_one_side_stays_clear(self):
        """The counterpart: distance alone must not produce blockage. Float precision holds
        up here, since the closest-point arithmetic stays proportional to the sphere rather
        than to the length of the sightline."""
        import audio2
        distant = audio2.AudEmitter("occluderSphereDistantClearEmitter")
        distant.SetPlacement(FRONT, TOP, (1.0e12, 0, 1.0e12))

        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()

        self.assertEqual(self.occlusion.GetEmitterOcclusion(distant.ID), 0.0)

    def test_blocked_occlusion_value_is_configurable(self):
        original = self.occlusion.blockedOcclusion
        try:
            self.occlusion.blockedOcclusion = 0.5
            self.SetSphere(BLOCKING_SPHERE)
            self.Pump()

            self.assertEqual(self.GetOcclusion(), 0.5)
        finally:
            self.occlusion.blockedOcclusion = original

    def test_registered_occluders_are_counted(self):
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 0)

        self.SetSphere(BLOCKING_SPHERE, occluderID=1)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 1)

        # The same ID again moves that occluder rather than adding another.
        self.SetSphere(OFF_AXIS_SPHERE, occluderID=1)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 1)

        self.SetSphere(BEYOND_EMITTER_SPHERE, occluderID=2)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 2)

        self.occlusion.RemoveOccluderSphere(1)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 1)

        self.SetSphere(BEYOND_EMITTER_SPHERE, radius=0.0, occluderID=2)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 0)

    def test_a_far_occluder_still_blocks_past_nearer_ones_that_miss(self):
        """Occluders are tested nearest surface first so an emitter can stop scanning
        once they are farther away than it is. A blocker beyond two nearer misses must
        still be found, or that early exit is cutting the scan short."""
        self.SetSphere(NEAR_MISS_SPHERE, occluderID=1)
        self.SetSphere(MID_MISS_SPHERE, occluderID=2)
        self.SetSphere(FAR_BLOCKING_SPHERE, occluderID=3)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

    def test_removing_the_blocker_clears_the_emitter_while_others_remain(self):
        """Only the last occluder leaving fades everything out directly; removing one of
        several has to reach the emitter through the next sightline pass instead."""
        self.SetSphere(NEAR_MISS_SPHERE, occluderID=1)
        self.SetSphere(FAR_BLOCKING_SPHERE, occluderID=3)
        self.Pump()
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.occlusion.RemoveOccluderSphere(3)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)
        self.assertEqual(self.occlusion.GetOccluderSphereCount(), 1)

    def test_occluders_blocking_at_least_one_emitter_are_listed(self):
        """One occluder blocks the emitter, one blocks nothing; only the blocker is
        listed. This is the one-call form built for QA overlays."""
        self.SetSphere(BLOCKING_SPHERE, occluderID=7)
        self.SetSphere(OFF_AXIS_SPHERE, occluderID=8)
        self.Pump()

        self.assertEqual(list(self.occlusion.GetBlockingOccluderIDs()), [7])

    def test_no_occluders_are_listed_once_the_sightline_clears(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertEqual(list(self.occlusion.GetBlockingOccluderIDs()), [OCCLUDER_ID])

        self.SetSphere(OFF_AXIS_SPHERE)
        self.Pump()

        self.assertEqual(list(self.occlusion.GetBlockingOccluderIDs()), [])

    def test_no_occluders_are_listed_while_occlusion_is_disabled(self):
        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertEqual(list(self.occlusion.GetBlockingOccluderIDs()), [OCCLUDER_ID])

        self.occlusion.enabled = False

        self.assertEqual(list(self.occlusion.GetBlockingOccluderIDs()), [])

    def test_the_blocking_occluder_can_be_identified(self):
        self.SetSphere(BLOCKING_SPHERE, occluderID=4242)
        self.Pump()

        self.assertEqual(self.occlusion.GetBlockingOccluder(self.emitter.ID), 4242)

    def test_a_clear_sightline_has_no_blocking_occluder(self):
        self.SetSphere(OFF_AXIS_SPHERE)
        self.Pump()

        self.assertEqual(self.occlusion.GetBlockingOccluder(self.emitter.ID), 0)

    def test_an_emitter_that_does_not_exist_has_no_blocking_occluder(self):
        self.SetSphere(BLOCKING_SPHERE)

        self.assertEqual(self.occlusion.GetBlockingOccluder(self.emitter.ID + 1000000), 0)

    def test_the_reported_origin_can_be_read_back(self):
        origin = (5524469439110.0, 15879459987.0, 1989687970679.0)

        self.occlusion.SetOrigin(*origin)

        self.assertEqual(self.occlusion.GetOriginX(), origin[0])
        self.assertEqual(self.occlusion.GetOriginY(), origin[1])
        self.assertEqual(self.occlusion.GetOriginZ(), origin[2])

    def test_the_position_audio_uses_for_a_game_object_can_be_read_back(self):
        """The audio-space position and its placedness are what separate a real blockage
        from two positions in mismatched coordinate spaces. The position arrives in
        Python as an (x, y, z) tuple."""
        self.assertEqual(self.emitter.position, EMITTER_POSITION)
        self.assertTrue(self.emitter.HasUsableWorldPosition())

    def test_an_unplaced_game_object_reports_no_usable_position(self):
        import audio2
        unpositioned = audio2.AudEmitter("occluderSphereUnplacedProbe")

        self.assertFalse(unpositioned.HasUsableWorldPosition())

    def test_a_moved_occluder_is_not_delayed_by_the_recompute_throttle(self):
        """Occluder edits invalidate line of sight, so they take effect on the next
        update however long the throttle interval is."""
        self.occlusion.losRecomputeInterval = LONG_RECOMPUTE_INTERVAL

        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

        self.SetSphere(OFF_AXIS_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_radius_scale_change_is_not_delayed_by_the_recompute_throttle(self):
        """Scaling changes every sightline's answer, so it also invalidates line of
        sight. Tuning it by ear in a live client depends on reacting immediately."""
        self.occlusion.losRecomputeInterval = LONG_RECOMPUTE_INTERVAL

        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

        self.occlusion.radiusScale = 0.0
        self.Pump()

        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_a_moved_origin_waits_for_the_recompute_throttle(self):
        """The origin follows the player every tick, so unlike occluder edits it must
        not force a recompute; it is picked up by the next scheduled pass."""
        self.occlusion.losRecomputeInterval = LONG_RECOMPUTE_INTERVAL

        self.SetSphere(BLOCKING_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

        # Far enough that the sphere would leave the sightline entirely once this is
        # taken into account, which the throttle is expected to postpone.
        self.occlusion.SetOrigin(0.0, SPHERE_RADIUS * 100.0, 0.0)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

    def test_a_sightline_source_replaces_the_listener_as_the_segment_start(self):
        """A sphere beyond the emitter is clear from the listener, but a source past the
        sphere looks back through it, so the same layout blocks once the source is set."""
        self.SetSphere(BEYOND_EMITTER_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        self.occlusion.SetSightlineSource(*SOURCE_BEYOND_SPHERE)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)
        self.assertEqual(self.occlusion.GetBlockingOccluder(self.emitter.ID), OCCLUDER_ID)

    def test_clearing_the_sightline_source_returns_to_the_listener(self):
        self.assertFalse(self.occlusion.HasSightlineSource())

        self.SetSphere(BEYOND_EMITTER_SPHERE)
        self.occlusion.SetSightlineSource(*SOURCE_BEYOND_SPHERE)
        self.Pump()
        self.assertTrue(self.occlusion.HasSightlineSource())
        self.assertGreater(self.GetOcclusion(), 0.0)

        self.occlusion.ClearSightlineSource()
        self.Pump()

        self.assertFalse(self.occlusion.HasSightlineSource())
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_the_sightline_source_is_given_in_game_world_space(self):
        """The source arrives in the same coordinates as occluder centres, so a ship
        position at solar system range only lines up once the origin relates the two."""
        origin = (5524469439110.0, 15879459987.0, 1989687970679.0)
        world_source = (origin[0] + SOURCE_BEYOND_SPHERE[0],
                        origin[1] + SOURCE_BEYOND_SPHERE[1],
                        origin[2] + SOURCE_BEYOND_SPHERE[2])
        world_sphere = (origin[0] + BEYOND_EMITTER_SPHERE[0],
                        origin[1] + BEYOND_EMITTER_SPHERE[1],
                        origin[2] + BEYOND_EMITTER_SPHERE[2])

        self.occlusion.SetOrigin(*origin)
        self.occlusion.SetSightlineSource(*world_source)
        self.SetSphere(world_sphere)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

    def test_flipping_the_sightline_source_is_not_delayed_by_the_recompute_throttle(self):
        """Switching perspective is done by ear mid-test, so setting or clearing the
        source must be audible on the next update however long the interval is."""
        self.occlusion.losRecomputeInterval = LONG_RECOMPUTE_INTERVAL

        self.SetSphere(BEYOND_EMITTER_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

        self.occlusion.SetSightlineSource(*SOURCE_BEYOND_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

        self.occlusion.ClearSightlineSource()
        self.Pump()
        self.assertEqual(self.GetOcclusion(), 0.0)

    def test_moving_an_existing_sightline_source_waits_for_the_recompute_throttle(self):
        """Once set, the source follows the ship every tick, so like the origin it must
        not force a recompute; it is picked up by the next scheduled pass."""
        self.occlusion.losRecomputeInterval = LONG_RECOMPUTE_INTERVAL

        self.occlusion.SetSightlineSource(*SOURCE_BEYOND_SPHERE)
        self.SetSphere(BEYOND_EMITTER_SPHERE)
        self.Pump()
        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)

        # A vantage the sphere would not block, which the throttle is expected
        # to postpone reacting to.
        self.occlusion.SetSightlineSource(*SOURCE_BESIDE_EMITTER)
        self.Pump()

        self.assertEqual(self.GetOcclusion(), self.occlusion.blockedOcclusion)
