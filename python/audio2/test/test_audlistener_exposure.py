import unittest

import audio2
from eveaudio import ALL_SOUNDBANKS 
from eveaudio.audiomanager import CreateAudioSettings, AudioManager

class TestAudio2Exposure(unittest.TestCase):
    def test_audio2_getorcreatemanager(self):
        """Test that audio2.GetOrCreateManager() returns an instance of AudioManager and it is a singleton."""
        manager = audio2.GetOrCreateManager()
        self.assertTrue(isinstance(manager, audio2.AudManager))
        self.assertEquals(manager, audio2.GetOrCreateManager())