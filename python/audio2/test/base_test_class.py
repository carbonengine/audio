import os
import unittest

import blue
from audio2.audiomanager import AudioManager

from const import SOUNDBANK_FILEPATH
from utils import GetEventMetadataFromFile


class BaseAudio2TestClass(unittest.TestCase):
    """Initializes Wwise for other test classes."""
    @classmethod
    def setUpClass(cls):
        super(BaseAudio2TestClass, cls).setUpClass()

        blue.paths.SetSearchPath("soundbanks", SOUNDBANK_FILEPATH)
        applicationName = "Audio2 Testing"
        baseSoundbankPath = "soundbanks:/"
        languageDirectory = "English(US)"
        cls.audioManager = AudioManager(baseSoundbankPath, languageDirectory, applicationName)

    def Initialize(self, defaultSoundBanks=[]):
        audioMetadata = GetEventMetadataFromFile()
        self.audioManager.Initialize(audioMetadata, defaultSoundBanks=defaultSoundBanks)