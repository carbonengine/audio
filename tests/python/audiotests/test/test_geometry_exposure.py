# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import BaseAudio2TestClass


BOX_TRIANGLES = 12
ORIGIN = (0.0, 0.0, 0.0)
ELSEWHERE = (500.0, 0.0, 0.0)
BOX_SIZE = (10.0, 20.0, 30.0)
MIRRORED_SIZE = (-10.0, 20.0, 30.0)
# Where trinity parks an object that left the scene.
PARKED = (0.0, -3.4028234663852886e38, 0.0)
SOLAR_SYSTEM_DISTANCE = (-9.4e11, 6.3e10, 8.6e11)
FLAT_SIZE = (10.0, 0.0, 30.0)

HULL = 1
OTHER_HULL = 2

SETTINGS = (
    "transmissionLoss",
    "enableDiffractionOnBoundaryEdges",
    "geometrySolid",
    "geometryMaxDistance",
    "geometryMaxTriangles",
)


def SaveSettings(manager):
    return {name: getattr(manager, name) for name in SETTINGS}


def RestoreSettings(manager, settings):
    for name, value in settings.items():
        setattr(manager, name, value)


class TestGeometryExposure(BaseAudio2TestClass):
    """Tests that meshes registered through AudGeometry reach Wwise, and stay registered while Wwise is not available."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        import blue
        self.audioManager.Enable()
        self.manager = self.audioManager.manager
        self.settings = SaveSettings(self.manager)
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        self.source = blue.LoadExtension("_audiotests").FakeGeometrySource()
        self.assertTrue(self.source.IsConnected(), "AudGeometry could not be created the way trinity creates it.")
        self.registered = []

    def tearDown(self):
        for geometrySetId, instanceId in self.registered:
            self.source.Remove(geometrySetId, instanceId)
        self.Update()
        self.source = None
        RestoreSettings(self.manager, self.settings)
        self.manager.spatialAudioGeometryEnabled = False
        self.audioManager.Disable()

    def Update(self, times=1):
        """Let the engine run one frame per time, which runs AudManager::Process() once."""
        import blue
        for _ in range(times):
            blue.pyos.synchro.Yield()

    def AddBox(self, geometrySetId, instanceId, center=ORIGIN, size=BOX_SIZE):
        self.source.AddBox(geometrySetId, instanceId, center, size)
        self.registered.append((geometrySetId, instanceId))

    def AddTriangle(self, geometrySetId, instanceId, center=ORIGIN, size=BOX_SIZE):
        self.source.AddTriangle(geometrySetId, instanceId, center, size)
        self.registered.append((geometrySetId, instanceId))

    def Remove(self, geometrySetId, instanceId):
        self.source.Remove(geometrySetId, instanceId)
        self.registered.remove((geometrySetId, instanceId))

    def AssertInWwise(self, sets, instances):
        self.assertEqual(self.manager.geometrySetsInWwise, sets)
        self.assertEqual(self.manager.geometryInstancesInWwise, instances)

    def test_a_registered_mesh_reaches_wwise_on_the_next_tick(self):
        self.AddBox(HULL, 1)
        self.AssertInWwise(sets=0, instances=0)

        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, BOX_TRIANGLES)

    def test_instances_share_their_geometry_set(self):
        self.AddBox(HULL, 1)
        self.AddBox(HULL, 2, center=ELSEWHERE)
        self.AddBox(OTHER_HULL, 3)
        self.Update()

        self.AssertInWwise(sets=2, instances=3)
        self.assertEqual(self.manager.geometryTrianglesInWwise, 3 * BOX_TRIANGLES)

    def test_a_set_leaves_wwise_with_its_last_instance(self):
        self.AddBox(HULL, 1)
        self.AddBox(HULL, 2, center=ELSEWHERE)
        self.Update()

        self.Remove(HULL, 1)
        self.Update()
        self.AssertInWwise(sets=1, instances=1)

        self.Remove(HULL, 2)
        self.Update()
        self.AssertInWwise(sets=0, instances=0)

    def test_an_instance_can_be_removed_and_registered_again_in_one_tick(self):
        self.AddBox(HULL, 1)
        self.Update()

        self.Remove(HULL, 1)
        self.AddBox(HULL, 1, center=ELSEWHERE)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)

    def test_an_instance_can_move_to_another_geometry_set(self):
        """A resource reload gives the mesh a new geometry set while the instance keeps its ID."""
        self.AddBox(HULL, 1)
        self.Update()

        self.source.AddTriangle(OTHER_HULL, 1, ORIGIN, BOX_SIZE)
        self.registered = [(OTHER_HULL, 1)]
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, 1)

    def test_moving_and_mirroring_an_instance_keeps_it_in_wwise(self):
        self.AddBox(HULL, 1)
        self.Update()

        self.source.Move(HULL, 1, ELSEWHERE, MIRRORED_SIZE)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)

    def test_an_instance_parked_outside_the_world_leaves_wwise_until_it_returns(self):
        self.AddBox(HULL, 1)
        self.AddBox(HULL, 2, center=ELSEWHERE)
        self.Update()

        for unusable in (PARKED, SOLAR_SYSTEM_DISTANCE):
            self.source.Move(HULL, 1, unusable, BOX_SIZE)
            self.Update()
            self.AssertInWwise(sets=1, instances=1)

            self.source.Move(HULL, 1, ORIGIN, BOX_SIZE)
            self.Update()
            self.AssertInWwise(sets=1, instances=2)

    def test_an_instance_registered_outside_the_world_is_not_sent(self):
        self.AddBox(HULL, 1, center=PARKED)
        self.AddBox(OTHER_HULL, 2, size=FLAT_SIZE)
        self.Update()

        self.AssertInWwise(sets=0, instances=0)

    def PlaceListenerAtOrigin(self):
        import audio2
        audio2.GetListener().SetPosition((0.0, 0.0, 1.0), (0.0, 1.0, 0.0), ORIGIN)

    def test_a_mesh_beyond_the_distance_limit_waits_outside_wwise(self):
        self.PlaceListenerAtOrigin()
        self.manager.geometryMaxDistance = 100.0
        self.AddBox(HULL, 1)
        self.AddBox(HULL, 2, center=ELSEWHERE)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryInstancesRegistered, 2)

        self.manager.geometryMaxDistance = 0.0
        self.Update()
        self.AssertInWwise(sets=1, instances=2)

        self.manager.geometryMaxDistance = 100.0
        self.Update()
        self.AssertInWwise(sets=1, instances=1)

    def test_the_triangle_budget_keeps_the_nearest_meshes(self):
        self.PlaceListenerAtOrigin()
        self.manager.geometryMaxTriangles = BOX_TRIANGLES
        self.AddTriangle(HULL, 1, center=ELSEWHERE)
        self.AddBox(OTHER_HULL, 2)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, BOX_TRIANGLES)

        self.Remove(OTHER_HULL, 2)
        self.Update()
        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, 1)

    def test_a_mesh_larger_than_the_budget_stays_out_of_wwise(self):
        self.manager.geometryMaxTriangles = BOX_TRIANGLES - 1
        self.AddBox(HULL, 1)
        self.AddTriangle(OTHER_HULL, 2)
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, 1)
        self.assertEqual(self.manager.geometryInstancesRegistered, 2)

    def test_a_set_leaves_wwise_when_the_budget_drops_its_instances(self):
        self.PlaceListenerAtOrigin()
        self.AddBox(HULL, 1)
        self.AddTriangle(OTHER_HULL, 2, center=ELSEWHERE)
        self.Update()
        self.AssertInWwise(sets=2, instances=2)

        self.manager.geometryMaxTriangles = BOX_TRIANGLES
        self.Update()
        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, BOX_TRIANGLES)

        self.manager.geometryMaxTriangles = 0
        self.Update()
        self.AssertInWwise(sets=2, instances=2)

    def test_moving_an_unknown_instance_does_nothing(self):
        self.source.Move(HULL, 99, ELSEWHERE, BOX_SIZE)
        self.source.Remove(HULL, 99)
        self.Update()

        self.AssertInWwise(sets=0, instances=0)

    def test_geometry_registered_while_the_switch_is_off_is_sent_when_it_turns_on(self):
        self.manager.spatialAudioGeometryEnabled = False
        self.AddBox(HULL, 1)
        self.Update()
        self.AssertInWwise(sets=0, instances=0)

        self.manager.spatialAudioGeometryEnabled = True
        self.Update()

        self.AssertInWwise(sets=1, instances=1)

    def test_turning_the_switch_off_removes_everything_from_wwise(self):
        self.AddBox(HULL, 1)
        self.Update()

        self.manager.spatialAudioGeometryEnabled = False

        self.AssertInWwise(sets=0, instances=0)

    def test_geometry_survives_audio_being_disabled_and_enabled(self):
        self.AddBox(HULL, 1)
        self.Update()

        self.audioManager.Disable()
        self.AssertInWwise(sets=0, instances=0)

        self.audioManager.Enable()
        self.Update()

        self.AssertInWwise(sets=1, instances=1)

    def test_split_vertices_are_welded(self):
        """Two corners of the triangle sit on the same position, so welding leaves nothing to send."""
        self.source.AddCollapsedTriangle(HULL, 1)
        self.registered.append((HULL, 1))
        self.Update()

        self.AssertInWwise(sets=0, instances=0)

    def test_a_triangle_with_a_bad_index_is_dropped(self):
        self.source.AddTriangleWithBadIndex(HULL, 1)
        self.registered.append((HULL, 1))
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
        self.assertEqual(self.manager.geometryTrianglesInWwise, 1)

    def test_mesh_settings_can_change_while_geometry_is_in_wwise(self):
        self.AddBox(HULL, 1)
        self.Update()

        self.manager.transmissionLoss = 0.25
        self.manager.enableDiffractionOnBoundaryEdges = False
        self.manager.geometrySolid = False
        self.Update()

        self.AssertInWwise(sets=1, instances=1)
