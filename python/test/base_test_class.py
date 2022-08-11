import unittest

import audio2
from eveaudio import ALL_SOUNDBANKS
from fsdBuiltData.common.wwiseEvents import WwiseEvents 

class BaseAudio2TestClass(unittest.TestCase):
    """Initializes Wwise for other test classes."""
    @classmethod
    def setUpClass(cls):
        super(BaseAudio2TestClass, cls).setUpClass()

        settings = audio2.AudSettings()
        settings.applicationName = "Audio2 Testing"
        settings.baseSoundbankPath = "res:/Audio/"
        settings.soundbankLanguage = "English(US)"
        cls.staticDataRepository = audio2.GetStaticDataRepository()
        cls.staticDataRepository.Initialize(WwiseEvents().wwiseEventsByEventName)
        cls.audioManager = audio2.GetOrCreateManager()
        cls.audioManager.UpdateSettings(settings)
        cls.audioManager.SetEnabled(True)
        for bank in ALL_SOUNDBANKS:
            cls.audioManager.LoadBank(bank)

    @classmethod
    def tearDownClass(cls):
        cls.audioManager.ClearBanks()