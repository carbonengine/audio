import blue
from audio2.audiomanager import AudioManager

from const import SOUNDBANK_FILEPATH, ALL_SOUNDBANKS
from utils import GetAudioMetadataFromFile

class TestAudioManager(AudioManager):
    """An audio manager that automatically loads all sounbanks needed for testing."""

    def __init__(self, applicationName):
        blue.paths.SetSearchPath("soundbanks", SOUNDBANK_FILEPATH)
        super(TestAudioManager, self).__init__("soundbanks:/", "English(US)", applicationName)
        self.Initialize(GetAudioMetadataFromFile(), defaultSoundBanks=ALL_SOUNDBANKS)
