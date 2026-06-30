# Copyright © 2022 CCP ehf.

import blue
import time

from audiotests.base_test_class import (
    COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK, ONE_SHOT_EVENT, ESSENTIAL_EVENT, ESSENTIAL_BNK,
    NONESSENTIAL_BNK, NONESSENTIAL_BNK_EVENT, NONESSENTIAL_STREAM_BNK, NONESSENTIAL_STREAM_EVENT
)
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout



class TestEnabledAudGameObjExposure(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[
            COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK, ESSENTIAL_BNK, NONESSENTIAL_BNK, NONESSENTIAL_STREAM_BNK
        ])

    def setUp(self):
        # Even though this is a test for AudGameObjResource exposure, it is
        # not directly exposed through blue, but only exposed as a parent class
        # through other audio emitter classes. That is why these tests use AudEmitter.
        import audio2
        self.emitter = audio2.AudEmitter("emitter1")
        self.emitter.SetPosition((0,0,0), (0,0,0), (0,0,0))
        self.audioManager.Enable()

        self.listener = audio2.GetListener()
        self.listener.SetPosition((0,0,0), (0,0,0), (0,0,0))
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        
    def tearDown(self):
        self.emitter.eventPrefix = ""
        self.emitter.StopAll()
        self.audioManager.Disable()
        
        
    def wait_for_audio_condition(self, condition_func, *args, **kwargs):
        """
        Stricter timing windows:
        - expected_max_ms=100: Operation should complete within 100ms
        - timeout_ms=200: Absolute failure after 200ms
        - interval_ms=10: Check every 10ms
        """
        elapsed = 0
        expected_max_ms = kwargs.get("expected_max_ms", 100)
        timeout_ms = kwargs.get("timeout_ms", 200)
        interval_ms = kwargs.get("interval_ms", 10)
        start_time = time.time()
        
        while elapsed < timeout_ms:
            if condition_func(*args):
                elapsed_ms = (time.time() - start_time) * 1000
                if elapsed_ms > expected_max_ms:
                    raise AssertionError(
                        "Condition met but took {:.2f}ms (expected max {}ms)".format(elapsed_ms, expected_max_ms)
                    )
                return True
                
            blue.pyos.synchro.SleepWallclock(interval_ms)
            blue.os.Pump()
            elapsed += interval_ms
        
        return False

    def check_event_playing(self):
        return len(self.emitter.GetPlayingEvents()) == 1
        
    def get_events_values(self, events_dict):
        return list(events_dict.values())

    def test_enabled_audgameobjresource_sendevent_essential(self):
        # test event streamed from essential
        playingID = self.emitter.SendEvent(ESSENTIAL_EVENT)
        self.assertTrue(self.wait_for_audio_condition(self.check_event_playing))
        self.assertTrue(playingID > 0)

    def test_enabled_audgameobjresource_sendevent_nonessential_bnk(self):
        """Test that a SoundBank marked as non-essential gets loaded correctly by LowLevelIO"""
        playingID = self.emitter.SendEvent(NONESSENTIAL_BNK_EVENT)
        print(playingID)
        self.assertTrue(self.wait_for_audio_condition(self.check_event_playing))
        self.assertTrue(playingID > 0)
    
    def test_enabled_audgameobjresource_sendevent_nonessential_stream(self):
        """Test that a streamed event marked as non essential media gets loaded correctly by LowLevelIO"""
        playingID = self.emitter.SendEvent(NONESSENTIAL_STREAM_EVENT)
        self.assertTrue(self.wait_for_audio_condition(self.check_event_playing))
        self.assertTrue(playingID > 0)

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
        playingID1 = self.emitter.SendEvent(LOOP_EVENT)
        playingID2 = self.emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)

        self.emitter.StopSound(playingID1, 10)
        def check_one_event_playing():
            return len(self.emitter.GetPlayingEvents()) == 1
        self.assertTrue(self.wait_for_audio_condition(check_one_event_playing, 
                                                    expected_max_ms=100,
                                                    timeout_ms=200)) 

        self.emitter.StopSound(playingID2, 10)
        def check_no_events_playing():
            return len(self.emitter.GetPlayingEvents()) == 0
        self.assertTrue(self.wait_for_audio_condition(check_no_events_playing,
                                                    expected_max_ms=100,
                                                    timeout_ms=200))

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
        self.assertEqual(len(self.emitter.GetPlayingEvents()), 0)

    def test_enabled_audgameobjresource_setattenuationscalingfactor(self):
        """Test that AudGameObjResource::SetAttenuationScalingFactor works."""
        self.assertTrue(self.emitter.SetAttenuationScalingFactor(0.5))
        self.assertEqual(self.emitter.scalingFactor, 0.5)
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
        self.assertTrue(list(self.emitter.GetPlayingEvents().values())[0] == LOOP_EVENT)

    def test_audgameobjresource_plays_one_shot_if_woken_quickly(self):
        """Test that if a one shot is sent to an instance of AudGameObjResource while it is culled it will be actually be played if woken up in the defined one shot window."""
        import audio2
        oneShotWindow = float(audio2.GetOrCreateManager().oneShotWindow)
        self.emitter.ForceCullingStateChange()
        self.emitter.SendEvent(ONE_SHOT_EVENT)
        
        def check_one_shot_playing():
            events = self.emitter.GetPlayingEvents()
            values = self.get_events_values(events)
            return (len(events) == 1 and 
                   (values[0] if values else None) == ONE_SHOT_EVENT)
        
        # Give it time to pump after sending event. If one frame is not allowed to pass between sending a one shot and 
        # waking it up then sound prioritization will not be able to determine that the audio emitter is in range of the listener.
        blue.pyos.synchro.SleepWallclock(15)
        self.emitter.ForceCullingStateChange()
        self.assertTrue(self.wait_for_audio_condition(check_one_shot_playing, 
                                                    timeout_ms=int(oneShotWindow)))

    def test_audgameobjresource_fails_to_play_one_shot_if_not_woken_quickly(self):
        """Test that if a one shot is sent to an instance of AudGameObjResource while it is culled and it is woken up after the one shot window it will not play."""
        import audio2
        oneShotWindow = float(audio2.GetOrCreateManager().oneShotWindow) # This is in milliseconds
        self.emitter.ForceCullingStateChange() # This has to be used instead of Cull() or else this will wake up automatically on the next tick.
        self.emitter.SendEvent(ONE_SHOT_EVENT)
        blue.pyos.synchro.SleepWallclock(oneShotWindow + 200.0) # 200.0 has to be used because there is about a 10 ms drift when doing this.
        self.emitter.ForceCullingStateChange()
        blue.pyos.synchro.SleepWallclock(oneShotWindow + 200.0) # 200.0 has to be used because there is about a 10 ms drift when doing this.
        self.assertTrue(len(self.emitter.GetPlayingEvents()) == 0)

    def test_audgameobjresource_doesnt_wake_if_muted(self):
        """Test that if a one shot is sent to an instance of AudGameObjResource while it is culled and it is woken up after the one shot window it will not play."""
        self.emitter.ForceCullingStateChange() # This has to be used instead of Cull() or else this will wake up automatically on the next tick.
        self.emitter.Mute()
        self.emitter.SendEvent(ONE_SHOT_EVENT)
        self.emitter.ForceCullingStateChange() # This should not wake it up.
        blue.pyos.synchro.SleepWallclock(100) 
        self.assertTrue(self.emitter.IsMuted())
        self.assertTrue(self.emitter.IsCulled())

        # Test that you even if you force culling state change it will stay muted 
        self.emitter.ForceCullingStateChange()
        blue.pyos.synchro.SleepWallclock(100) 
        self.assertTrue(self.emitter.IsMuted())
        self.assertTrue(self.emitter.IsCulled())

        self.emitter.Unmute()
        blue.pyos.synchro.SleepWallclock(100) 
        self.assertFalse(self.emitter.IsMuted())
        self.assertFalse(self.emitter.IsCulled())


    def test_audgameobjresource_wakes_if_muted(self):
        """Test that if a one shot is sent to an instance of AudGameObjResource while it is culled and it is woken up after the one shot window it will not play."""
        self.emitter.ForceCullingStateChange() # This has to be used instead of Cull() or else this will wake up automatically on the next tick.
        self.emitter.Mute()
        self.emitter.SendEvent(ONE_SHOT_EVENT)
        self.emitter.ForceCullingStateChange() # This should not wake it up.
        blue.pyos.synchro.SleepWallclock(100) 
        self.assertTrue(len(self.emitter.GetPlayingEvents()) == 0)


    def test_audgameobjresource_sanitizes_events(self):
        def check_playing_id(playingID):
            self.assertTrue(playingID > 0)

        playingID = self.emitter.SendEvent(" {}".format(ONE_SHOT_EVENT))
        self.wait_for_audio_condition(check_playing_id, playingID)

        playingID = self.emitter.SendEvent("{} ".format(ONE_SHOT_EVENT))
        self.wait_for_audio_condition(check_playing_id, playingID)

        playingID = self.emitter.SendEvent(" {} ".format(ONE_SHOT_EVENT))
        self.wait_for_audio_condition(check_playing_id, playingID)

        playingID = self.emitter.SendEvent(" {}\n  \t  \r".format(ONE_SHOT_EVENT))
        self.wait_for_audio_condition(check_playing_id, playingID)

        playingID = self.emitter.SendEvent("\n\n\n\n\n\n{}\n\n\n\n\n".format(ONE_SHOT_EVENT))
        self.wait_for_audio_condition(check_playing_id, playingID)

        # Test with event prefixes
        prefix = ONE_SHOT_EVENT[:5]
        self.emitter.eventPrefix = prefix 
        playingID = self.emitter.SendEvent(" {}\n".format(ONE_SHOT_EVENT[5:]))
        self.wait_for_audio_condition(check_playing_id, playingID)