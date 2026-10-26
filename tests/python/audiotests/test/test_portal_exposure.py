# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import BaseAudio2TestClass


OUTDOORS_ROOM_ID = 0xFFFFFFFFFFFFFFFF
ROOM_SIZE = (20.0, 20.0, 20.0)
PORTAL_SIZE = (4.0, 4.0, 2.0)
ROOM_CHANGE_UPDATES = 3

# Two rooms that share the wall at z = 0, with the front room on the -z side.
FRONT_ROOM_CENTER = (0.0, 0.0, -10.0)
BACK_ROOM_CENTER = (0.0, 0.0, 10.0)
FAR_AWAY = (0.0, 0.0, 1000.0)


class TestPortalExposure(BaseAudio2TestClass):
    """Tests which rooms a portal joins and when it is sent to Wwise."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        self.audioManager.Enable()
        self.manager = self.audioManager.manager
        self.manager.spatialAudioRoomsEnabled = True
        if not self.manager.spatialAudioRoomsEnabled:
            self.skipTest("Spatial audio rooms are not available on this platform.")
        self.volumes = []

    def tearDown(self):
        self.manager.spatialAudioRoomsEnabled = False
        self.volumes = []
        self.audioManager.Disable()

    def Update(self, times=1):
        """Let the engine run one frame per time, which runs AudManager::Process() once."""
        import blue
        for _ in range(times):
            blue.pyos.synchro.Yield()

    def Place(self, consumer, center, size):
        import blue
        volume = blue.LoadExtension("_audiotests").FakeVolumeObject()
        volume.consumer = consumer
        volume.Place(center, size)
        self.volumes.append(volume)
        return volume

    def AddRoom(self, name, center):
        import audio2
        room = audio2.AudRoom()
        room.name = name
        volume = self.Place(room, center, ROOM_SIZE)
        self.assertTrue(room.isRegistered, "Room '%s' was not sent to Wwise." % name)
        return room, volume

    def AddPortal(self, center, size=PORTAL_SIZE):
        import audio2
        portal = audio2.AudPortal()
        portal.name = "testPortal"
        volume = self.Place(portal, center, size)
        return portal, volume

    def AssertRooms(self, portal, frontRoomID, backRoomID, message):
        self.assertTrue(portal.isRegistered, message)
        self.assertEqual((portal.frontRoomID, portal.backRoomID), (frontRoomID, backRoomID), message)

    def test_a_portal_across_a_room_wall_joins_the_rooms_on_its_sides(self):
        frontRoom, _ = self.AddRoom("frontRoom", FRONT_ROOM_CENTER)
        backRoom, _ = self.AddRoom("backRoom", BACK_ROOM_CENTER)
        portal, _ = self.AddPortal((0.0, 0.0, 0.0))
        self.Update()
        self.AssertRooms(portal, frontRoom.roomID, backRoom.roomID, "The portal did not join the rooms in front of and behind it.")

        turnedPortal, _ = self.AddPortal((0.0, 0.0, 0.0), (-PORTAL_SIZE[0], PORTAL_SIZE[1], -PORTAL_SIZE[2]))
        self.Update()
        self.AssertRooms(turnedPortal, backRoom.roomID, frontRoom.roomID, "Turning the portal round did not swap its rooms.")

    def test_a_portal_on_an_outer_wall_leads_outdoors(self):
        backRoom, _ = self.AddRoom("backRoom", BACK_ROOM_CENTER)
        portal, _ = self.AddPortal((0.0, 0.0, 0.0))
        self.Update()
        self.AssertRooms(portal, OUTDOORS_ROOM_ID, backRoom.roomID, "The portal did not lead from the room to outdoors.")

    def test_a_portal_with_the_same_room_on_both_sides_is_not_sent(self):
        self.AddRoom("frontRoom", FRONT_ROOM_CENTER)
        self.AddRoom("backRoom", BACK_ROOM_CENTER)
        portal, portalVolume = self.AddPortal(FRONT_ROOM_CENTER)
        self.Update()
        self.assertFalse(portal.isRegistered, "A portal inside one room was sent to Wwise.")

        portalVolume.Place((0.0, 0.0, 0.0), PORTAL_SIZE)
        self.Update()
        self.assertTrue(portal.isRegistered, "The portal was not sent once it crossed the room wall.")

        portalVolume.Place(FRONT_ROOM_CENTER, PORTAL_SIZE)
        self.Update(ROOM_CHANGE_UPDATES)
        self.assertFalse(portal.isRegistered, "The portal stayed in Wwise after moving inside one room.")
        self.assertEqual((portal.frontRoomID, portal.backRoomID), (OUTDOORS_ROOM_ID, OUTDOORS_ROOM_ID), "The removed portal kept its rooms.")

    def test_new_rooms_replace_the_sent_rooms_only_after_three_updates_in_a_row(self):
        frontRoom, _ = self.AddRoom("frontRoom", FRONT_ROOM_CENTER)
        backRoom, backRoomVolume = self.AddRoom("backRoom", BACK_ROOM_CENTER)
        portal, _ = self.AddPortal((0.0, 0.0, 0.0))
        self.Update()
        self.AssertRooms(portal, frontRoom.roomID, backRoom.roomID, "The portal did not join the two rooms.")

        for _ in range(ROOM_CHANGE_UPDATES):
            backRoomVolume.Place(FAR_AWAY, ROOM_SIZE)
            self.Update()
            self.AssertRooms(portal, frontRoom.roomID, backRoom.roomID, "Rooms found for one update replaced the sent ones.")
            backRoomVolume.Place(BACK_ROOM_CENTER, ROOM_SIZE)
            self.Update()
            self.AssertRooms(portal, frontRoom.roomID, backRoom.roomID, "The rooms changed when the room came back.")

        backRoomVolume.Place(FAR_AWAY, ROOM_SIZE)
        for update in range(1, ROOM_CHANGE_UPDATES):
            self.Update()
            self.AssertRooms(portal, frontRoom.roomID, backRoom.roomID, "The rooms changed after %d update(s)." % update)
        self.Update()
        self.AssertRooms(portal, frontRoom.roomID, OUTDOORS_ROOM_ID, "The rooms did not change after %d updates in a row." % ROOM_CHANGE_UPDATES)
