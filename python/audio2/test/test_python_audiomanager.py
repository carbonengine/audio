import os
import shutil
import unittest

from audio2.audiomanager import INIT_BANK
from base_test_class import COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK, SOUNDBANK_FILEPATH
from base_test_class import BaseAudio2TestClass
from utils import run_in_tasklet

class TestPythonAudioManager(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super(TestPythonAudioManager, cls).setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        self.Initialize(defaultSoundBanks=[]) # Reset the default soundbanks
        self.audioManager.UnloadSoundBanks([COMMON_BNK, ONE_SHOT_BNK, LOOP_BNK])
        self.audioManager.Enable()

    def tearDown(self):
        self.audioManager.Disable()

    @run_in_tasklet
    def test_init_soundbank_is_loaded_when_enabled(self):
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])

    @run_in_tasklet
    def test_unload_soundbank_doesnt_remove_init_bank(self):
        self.audioManager.UnloadSoundBank(INIT_BANK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK])

    @run_in_tasklet
    def test_unload_soundbank_doesnt_remove_default_banks(self):
        self.audioManager.Disable()
        self.Initialize(defaultSoundBanks=[ONE_SHOT_BNK])
        self.audioManager.Enable()
        self.audioManager.UnloadSoundBank(ONE_SHOT_BNK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK, ONE_SHOT_BNK])

    @run_in_tasklet
    def test_swap_soundbanks_keeps_default_and_init_soundbanks(self):
        self.audioManager.LoadSoundBank(LOOP_BNK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK, LOOP_BNK])
        self.audioManager.SwapSoundBanks([ONE_SHOT_BNK])
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK, ONE_SHOT_BNK])
        self.audioManager.SwapSoundBanks([])
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK])

    @run_in_tasklet
    def test_reload_soundbanks(self):
        """Test that ReloadSoundBanks returns an empty list if successful and banks that failed if not."""
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK, LOOP_BNK])
        self.assertEquals(self.audioManager.ReloadSoundBanks(), [])

        oneShotBankFilepath = os.path.join(SOUNDBANK_FILEPATH, ONE_SHOT_BNK)
        movedOneShotBankFilepath = os.path.join(SOUNDBANK_FILEPATH, "{}.tmp".format(ONE_SHOT_BNK))
        shutil.move(oneShotBankFilepath, movedOneShotBankFilepath)
        self.assertEquals(self.audioManager.ReloadSoundBanks(), [ONE_SHOT_BNK])
        shutil.move(movedOneShotBankFilepath, oneShotBankFilepath)

    @run_in_tasklet
    def test_banks_already_loaded_when_disabled_load_again_when_enabled(self):
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK])
        self.audioManager.Disable()
        self.audioManager.Enable()
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK, COMMON_BNK, ONE_SHOT_BNK])

    @run_in_tasklet
    def test_banks_requested_when_disabled_get_loaded_when_enabled(self):
        """Test that any request to load a bank/unload a bank gets tracked while the audio manager is disabled."""
        self.audioManager.Disable()
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK])
        self.audioManager.Enable()
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK, COMMON_BNK, ONE_SHOT_BNK])

        self.audioManager.Disable()
        self.audioManager.SwapSoundBanks([LOOP_BNK])
        self.audioManager.Enable()
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK, LOOP_BNK])

    @run_in_tasklet
    def test_adding_and_remove_default_soundbank(self):
        """Test that adding a soundbank as default doesn't remove it when unloading. Alternatively test removing a default soundbank."""
        self.audioManager.AddAndLoadDefaultSoundBank(COMMON_BNK)
        self.audioManager.UnloadSoundBank(COMMON_BNK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK, COMMON_BNK])

        self.audioManager.RemoveAndUnloadDefaultSoundBank(COMMON_BNK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])


if __name__ == "__main__":
    unittest.main()
