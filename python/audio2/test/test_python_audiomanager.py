import unittest

from audio2.audiomanager import INIT_BANK
from base_test_class import BaseAudio2TestClass
from const import COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK, SOUNDBANK_FILEPATH
from utils import PumpOSWithTimeout

class TestPythonAudioManager(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super(TestPythonAudioManager, cls).setUpClass()
        cls.Initialize(cls)

    def loadedSoundBanksAreEmpty(self):
        return self.audioManager.GetLoadedSoundBanks() == []

    def setUp(self):
        self.Initialize(defaultSoundBanks=[]) # Reset the default soundbanks
        self.audioManager.UnloadSoundBanks([COMMON_BNK, ONE_SHOT_BNK, LOOP_BNK])
        self.audioManager.Enable()

        PumpOSWithTimeout(self.loadedSoundBanksAreEmpty)

    def tearDown(self):
        self.audioManager.Disable()

    def test_init_soundbank_is_loaded_when_enabled(self):
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])

    def test_unload_soundbank_doesnt_remove_init_bank(self):
        self.audioManager.UnloadSoundBank(INIT_BANK)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK])

    def test_unload_soundbank_doesnt_remove_default_banks(self):
        self.audioManager.Disable()
        self.Initialize(defaultSoundBanks=[ONE_SHOT_BNK])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.loadedSoundBanksAreEmpty)
        self.audioManager.UnloadSoundBank(ONE_SHOT_BNK)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()),set([INIT_BANK, ONE_SHOT_BNK]))

    def test_swap_soundbanks_keeps_default_and_init_soundbanks(self):
        self.audioManager.LoadSoundBank(LOOP_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK, LOOP_BNK])
        self.audioManager.SwapSoundBanks([ONE_SHOT_BNK])
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()),set([INIT_BANK, ONE_SHOT_BNK]))
        self.audioManager.SwapSoundBanks([])
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK])

    def test_banks_already_loaded_when_disabled_load_again_when_enabled(self):
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK])
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.audioManager.Disable()
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK, ONE_SHOT_BNK]))

    def test_banks_requested_when_disabled_get_loaded_when_enabled(self):
        """Test that any request to load a bank/unload a bank gets tracked while the audio manager is disabled."""
        self.audioManager.Disable()
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK, ONE_SHOT_BNK]))

        self.audioManager.Disable()
        self.audioManager.SwapSoundBanks([LOOP_BNK])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEquals(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, LOOP_BNK]))

    def test_adding_and_remove_default_soundbank(self):
        """Test that adding a soundbank as default doesn't remove it when unloading. Alternatively test removing a default soundbank."""
        
        self.audioManager.AddAndLoadDefaultSoundBank(COMMON_BNK)
        self.audioManager.UnloadSoundBank(COMMON_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK]))

        self.audioManager.RemoveAndUnloadDefaultSoundBank(COMMON_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEquals(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])


if __name__ == "__main__":
    unittest.main()
