import os
import logging
import unittest

import blue

from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK
from audiotests.base_test_class import GetAudioMetadataFromFile
from audiotests.utils import PumpOSWithTimeout

class TestAudManagerExposure(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import audio2
        super().setUpClass()

        blue.paths.SetSearchPath("soundbanks", os.path.abspath(os.path.join(os.path.dirname(__file__), "soundbanks")))
        settings = audio2.AudSettings()
        settings.applicationName = "Audio2 Testing"
        settings.baseSoundbankPath = "soundbanks:/"
        settings.soundbankLanguage = "English(US)" 
        cls.audioManager = audio2.GetOrCreateManager()
        cls.staticDataRepository = audio2.GetStaticDataRepository()
        cls.audioManager.UpdateSettings(settings)
        cls.staticDataRepository.Initialize(GetAudioMetadataFromFile())
        cls.audioManager.DisableAudioCulling()

    def loadedSoundBanksAreEmpty(self):
        return self.audioManager.GetLoadedSoundBanks() == []

    def alwaysTrueBoolean(self):
        '''This can be used with the PumpOsWithTimeout function to simply have it pump a few times.'''
        return True

    def setUp(self):
        self.audioManager.Enable([])
        PumpOSWithTimeout(self.loadedSoundBanksAreEmpty)

    def tearDown(self):
        self.audioManager.Disable()

    def test_audmanager_load_and_unload_bank(self):
        """Test all methods relating to loading and unloading soundbanks in AudManager."""
        from audio2.audiomanager import INIT_BANK
        # Test while audmanager is enabled

        # Test loading legit soundbank
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, ONE_SHOT_BNK])) # Init.bnk always gets loaded when you Enable Carbon Audio

        # Test loading an already loaded soundbank does nothing
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, ONE_SHOT_BNK]))

        # Test unloading legit soundbank
        self.audioManager.UnloadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])

        # Test clearing soundbanks
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        self.audioManager.LoadBank(LOOP_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        banksBeforeClear = self.audioManager.GetLoadedSoundBanks()
        self.assertEqual(len(banksBeforeClear), 3)
        self.audioManager.ClearBanks()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        banksAfterClear = self.audioManager.GetLoadedSoundBanks()
        self.assertEqual(banksAfterClear, [])

        # Test trying to load a soundbank that doesn't exist
        self.audioManager.LoadBank("FAKE.bnk")
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [])

        # Test with audio disabled
        self.audioManager.Disable()

        self.audioManager.LoadBank(ONE_SHOT_BNK)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [])

    def test_audmanager_sends_events_sent_while_loading_after_complete(self):
        """Test that events sent while it's soundbanks are currently loading are resent once the soundbank it depends on is finished loading."""
        import audio2

        emitter1 = audio2.AudEmitter("emitter1")
        self.audioManager.LoadBank(COMMON_BNK)
        self.audioManager.LoadBank(LOOP_BNK)
        self.assertEqual(emitter1.SendEvent(LOOP_EVENT), 0) 
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(list(emitter1.GetPlayingEvents().values())[0], LOOP_EVENT)

    def test_audmanager_sends_events_sent_while_loading_after_complete_with_prefix(self):
        """Test that events sent to an emitter with an event prefix while it's soundbanks are currently loading are correctly resent once the soundbank it depends on is finished loading."""
        import audio2

        eventPrefix = "Play_"
        prefixEmitter = audio2.AudEmitter("prefixEmitter")
        prefixEmitter.eventPrefix = eventPrefix
        self.audioManager.LoadBank(COMMON_BNK)
        self.audioManager.LoadBank(LOOP_BNK)
        # Send event without Play_ at the beginning, as the audio emitter will prepend it for us.
        self.assertEqual(prefixEmitter.SendEvent(LOOP_EVENT.replace(eventPrefix, "")), 0) 
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        # The event, at the send, should come out with the full prefix (e.g. Play_TestLoop)
        self.assertEqual(list(prefixEmitter.GetPlayingEvents().values())[0], LOOP_EVENT)

    def test_audmanager_soundbanks_race_condition(self):
        """Test all methods relating to loading and unloading soundbanks in AudManager."""
        from audio2.audiomanager import INIT_BANK
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=1)
        self.audioManager.UnloadBank(ONE_SHOT_BNK)
        self.audioManager.LoadBank(ONE_SHOT_BNK)

        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertCountEqual(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK, ONE_SHOT_BNK])

        self.audioManager.UnloadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=1)
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        self.audioManager.UnloadBank(ONE_SHOT_BNK)

        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])

    def test_audmanager_audio_emitters(self):
        """Test that the AudManager keeps track of all audio emitters."""
        import audio2
        emitter1 = audio2.AudEmitter("emitter1")
        emitter2 = audio2.AudEmitter("emitter2")
        retrievedEmitter1 = self.audioManager.GetAudioEmitter(emitter1.ID)
        retrievedEmitter2 = self.audioManager.GetAudioEmitter(emitter2.ID)
        self.assertEqual(emitter1, retrievedEmitter1)
        self.assertEqual(emitter2, retrievedEmitter2)

    def test_audmanager_stopall(self):
        """Test that AudManager::StopAll works as expected."""
        import audio2
        self.audioManager.LoadBank(COMMON_BNK)
        self.audioManager.LoadBank(LOOP_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        emitter1 = audio2.AudEmitter("emitter1")
        emitter2 = audio2.AudEmitter("emitter2")

        self.assertTrue(emitter1.SendEvent(LOOP_EVENT) > 0) 
        self.assertTrue(emitter2.SendEvent(LOOP_EVENT) > 0)

        self.audioManager.StopAll()

        
        blue.pyos.synchro.SleepWallclock(1200)# The default fade out duration when stoping sounds is 1 second, so give this a little more than that to stop.
        self.assertTrue(len(emitter1.GetPlayingEvents()) == 0)
        self.assertTrue(len(emitter2.GetPlayingEvents()) == 0)

        # Test with audio disabled.
        self.audioManager.Disable()
        self.audioManager.StopAll()

    def test_audmanager_setglobalrtpc(self):
        """Test that AudManager::SetGlobalRTPC works as expected."""
        self.assertTrue(self.audioManager.SetGlobalRTPC("test_rtpc", 1.0))
        self.assertFalse(self.audioManager.SetGlobalRTPC("test_rtpc", float("inf")))

    def test_audmanager_setstate(self):
        """Test that AudManager::SetState works as expected."""
        self.assertTrue(self.audioManager.SetState("test_state_group", "test_state"))

    def test_audmanager_debug_apis(self):
        """Test all debugging APIs exposed by AudManager."""
        self.assertFalse(self.audioManager.GetDebugDisplayAllEmitters())

        self.audioManager.EnableDebugDisplayAllEmitters()
        self.assertTrue(self.audioManager.GetDebugDisplayAllEmitters())

        self.audioManager.DisableDebugDisplayAllEmitters()
        self.assertFalse(self.audioManager.GetDebugDisplayAllEmitters())

        # Test with audio disabled.
        self.audioManager.Disable()

        self.audioManager.EnableDebugDisplayAllEmitters()
        self.assertTrue(self.audioManager.GetDebugDisplayAllEmitters())

