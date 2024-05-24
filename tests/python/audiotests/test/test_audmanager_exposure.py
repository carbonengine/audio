import os
import unittest

import audio2
import blue

from audio2.audiomanager import INIT_BANK
from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK
from audiotests.base_test_class import GetEventMetadataFromFile
from audiotests.utils import PumpOSWithTimeout

class TestAudManagerExposure(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super(TestAudManagerExposure, cls).setUpClass()

        blue.paths.SetSearchPath("soundbanks", os.path.abspath(os.path.join(os.path.dirname(__file__), "soundbanks")))
        settings = audio2.AudSettings()
        settings.applicationName = "Audio2 Testing"
        settings.baseSoundbankPath = "soundbanks:/"
        settings.soundbankLanguage = "English(US)" 
        cls.audioManager = audio2.GetOrCreateManager()
        cls.staticDataRepository = audio2.GetStaticDataRepository()
        cls.audioManager.UpdateSettings(settings)
        cls.staticDataRepository.Initialize(GetEventMetadataFromFile())
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
        # Test while audmanager is enabled

        # Test loading legit soundbank
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, ONE_SHOT_BNK])) # Init.bnk always gets loaded when you Enable Carbon Audio

        # Test loading an already loaded soundbank does nothing 
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, ONE_SHOT_BNK]))

        # Test unloading legit soundbank
        self.audioManager.UnloadBank(ONE_SHOT_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])

        # Test clearing soundbanks
        self.audioManager.LoadBank(ONE_SHOT_BNK)
        self.audioManager.LoadBank(LOOP_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        banksBeforeClear = self.audioManager.GetLoadedSoundBanks() 
        self.assertEquals(len(banksBeforeClear), 3)
        self.audioManager.ClearBanks()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        banksAfterClear = self.audioManager.GetLoadedSoundBanks() 
        self.assertEquals(banksAfterClear, [])

        # Test trying to load a soundbank that doesn't exist
        self.audioManager.LoadBank("FAKE.bnk")
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [])

        # Test with audio disabled
        self.audioManager.Disable()

        self.audioManager.LoadBank(ONE_SHOT_BNK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [])

    def test_audmanager_audio_emitters(self):
        """Test that the AudManager keeps track of all audio emitters."""
        emitter1 = audio2.AudEmitter("emitter1")
        emitter2 = audio2.AudEmitter("emitter2")
        retrievedEmitter1 = self.audioManager.GetAudioEmitter(emitter1.ID)
        retrievedEmitter2 = self.audioManager.GetAudioEmitter(emitter2.ID)
        self.assertEquals(emitter1, retrievedEmitter1)
        self.assertEquals(emitter2, retrievedEmitter2)

    def test_audmanager_stopall(self):
        """Test that AudManager::StopAll works as expected."""
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

    def test_audmanager_disabling_clears_banks(self):
        """Test that disabling AudManager clears all soundbanks."""
        self.audioManager.LoadBank(INIT_BANK)
        self.audioManager.Disable()
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [])
