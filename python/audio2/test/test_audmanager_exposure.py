import unittest

import audio2

class TestAudManagerExposure(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super(TestAudManagerExposure, cls).setUpClass()

        settings = audio2.AudSettings()
        settings.applicationName = "Audio2 Testing"
        settings.baseSoundbankPath = "res:/Audio/"
        settings.soundbankLanguage = "English(US)"
        cls.audioManager = audio2.GetOrCreateManager()
        cls.audioManager.UpdateSettings(settings)

    def setUp(self):
        self.audioManager.SetEnabled(True)

    def test_audmanager_load_and_unload_bank(self):
        """Test all methods relating to loading and unloading soundbanks in AudManager."""
        # Test while audmanager is enabled

        # Test loading legit soundbank
        legitimateBank = self.audioManager.LoadBank("Init.bnk")
        self.assertTrue(legitimateBank)

        # Test loading an already loaded soundbank should return True
        self.assertTrue(self.audioManager.LoadBank("Init.bnk"))
        self.assertEquals(len(self.audioManager.GetLoadedSoundBanks()), 1)

        # Test unloading legit soundbank
        self.audioManager.UnloadBank("Init.bnk")
        loadedBanks = self.audioManager.GetLoadedSoundBanks()
        self.assertEquals(loadedBanks, [])

        # Test clearing soundbanks
        self.audioManager.LoadBank("Init.bnk")
        self.audioManager.LoadBank("Music.bnk")
        self.audioManager.LoadBank("Interface.bnk")
        banksBeforeClear = self.audioManager.GetLoadedSoundBanks() 
        self.assertEquals(len(banksBeforeClear), 3)
        self.audioManager.ClearBanks()
        banksAfterClear = self.audioManager.GetLoadedSoundBanks() 
        self.assertEquals(banksAfterClear, [])

        # Test trying to load a soundbank that doesn't exist
        illegitimateBank = self.audioManager.LoadBank("FAKE.bnk")
        self.assertFalse(illegitimateBank)

        # Test with audio disabled
        self.audioManager.SetEnabled(False)

        legitimateBank = self.audioManager.LoadBank("Init.bnk")
        self.assertFalse(legitimateBank)

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
        self.audioManager.LoadBank("Init.bnk")
        self.audioManager.LoadBank("Common.bnk")
        self.audioManager.LoadBank("Atmos.bnk")
        eventName1 = "triglavian_atmo1_play"
        eventName2 = "titans_massacre_monument_play"
        emitter1 = audio2.AudEmitter("emitter1")
        emitter2 = audio2.AudEmitter("emitter2")

        self.assertTrue(emitter1.SendEvent(eventName1) > 0) 
        self.assertTrue(emitter2.SendEvent(eventName2) > 0)

        self.audioManager.StopAll()

        # Test with audio disabled.
        self.audioManager.SetEnabled(False)
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
        self.audioManager.SetEnabled(False)

        self.audioManager.EnableDebugDisplayAllEmitters()
        self.assertTrue(self.audioManager.GetDebugDisplayAllEmitters())