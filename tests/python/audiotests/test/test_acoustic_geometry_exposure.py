# Copyright © 2026 CCP ehf.

import math

from audiotests.base_test_class import BaseAudio2TestClass


BOX_TRIANGLES = 12
ORIGIN = (0.0, 0.0, 0.0)
# Longer than the selection interval of 250 ms.
SELECTION_WAIT_MS = 400

BOX_MESH = 10
OTHER_BOX_MESH = 11
BAD_MESH = 12
SHAPE = 100
OTHER_SHAPE = 101
BAD_SHAPE = 102

SETTINGS = (
    "geometryMaxDistance",
    "geometryMaxTriangles",
    "transmissionLoss",
)


class TestAcousticGeometryExposure(BaseAudio2TestClass):
    """Tests that destiny's convex collision reaches Wwise in the world frame, sent once while nothing moves."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        import audio2
        import blue
        self.audioManager.Enable()
        self.manager = self.audioManager.manager
        self.settings = {name: getattr(self.manager, name) for name in SETTINGS}
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        audio2.GetListener().SetPosition((0.0, 0.0, 1.0), (0.0, 1.0, 0.0), ORIGIN)
        self.source = blue.LoadExtension("_audiotests").FakeAcousticGeometrySource()
        self.source.DefineBoxMesh(BOX_MESH, 20.0, 20.0, 20.0)
        self.source.AddComponent(SHAPE, BOX_MESH, 0.0, 0.0, 0.0, 0.0)
        self.source.SetOrigin(0.0, 0.0, 0.0)
        self.manager.acousticGeometrySource = self.source
        self.manager.collisionGeometryEnabled = True

    def tearDown(self):
        self.manager.acousticGeometrySource = None
        self.manager.collisionGeometryEnabled = False
        self.Update()
        for name, value in self.settings.items():
            setattr(self.manager, name, value)
        self.manager.spatialAudioGeometryEnabled = False
        self.audioManager.Disable()

    def Update(self, times=1):
        import blue
        for _ in range(times):
            blue.pyos.synchro.Yield()

    def UpdateAfterSelection(self):
        import blue
        blue.pyos.synchro.SleepWallclock(SELECTION_WAIT_MS)
        self.Update()

    def AssertInWwise(self, sets, instances):
        self.assertEqual(self.manager.geometrySetsInWwise, sets)
        self.assertEqual(self.manager.geometryInstancesInWwise, instances)

    def InstancePosition(self, bodyId):
        return tuple(self.manager.GetAcousticInstanceWwisePosition(bodyId))

    def test_nothing_reaches_wwise_without_a_render_reference(self):
        self.source.ClearOrigin()
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.assertFalse(self.manager.worldFrameActive)
        self.AssertInWwise(sets=0, instances=0)

    def test_a_body_reaches_wwise_on_the_next_tick(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.assertTrue(self.manager.worldFrameActive)
        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, BOX_TRIANGLES)
        self.assertEqual(self.manager.acousticBodies, 1)

    def test_a_static_body_is_not_sent_again_while_the_ego_moves(self):
        """The regression this design exists for: trinity's ego-relative frame used to resend everything every frame."""
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()
        sets = self.manager.geometrySetsSent
        instances = self.manager.geometryInstancesSent

        for step in range(1, 31):
            self.source.SetOrigin(100.0 * step, 0.0, 0.0)
            self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometrySetsSent, sets)
        self.assertEqual(self.manager.geometryInstancesSent, instances)
        self.assertEqual(self.manager.worldFrameRebases, 0)

    def test_the_listener_keeps_its_world_position_as_the_ego_moves(self):
        """The listener is placed once in the scene; the ego moving moves it in the world, and Wwise has to follow."""
        import audio2
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.source.SetOrigin(250.0, 0.0, -40.0)
        self.Update()

        position = tuple(audio2.GetListener().wwisePosition)
        self.assertAlmostEqual(position[0], 250.0, places=3)
        self.assertAlmostEqual(position[1], 0.0, places=3)
        # Wwise is left-handed, so Z is mirrored.
        self.assertAlmostEqual(position[2], 40.0, places=3)

    def test_a_body_that_moves_is_sent_once(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()
        instances = self.manager.geometryInstancesSent

        self.source.AddBody(1, SHAPE, 100.0004, 0.0, 0.0, 0.0)
        self.UpdateAfterSelection()
        self.assertEqual(self.manager.geometryInstancesSent, instances, "A move below the tolerance was sent.")

        self.source.AddBody(1, SHAPE, 105.0, 0.0, 0.0, 0.0)
        self.UpdateAfterSelection()
        self.Update(3)
        self.assertEqual(self.manager.geometryInstancesSent, instances + 1)
        self.assertAlmostEqual(self.InstancePosition(1)[0], 105.0, places=3)

    def test_bodies_with_one_shape_share_its_set(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.source.AddBody(2, SHAPE, -100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.AssertInWwise(sets=1, instances=2)
        self.assertEqual(self.source.meshCalls, 1)
        self.assertEqual(self.source.componentsCalls, 1)

    def test_an_instance_is_placed_by_its_body_and_component(self):
        """Body at x 100 turned 90 degrees about Y, component 10 along its X: the component lands at z -10, mirrored in Wwise."""
        self.source.AddComponent(OTHER_SHAPE, BOX_MESH, 10.0, 0.0, 0.0, 0.0)
        self.source.AddBody(1, OTHER_SHAPE, 100.0, 0.0, 0.0, math.pi / 2.0)
        self.Update()

        position = self.InstancePosition(1)
        self.assertAlmostEqual(position[0], 100.0, places=3)
        self.assertAlmostEqual(position[1], 0.0, places=3)
        self.assertAlmostEqual(position[2], 10.0, places=3)

    def test_a_rebase_sends_instances_again_but_not_sets(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()
        sets = self.manager.geometrySetsSent
        instances = self.manager.geometryInstancesSent

        self.source.SetOrigin(150000.0, 0.0, 0.0)
        self.Update()

        self.assertEqual(self.manager.worldFrameRebases, 1)
        self.assertEqual(self.manager.geometrySetsSent, sets)
        self.assertEqual(self.manager.geometryInstancesSent, instances + 1)
        self.assertAlmostEqual(self.InstancePosition(1)[0], 100.0 - 150000.0, places=2)

    def test_a_new_generation_fetches_everything_again(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()
        meshCalls = self.source.meshCalls

        self.source.generation = self.source.generation + 1
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.source.meshCalls, meshCalls + 1)

    def test_the_triangle_budget_keeps_the_nearest_body(self):
        self.manager.geometryMaxTriangles = BOX_TRIANGLES
        self.source.AddBody(1, SHAPE, 2000.0, 0.0, 0.0, 0.0)
        self.source.AddBody(2, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertTrue(math.isfinite(self.InstancePosition(2)[0]))
        self.assertFalse(math.isfinite(self.InstancePosition(1)[0]))

    def test_a_body_beyond_the_radius_waits_outside_wwise(self):
        self.manager.geometryMaxDistance = 500.0
        self.source.AddBody(1, SHAPE, 2000.0, 0.0, 0.0, 0.0)
        self.source.AddBody(2, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertTrue(math.isfinite(self.InstancePosition(2)[0]))

    def test_a_removed_body_leaves_wwise(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.source.RemoveBody(1)
        self.UpdateAfterSelection()

        self.assertEqual(self.manager.geometryInstancesInWwise, 0)
        self.assertEqual(self.manager.acousticBodies, 0)

    def test_a_bad_mesh_is_left_out_without_affecting_the_rest(self):
        self.source.DefineBadMesh(BAD_MESH)
        self.source.AddComponent(BAD_SHAPE, BAD_MESH, 0.0, 0.0, 0.0, 0.0)
        self.source.AddBody(1, BAD_SHAPE, 50.0, 0.0, 0.0, 0.0)
        self.source.AddBody(2, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()
        meshCalls = self.source.meshCalls

        self.UpdateAfterSelection()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.source.meshCalls, meshCalls, "The bad mesh was fetched again.")

    def test_clearing_the_source_takes_everything_out(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()

        self.manager.acousticGeometrySource = None
        self.Update()

        self.assertFalse(self.manager.worldFrameActive)
        self.AssertInWwise(sets=0, instances=0)

    def test_trinity_meshes_stay_out_of_wwise_while_collision_is_the_geometry(self):
        import blue
        trinity = blue.LoadExtension("_audiotests").FakeGeometrySource()
        trinity.AddBox(7, 7, ORIGIN, (10.0, 10.0, 10.0))
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        try:
            self.Update()
            self.AssertInWwise(sets=1, instances=1)
            self.assertEqual(self.manager.acousticBodies, 1)

            self.manager.collisionGeometryEnabled = False
            self.Update()
            self.AssertInWwise(sets=1, instances=1)
            self.assertEqual(self.manager.acousticBodies, 0)
            self.assertTrue(self.manager.worldFrameActive)
        finally:
            trinity.Remove(7, 7)

    def test_the_defaults_match_unreal(self):
        self.assertFalse(self.manager.enableDiffractionOnBoundaryEdges)
        self.assertFalse(self.manager.geometrySolid)
    def test_changing_a_mesh_setting_sends_each_set_once(self):
        self.source.AddBody(1, SHAPE, 100.0, 0.0, 0.0, 0.0)
        self.Update()
        sets = self.manager.geometrySetsSent

        self.manager.transmissionLoss = 0.5
        self.Update(3)

        self.assertEqual(self.manager.geometrySetsSent, sets + 1)
