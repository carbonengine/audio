import json
import os
import unittest

import blue
from audio2.audiomanager import AudioManager

AUDIO_METADATA_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "test", "soundbanks", "AudioMetadata.json"))
SOUNDBANK_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "test", "soundbanks"))
ONE_SHOT_BNK = "TestOneShot.bnk"
ONE_SHOT_EVENT = "Play_TestOneShot"
COMMON_BNK = "Common.bnk"
LOOP_BNK = "TestLoop.bnk"
LOOP_EVENT = "Play_TestLoop"
LOAD_BANK_BNK = "TestLoadBank.bnk"
LOAD_BANK_EVENT = "Play_TestLoadBank"


def GetEventMetadataFromFile():
    """Get event metadata from file and returns it as a dict. Also converts eventIDs to long in the process."""
    with open(AUDIO_METADATA_FILEPATH, "r") as f:
        audioMetadata = json.loads(f.read())

    # eventID's have to be long or else CarbonAudio doesn't correctly grab it.
    for eventName, eventInfo in audioMetadata.iteritems():
        eventInfo["eventID"] = long(eventInfo["eventID"])

    return audioMetadata


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

    def alwaysTrueBoolean(self):
        '''This can be used with the PumpOsWithTimeout function to simply have it pump a few times.'''
        return True
