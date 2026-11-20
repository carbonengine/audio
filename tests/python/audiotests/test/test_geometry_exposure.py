# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import RestoreSettings, SaveSettings


BOX_TRIANGLES = 12
ORIGIN = (0.0, 0.0, 0.0)
ELSEWHERE = (500.0, 0.0, 0.0)
BOX_SIZE = (10.0, 20.0, 30.0)
MIRRORED_SIZE = (-10.0, 20.0, 30.0)
# Where trinity parks an object that left the scene.
PARKED = (0.0, -3.4028234663852886e38, 0.0)
SOLAR_SYSTEM_DISTANCE = (-9.4e11, 6.3e10, 8.6e11)
FLAT_SIZE = (10.0, 0.0, 30.0)

# A spot in a solar system, far enough out that positions there don't fit in a float.
SOLAR_SYSTEM_SPOT = (1.0e11, 2.0e9, -3.0e11)

HULL = 1
OTHER_HULL = 2

SETTINGS = (
    "transmissionLoss",
    "enableDiffractionOnBoundaryEdges",
    "geometrySolid",
    "geometryPortalSubtraction",
    "geometryMaxDistance",
    "geometryMaxTriangles",
    "worldRebaseDistance",
    "loadBalancingSpread",
    "numberOfPrimaryRays",
    "maxDiffractionOrder",
    "maxDiffractionPaths",
)


def Add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def Subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


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
        self.settings = SaveSettings(self.manager, SETTINGS)
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        self.source = blue.LoadExtension("_audiotests").FakeGeometrySource()
        self.assertTrue(self.source.IsConnected(), "AudGeometry or AudWorld could not be created the way trinity creates them.")
        self.registered = []
        self.ResetWorldFrame()

    def tearDown(self):
        for geometrySetId, instanceId in self.registered:
            self.source.Remove(geometrySetId, instanceId)
        self.Update()
        self.ResetWorldFrame()
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

    def ResetWorldFrame(self):
        """Puts Wwise's origin back on a scene origin at zero."""
        rebaseDistance = self.manager.worldRebaseDistance
        self.manager.worldRebaseDistance = 0.0
        self.ArriveAt(ORIGIN)
        self.manager.worldRebaseDistance = rebaseDistance

    def ArriveAt(self, origin):
        """Jumps the scene origin to origin and holds it there for a frame, the way trinity does after a warp."""
        self.source.SetSceneOrigin(*origin)
        self.source.SetSceneOrigin(*origin)

    def PlaceListener(self, position):
        import audio2
        audio2.GetListener().SetPosition((0.0, 0.0, 1.0), (0.0, 1.0, 0.0), position)

    def MovePlayer(self, origin, meshes):
        """Moves the scene origin, then hands every mesh over relative to it, the way trinity does each frame.

        :param meshes: Where each (geometrySetId, instanceId) is in the world.
        """
        self.source.SetSceneOrigin(*origin)
        for (geometrySetId, instanceId), position in meshes.items():
            self.source.Move(geometrySetId, instanceId, Subtract(position, origin), BOX_SIZE)

    def AddBoxInWorld(self, geometrySetId, instanceId, origin, position):
        """Registers a box at a world position while the scene origin is at origin."""
        self.ArriveAt(origin)
        self.AddBox(geometrySetId, instanceId, Subtract(position, origin))

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
        self.manager.enableDiffractionOnBoundaryEdges = not self.manager.enableDiffractionOnBoundaryEdges
        self.manager.geometrySolid = False
        self.manager.geometryPortalSubtraction = not self.manager.geometryPortalSubtraction
        self.Update()

        self.AssertInWwise(sets=1, instances=1)

    def test_boundary_edges_do_not_diffract_by_default(self):
        """They add many edges for Wwise to search, and closed hulls don't need them."""
        self.assertFalse(self.manager.enableDiffractionOnBoundaryEdges)

    def test_portals_do_not_cut_meshes_by_default(self):
        """Wwise re-cuts every mesh a portal overlaps on each portal visibility rebuild, which stalls the audio thread with big meshes."""
        self.assertFalse(self.manager.geometryPortalSubtraction)

    def test_a_still_mesh_is_not_sent_again_while_the_player_flies(self):
        """The scene origin follows the player, so every mesh's transform changes when the player moves."""
        mesh = Add(SOLAR_SYSTEM_SPOT, (1000.0, 0.0, 0.0))
        self.AddBoxInWorld(HULL, 1, SOLAR_SYSTEM_SPOT, mesh)
        self.Update()
        sends = self.manager.geometryInstanceSends
        rebases = self.manager.worldRebases

        for step in range(1, 11):
            self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (15.0 * step, 2.0 * step, 0.0)), {(HULL, 1): mesh})
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends)
        self.assertEqual(self.manager.worldRebases, rebases)
        self.AssertInWwise(sets=1, instances=1)

    def test_a_mesh_that_moves_in_the_world_is_sent_again(self):
        mesh = Add(SOLAR_SYSTEM_SPOT, (1000.0, 0.0, 0.0))
        self.AddBoxInWorld(HULL, 1, SOLAR_SYSTEM_SPOT, mesh)
        self.Update()
        sends = self.manager.geometryInstanceSends

        for step in range(1, 4):
            origin = Add(SOLAR_SYSTEM_SPOT, (15.0 * step, 0.0, 0.0))
            self.MovePlayer(origin, {(HULL, 1): Add(mesh, (0.0, 10.0 * step, 0.0))})
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends + 3)

    def test_float_jitter_of_a_far_mesh_is_not_a_move(self):
        """200 km out a float steps by 1.6 cm, so the mesh's scene position wobbles as the player creeps along."""
        mesh = Add(SOLAR_SYSTEM_SPOT, (200000.0, 0.0, 0.0))
        self.AddBoxInWorld(HULL, 1, SOLAR_SYSTEM_SPOT, mesh)
        self.Update()
        sends = self.manager.geometryInstanceSends

        for step in range(1, 11):
            self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (0.003 * step, 0.0, 0.0)), {(HULL, 1): mesh})
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends)

    def test_still_meshes_are_sent_once_when_the_player_gets_far_from_wwise_origin(self):
        self.manager.worldRebaseDistance = 500.0
        meshes = {
            (HULL, 1): Add(SOLAR_SYSTEM_SPOT, (100.0, 0.0, 0.0)),
            (HULL, 2): Add(SOLAR_SYSTEM_SPOT, (-100.0, 0.0, 0.0)),
        }
        for (geometrySetId, instanceId), position in meshes.items():
            self.AddBoxInWorld(geometrySetId, instanceId, SOLAR_SYSTEM_SPOT, position)
        self.Update()
        sends = self.manager.geometryInstanceSends
        rebases = self.manager.worldRebases

        self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (400.0, 0.0, 0.0)), meshes)
        self.Update()
        self.assertEqual(self.manager.geometryInstanceSends, sends)

        self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (600.0, 0.0, 0.0)), meshes)
        self.Update()
        self.assertEqual(self.manager.worldRebases, rebases + 1)
        self.assertEqual(self.manager.geometryInstanceSends, sends + 2)

        self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (700.0, 0.0, 0.0)), meshes)
        self.Update()
        self.assertEqual(self.manager.geometryInstanceSends, sends + 2)
        self.AssertInWwise(sets=1, instances=2)

    def test_the_wwise_origin_waits_for_the_player_to_arrive_after_a_warp(self):
        """Following a warp would send every still mesh again on every frame of it, slowing down included."""
        self.ArriveAt(SOLAR_SYSTEM_SPOT)
        rebases = self.manager.worldRebases
        destination = Add(SOLAR_SYSTEM_SPOT, (5.0e9, 0.0, 0.0))

        for step in range(1, 5):
            self.source.SetSceneOrigin(*Add(SOLAR_SYSTEM_SPOT, (1.0e9 * step, 0.0, 0.0)))
            self.Update()
        for remaining in (1.0e6, 1.0e5, 3.0e4, 1.0e4, 3.0e3):
            self.source.SetSceneOrigin(*Subtract(destination, (remaining, 0.0, 0.0)))
            self.Update()
        self.source.SetSceneOrigin(*destination)
        self.Update()
        self.assertEqual(self.manager.worldRebases, rebases)

        self.source.SetSceneOrigin(*destination)
        self.Update()
        self.assertEqual(self.manager.worldRebases, rebases + 1)

    def test_a_mesh_moving_along_with_the_player_moves_in_wwise(self):
        """Trinity only hands over a transform that changed, and one moving along with the player doesn't."""
        self.AddBoxInWorld(HULL, 1, SOLAR_SYSTEM_SPOT, Add(SOLAR_SYSTEM_SPOT, (100.0, 0.0, 0.0)))
        self.Update()
        sends = self.manager.geometryInstanceSends

        for step in range(1, 4):
            self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (15.0 * step, 0.0, 0.0)), {})
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends + 3)

    def test_a_mesh_registered_while_the_player_moves_is_not_sent_twice(self):
        self.ArriveAt(SOLAR_SYSTEM_SPOT)
        mesh = Add(SOLAR_SYSTEM_SPOT, (1000.0, 0.0, 0.0))
        origin = Add(SOLAR_SYSTEM_SPOT, (15.0, 0.0, 0.0))
        self.source.SetSceneOrigin(*origin)
        self.AddBox(HULL, 1, Subtract(mesh, origin))
        self.Update()
        sends = self.manager.geometryInstanceSends

        for step in range(2, 5):
            self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (15.0 * step, 0.0, 0.0)), {(HULL, 1): mesh})
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends)
        self.AssertInWwise(sets=1, instances=1)

    def test_a_far_mesh_is_sent_every_time_it_moves(self):
        """Sounds sit on the meshes, so even a small move of a far mesh can put one inside it."""
        self.PlaceListener(ORIGIN)
        self.AddBox(HULL, 1, center=(1000.0, 0.0, 0.0))
        self.Update()
        sends = self.manager.geometryInstanceSends

        for step in range(1, 4):
            self.source.Move(HULL, 1, (1000.0, 0.5 * step, 0.0), BOX_SIZE)
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends + 3)

    def test_spatial_audio_settings_change_while_spatial_audio_runs(self):
        self.AddBox(HULL, 1)
        self.Update()

        self.manager.loadBalancingSpread = 4
        self.manager.numberOfPrimaryRays = 20
        self.manager.maxDiffractionOrder = 2
        self.manager.maxDiffractionPaths = 4
        self.Update()

        self.assertEqual(self.manager.loadBalancingSpread, 4)
        self.assertEqual(self.manager.numberOfPrimaryRays, 20)
        self.assertEqual(self.manager.maxDiffractionOrder, 2)
        self.assertEqual(self.manager.maxDiffractionPaths, 4)
        self.AssertInWwise(sets=1, instances=1)

    def test_a_rebase_distance_of_zero_sends_positions_relative_to_the_player(self):
        """How Wwise got positions before the world frame, every still mesh goes again whenever the player moves."""
        self.manager.worldRebaseDistance = 0.0
        mesh = Add(SOLAR_SYSTEM_SPOT, (1000.0, 0.0, 0.0))
        self.AddBoxInWorld(HULL, 1, SOLAR_SYSTEM_SPOT, mesh)
        self.Update()
        sends = self.manager.geometryInstanceSends

        for step in range(1, 4):
            self.MovePlayer(Add(SOLAR_SYSTEM_SPOT, (15.0 * step, 0.0, 0.0)), {(HULL, 1): mesh})
            self.Update()

        self.assertEqual(self.manager.geometryInstanceSends, sends + 3)
