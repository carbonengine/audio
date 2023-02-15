import unittest
import uthread2

import audio2
from base_test_class import BaseAudio2TestClass


class TestEnabledAudGameObjExposure(BaseAudio2TestClass):
    def tearDown(self):
        self.emitter.eventPrefix = ""

    @classmethod
    def setUpClass(cls):
        super(TestEnabledAudGameObjExposure, cls).setUpClass()
        cls.audioManager.SetEnabled(True)

    def setUp(self):
        # Even though this is a test for AudGameObjResource exposure, it is
        # not directly exposed through blue, but only exposed as a parent class
        # through other audio emitter classes. That is why these tests use AudEmitter.
        self.emitter = audio2.AudEmitter("emitter1")

    def test_enabled_audgameobjresource_sendevent(self):
        """Test that AudGameObjResource::SendEvent works while audio is enabled."""
        # test existing event
        playingID = self.emitter.SendEvent("triglavian_atmo1_play")
        self.assertTrue(playingID > 0)

        # test non-existing event
        playingID = self.emitter.SendEvent("fake_event")
        self.assertTrue(playingID == 0)

        # test having a event prefix on the emitter
        self.emitter.eventPrefix = "ship_engine_XXS_"
        playingID = self.emitter.SendEvent("booster_1st_on")
        self.assertTrue(playingID > 0)

    def test_enabled_audgameobjresource_stopevent(self):
        """test that AudGameObjResource::StopEvent works while audio is enabled."""
        # Test that stop event stops multiple instances of the same event
        playingID1 = self.emitter.SendEvent("triglavian_atmo1_play")
        playingID2 = self.emitter.SendEvent("triglavian_atmo1_play")
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)
        stopped = self.emitter.StopEvent("triglavian_atmo1_play", 10)
        self.assertTrue(stopped)

        # Wait for the event stopped callback to be hit and verify that 
        # that StopEvent returns false if the event is not playing.
        uthread2.Sleep(0.1)
        stopped = self.emitter.StopEvent("triglavian_atmo1_play")
        self.assertFalse(stopped)

    def test_enabled_audgameobjresource_stopsound(self):
        """Test that AudGameObjReseource::StopSound only stops one instance of the same event."""
        emitter = audio2.AudEmitter("stopsound_test")
        playingID1 = emitter.SendEvent("cloud_cover_hell_play")
        playingID2 = emitter.SendEvent("cloud_cover_hell_play")
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)
        emitter.StopSound(playingID1, 10)
        uthread2.Sleep(0.1) # give time for wwise callback for stopped sound
        self.assertEquals(len(emitter.GetPlayingEvents()), 1)
        emitter.StopSound(playingID2, 10)
        uthread2.Sleep(0.1) # give time for wwise callback for stopped sound
        self.assertEquals(len(emitter.GetPlayingEvents()), 0)

        self.assertFalse(emitter.StopSound(999999, 10))

    def test_enabled_audgameobjresource_stopall(self):
        """Test that AudGameObjResource::StopAll() stops all instances of playing events."""
        playingID1 = self.emitter.SendEvent("triglavian_atmo1_play")
        playingID2 = self.emitter.SendEvent("triglavian_atmo1_play")
        playingID3 = self.emitter.SendEvent("triglavian_atmo1_play")
        self.assertTrue(playingID1 > 0)
        self.assertTrue(playingID2 > 0)
        self.assertTrue(playingID3 > 0)
        self.emitter.StopAll()
        uthread2.Sleep(1.5) # give time for wwise callback for stopped sound
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
        playingID = self.emitter.SendEvent("triglavian_atmo1_play")
        self.assertTrue(self.emitter.SeekOnEventMs(playingID, 10))

    def test_enabled_audgameobjresource_seekoneventpercent(self):
        """Test that AudGameObjResource::SeekOnEventPercent works while audio is enabled."""
        # Seek on non existant playing ID
        self.assertFalse(self.emitter.SeekOnEventPercent(5000, 0.5))

        # Seek on existing playing ID
        playingID = self.emitter.SendEvent("triglavian_atmo1_play")
        self.assertTrue(self.emitter.SeekOnEventPercent(playingID, 0.5))
        self.assertTrue(self.emitter.SeekOnEventPercent(playingID, 5000))