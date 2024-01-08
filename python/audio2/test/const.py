import os

AUDIO_METADATA_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "soundbanks", "SoundPrioritizationMetadata.json"))
SOUNDBANK_FILEPATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "soundbanks"))
ONE_SHOT_BNK = "TestOneShot.bnk"
ONE_SHOT_EVENT = "Play_TestOneShot"
COMMON_BNK = "Common.bnk"
LOOP_BNK = "TestLoop.bnk"
LOOP_EVENT = "Play_TestLoop"
VIDEOPLAYER_BNK = "VideoPlayer.bnk"

ALL_SOUNDBANKS = [COMMON_BNK, ONE_SHOT_BNK, LOOP_BNK, VIDEOPLAYER_BNK]
