# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import BaseAudio2TestClass


ORIGIN = (0.0, 0.0, 0.0)
BOX_SIZE = (20.0, 20.0, 20.0)
HULL = 1


class TestGeometryWorldFrameExposure(BaseAudio2TestClass):
    """Tests that trinity's meshes go to Wwise in the world frame, so a static mesh is sent once while the ego moves."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        import audio2
        import blue
        self.audioManager.Enable()
        self.manager = self.audioManager.manager
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        audio2.GetListener().SetPosition((0.0, 0.0, 1.0), (0.0, 1.0, 0.0), ORIGIN)
        extension = blue.LoadExtension("_audiotests")
        self.trinity = extension.FakeGeometrySource()
        self.reference = extension.FakeAcousticGeometrySource()
        self.reference.SetOrigin(0.0, 0.0, 0.0)
        self.manager.acousticGeometrySource = self.reference
        self.egoX = 0.0
        # World x of each box, by instance.
        self.boxes = {}

    def tearDown(self):
        for instanceId in self.boxes:
            self.trinity.Remove(HULL, instanceId)
        self.manager.acousticGeometrySource = None
        self.Update()
        self.manager.spatialAudioGeometryEnabled = False
        self.audioManager.Disable()

    def Update(self, times=1):
        import blue
        for _ in range(times):
            blue.pyos.synchro.Yield()

    def AddBox(self, instanceId, worldX):
        self.boxes[instanceId] = worldX
        self.trinity.AddBox(HULL, instanceId, (worldX - self.egoX, 0.0, 0.0), BOX_SIZE)

    def PushScene(self):
        """Trinity's scene is centred on the ego, so a static box moves the other way in it."""
        for instanceId, worldX in self.boxes.items():
            self.trinity.Move(HULL, instanceId, (worldX - self.egoX, 0.0, 0.0), BOX_SIZE)

    def MoveEgo(self, egoX):
        """As in the client: destiny has the reference before audio's tick, and trinity renders that frame after it."""
        self.egoX = egoX
        self.reference.SetOrigin(egoX, 0.0, 0.0)
        self.Update()
        self.PushScene()
        self.Update()

    def MoveEgoSlipped(self, egoX):
        """Trinity renders the frame before audio's tick has the reference for it."""
        self.egoX = egoX
        self.reference.SetOrigin(egoX, 0.0, 0.0)
        self.PushScene()
        self.Update()

    def test_trinity_is_the_geometry_unless_collision_is_asked_for(self):
        self.assertFalse(self.manager.collisionGeometryEnabled)
        self.AddBox(1, 100.0)
        self.Update()

        self.assertTrue(self.manager.worldFrameActive)
        self.assertEqual(self.manager.geometryInstancesInWwise, 1)
        self.assertEqual(self.manager.acousticBodies, 0)

    def test_a_static_mesh_is_not_sent_again_while_the_ego_moves(self):
        """The regression this exists for: trinity's ego-centred scene used to resend every static mesh every frame."""
        self.AddBox(1, 100.0)
        self.Update()
        sets = self.manager.geometrySetsSent
        instances = self.manager.geometryInstancesSent

        for step in range(1, 31):
            self.MoveEgo(100.0 * step)

        self.assertEqual(self.manager.geometryInstancesInWwise, 1)
        self.assertEqual(self.manager.geometrySetsSent, sets)
        self.assertEqual(self.manager.geometryInstancesSent, instances)
        self.assertEqual(self.manager.worldFrameRebases, 0)

    def test_a_static_mesh_far_from_the_ego_is_not_sent_again_for_float_rounding(self):
        self.AddBox(1, 30000.0)
        self.Update()
        instances = self.manager.geometryInstancesSent

        for step in range(1, 11):
            self.MoveEgo(0.37 * step)

        self.assertEqual(self.manager.geometryInstancesSent, instances)

    def test_a_mesh_that_moves_in_the_world_is_sent_again(self):
        self.AddBox(1, 100.0)
        self.Update()
        instances = self.manager.geometryInstancesSent

        self.boxes[1] = 105.0
        self.MoveEgo(50.0)

        self.assertEqual(self.manager.geometryInstancesSent, instances + 1)

    def test_a_rebase_sends_instances_again_but_not_sets(self):
        self.AddBox(1, 100.0)
        self.Update()
        sets = self.manager.geometrySetsSent
        instances = self.manager.geometryInstancesSent

        self.MoveEgo(150000.0)

        self.assertEqual(self.manager.worldFrameRebases, 1)
        self.assertEqual(self.manager.geometrySetsSent, sets)
        self.assertEqual(self.manager.geometryInstancesSent, instances + 1)

    def test_a_rebase_moves_every_mesh_and_is_not_taken_for_a_slip(self):
        """Every instance moves by the same amount in a rebase, which looks like a slip but has to reach Wwise."""
        for instanceId in range(1, 5):
            self.AddBox(instanceId, 100.0 * instanceId)
        self.Update()
        instances = self.manager.geometryInstancesSent
        slips = self.manager.geometrySlips

        self.MoveEgo(150000.0)

        self.assertEqual(self.manager.worldFrameRebases, 1)
        self.assertEqual(self.manager.geometrySlips, slips)
        self.assertEqual(self.manager.geometryInstancesSent, instances + 4)

        # Back in step after the rebase: nothing more goes to Wwise as the ego keeps moving.
        instances = self.manager.geometryInstancesSent
        self.MoveEgo(150100.0)
        self.assertEqual(self.manager.geometryInstancesSent, instances)

    def test_a_frame_where_trinity_slips_against_the_reference_sends_nothing(self):
        for instanceId in range(1, 5):
            self.AddBox(instanceId, 100.0 * instanceId)
        self.Update()
        instances = self.manager.geometryInstancesSent
        slips = self.manager.geometrySlips

        self.MoveEgoSlipped(300.0)

        self.assertEqual(self.manager.geometryInstancesSent, instances)
        self.assertEqual(self.manager.geometrySlips, slips + 1)

    def test_one_mesh_moving_among_static_ones_is_sent(self):
        for instanceId in range(1, 5):
            self.AddBox(instanceId, 100.0 * instanceId)
        self.Update()
        instances = self.manager.geometryInstancesSent
        slips = self.manager.geometrySlips

        self.boxes[2] = 220.0
        self.PushScene()
        self.Update()

        self.assertEqual(self.manager.geometryInstancesSent, instances + 1)
        self.assertEqual(self.manager.geometrySlips, slips)
