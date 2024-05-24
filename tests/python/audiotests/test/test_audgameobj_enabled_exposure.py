import audio2
import blue

from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK, ONE_SHOT_EVENT
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout



class TestEnabledAudGameObjExposure(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super(TestEnabledAudGameObjExposure, cls).setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK])

    def setUp(self):
        # Even though this is a test for AudGameObjResource exposure, it is
        # not directly exposed through blue, but only exposed as a parent class
        # through other audio emitter classes. That is why these tests use AudEmitter.
        self.emitter = audio2.AudEmitter("emitter1")
        self.emitter.SetPosition((0,0,0), (0,0,0), (0,0,0))
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

    def tearDown(self):
        self.emitter.eventPrefix = ""
        self.emitter.StopAll()
        self.audioManager.Disable()

    def test_enabled_audgameobjresource_sendevent(self):
        """Test that AudGameObjResource::SendEvent works while audio is enabled."""
        # test existing event
        playingID = self.emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID > 0)

        # test non-existing event
        playingID = self.emitter.SendEvent("fake_event")
        self.assertTrue(playingID == 0)

        # test having a event prefix on the emitter
        self.emitter.eventPrefix = "Play_"
        playingID = self.emitter.SendEvent(LOOP_EVENT.replace("Play_", ""))
        self.assertTrue(playingID > 0)

    def test_enabled_audgameobjresource_stopevent(self):
        """test that AudGameObjResource::StopEvent works while audio is enabled."""
        # Test that stop event stops multiple instances of the same event
        playingID1 = self.emitter.SendEvent(LOOP_EVENT)
        playingID2 = self.emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)
        stopped = self.emitter.StopEvent(LOOP_EVENT, 10)
        self.assertTrue(stopped)

        # Wait for the event stopped callback to be hit and verify that 
        # that StopEvent returns false if the event is not playing.
        blue.pyos.synchro.SleepWallclock(150)
        stopped = self.emitter.StopEvent(LOOP_EVENT)
        self.assertFalse(stopped)

    def test_enabled_audgameobjresource_stopsound(self):
        """Test that AudGameObjReseource::StopSound only stops one instance of the same event."""
        playingID1 = self.emitter.SendEvent(LOOP_EVENT)
        playingID2 = self.emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)

        self.emitter.StopSound(playingID1, 10)
        blue.pyos.synchro.SleepWallclock(200) # give time for wwise callback for stopped sound
        self.assertEquals(len(self.emitter.GetPlayingEvents()), 1)

        self.emitter.StopSound(playingID2, 10)
        blue.pyos.synchro.SleepWallclock(200)
        self.assertEquals(len(self.emitter.GetPlayingEvents()), 0)

        # Test a fake playingID returns False
        self.assertFalse(self.emitter.StopSound(999999, 10))

    def test_enabled_audgameobjresource_stopall(self):
        """Test that AudGameObjResource::StopAll() stops all instances of playing events."""
        playingID1 = self.emitter.SendEvent(LOOP_EVENT)
        playingID2 = self.emitter.SendEvent(LOOP_EVENT)
        playingID3 = self.emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)
        self.assertTrue(playingID3 > 0)
        self.emitter.StopAll()
        blue.pyos.synchro.SleepWallclock(1500) # give time for wwise callback for stopped sound
        self.assertEquals(len(self.emitter.GetPlayingEvents()), 0)

    def test_enabled_audgameobjresource_setattenuationscalingfactor(self):
        """Test that AudGameObjResource::SetAttenuationScalingFactor works."""
        self.assertTrue(self.emitter.SetAttenuationScalingFactor(0.5))
        self.assertEquals(self.emitter.scalingFactor, 0.5)
        self.assertFalse(self.emitter.SetAttenuationScalingFactor(0))

    def test_enabled_audgameobjresource_setswitch(self):
        """Test that AudGameObjResource::SetSwitch works while audio is enabled."""
        self.assertTrue(self.emitter.SetSwitch("wormhole_mass", "large"))
        self.assertTrue(self.emitter.SetSwitch("fake", "switch"))

    def test_enabled_audgameobjresource_setrtpc(self):
        """Test that AudGameObjResource::SetRTPC works while audio is enabled."""
        self.assertTrue(self.emitter.SetRTPC("anything_works", 10))
        self.assertFalse(self.emitter.SetRTPC("anything_works", float("inf")))

    def test_enabled_audgameobjresource_seekoneventms(self):
        """Test that AudGameObjResource::SeekOnEventMs works while audio is enabled."""
        # Seek on non existant playing ID
        self.assertFalse(self.emitter.SeekOnEventMs(5000, 10))

        # Seek on existing playing ID
        playingID = self.emitter.SendEvent(LOOP_EVENT)
        blue.pyos.synchro.SleepWallclock(500)
        self.assertTrue(self.emitter.SeekOnEventMs(playingID, 10))

    def test_enabled_audgameobjresource_seekoneventpercent(self):
        """Test that AudGameObjResource::SeekOnEventPercent works while audio is enabled."""
        # Seek on non existant playing ID
        self.assertFalse(self.emitter.SeekOnEventPercent(5000, 0.5))

        # Seek on existing playing ID
        playingID = self.emitter.SendEvent(LOOP_EVENT)
        blue.pyos.synchro.SleepWallclock(10)
        self.assertTrue(self.emitter.SeekOnEventPercent(playingID, 0.5))
        self.assertTrue(self.emitter.SeekOnEventPercent(playingID, 5000))

    def test_audgameobjresource_plays_loop_on_wake(self):
        """Test that if a loop is sent to an instance AudGameObjResource that is culled then that loop will play when it is woken up."""
        self.emitter.ForceCullingStateChange() # This has to be used instead of Cull() or else this will wake up automatically on the next tick.
        self.emitter.SendEvent(LOOP_EVENT)
        self.emitter.ForceCullingStateChange()
        blue.pyos.synchro.SleepWallclock(15)
        self.assertTrue(len(self.emitter.GetPlayingEvents()) == 1)
        self.assertTrue(self.emitter.GetPlayingEvents().values()[0] == LOOP_EVENT)

    def test_audgameobjresource_plays_one_shot_if_woken_quickly(self):
        """Test that if a one shot is sent to an instance of AudGameObjResource while it is culled it will be actually be played if woken up in the defined one shot window.""" 
        oneShotWindow = float(audio2.GetOrCreateManager().oneShotWindow) # This is in milliseconds
        self.emitter.ForceCullingStateChange() # This has to be used instead of Cull() or else this will wake up automatically on the next tick.
        self.emitter.SendEvent(ONE_SHOT_EVENT)
        blue.pyos.synchro.SleepWallclock(oneShotWindow) 
        self.emitter.ForceCullingStateChange()
        self.assertTrue(len(self.emitter.GetPlayingEvents()) == 1)
        self.assertTrue(self.emitter.GetPlayingEvents().values()[0] == ONE_SHOT_EVENT)

    def test_audgameobjresource_fails_to_play_one_shot_if_not_woken_quickly(self):
        """Test that if a one shot is sent to an instance of AudGameObjResource while it is culled and it is woken up after the one shot window it will not play."""
        oneShotWindow = float(audio2.GetOrCreateManager().oneShotWindow) # This is in milliseconds
        self.emitter.ForceCullingStateChange() # This has to be used instead of Cull() or else this will wake up automatically on the next tick.
        self.emitter.SendEvent(ONE_SHOT_EVENT)
        blue.pyos.synchro.SleepWallclock(oneShotWindow + 200.0) # 200.0 has to be used because there is about a 10 ms drift when doing this.
        self.emitter.ForceCullingStateChange()
        blue.pyos.synchro.SleepWallclock(oneShotWindow + 200.0) # 200.0 has to be used because there is about a 10 ms drift when doing this.
        self.assertTrue(len(self.emitter.GetPlayingEvents()) == 0)
