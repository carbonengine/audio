import json
import os
import unittest

import blue
from .utils import GetAudioMetadataFromFile
from .debug_utils import debug_delay_if_enabled

SOUNDBANK_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "test", "soundbanks"))
ONE_SHOT_BNK = "TestOneShot.bnk"
ONE_SHOT_EVENT = "Play_TestOneShot"
COMMON_BNK = "Common.bnk"
LOOP_BNK = "TestLoop.bnk"
LOOP_EVENT = "Play_TestLoop"
LOAD_BANK_BNK = "TestLoadBank.bnk"
LOAD_BANK_EVENT = "Play_TestLoadBank"
ESSENTIAL_BNK = "TestMusicEssential.bnk"
ESSENTIAL_EVENT = "Play_TestEssential"
NONESSENTIAL_BNK = "NonEssentialSoundBank.bnk"
NONESSENTIAL_BNK_EVENT = "Play_NonEssentialBank"
NONESSENTIAL_STREAM_BNK = "NonEssentialStream.bnk"
NONESSENTIAL_STREAM_EVENT = "Play_NonEssentialStream"


class BaseAudio2TestClass(unittest.TestCase):
    """Initializes Wwise for other test classes."""
    @classmethod
    def setUpClass(cls):
        from audio2.audiomanager import AudioManager
        super(BaseAudio2TestClass, cls).setUpClass()

        # Add a delay to the test if configured.
        debug_delay_if_enabled()

        blue.paths.SetSearchPath("soundbanks", SOUNDBANK_FILEPATH)
        applicationName = "Audio2 Testing"
        baseSoundbankPath = "soundbanks:/"
        languageDirectory = "English(US)"
        cls.audioManager = AudioManager(baseSoundbankPath, languageDirectory, applicationName)

    def Initialize(self, defaultSoundBanks=[]):
        audioMetadata = GetAudioMetadataFromFile()
        self.audioManager.Initialize(audioMetadata, defaultSoundBanks=defaultSoundBanks)

    def alwaysTrueBoolean(self):
        '''This can be used with the PumpOsWithTimeout function to simply have it pump a few times.'''
        return True
