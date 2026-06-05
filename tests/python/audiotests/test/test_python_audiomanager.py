import unittest

from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK, ONE_SHOT_EVENT, SOUNDBANK_FILEPATH
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import GetAudioMetadataFromFile, PumpOSWithTimeout
from audio2.audiomanager import AUDIO_STATE_DISABLED, AUDIO_STATE_ENABLED

class TestPythonAudioManager(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def loadedSoundBanksAreEmpty(self):
        return self.audioManager.GetLoadedSoundBanks() == []

    def pumpUntil(self, conditionFunc, maxTries=10):
        self.assertTrue(
            PumpOSWithTimeout(lambda: not conditionFunc(), maxTries=maxTries),
            "Timed out waiting for audio test condition."
        )

    def expectedSoundBanksAreLoaded(self, expectedSoundBanks):
        return set(expectedSoundBanks).issubset(set(self.audioManager.GetLoadedSoundBanks()))

    def setUp(self):
        self.Initialize(defaultSoundBanks=[]) # Reset the default soundbanks
        self.audioManager.UnloadSoundBanks([COMMON_BNK, ONE_SHOT_BNK, LOOP_BNK])
        self.audioManager.Enable()

        self.assertTrue(
            PumpOSWithTimeout(self.loadedSoundBanksAreEmpty),
            "Timed out waiting for the init soundbank to load."
        )

    def tearDown(self):
        self.audioManager.Disable()

    def test_init_soundbank_is_loaded_when_enabled(self):
        from audio2.audiomanager import INIT_BANK
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])

    def test_disable_clears_banks(self):
        self.audioManager.LoadSoundBanks([COMMON_BNK, LOOP_BNK])
        self.pumpUntil(lambda: self.expectedSoundBanksAreLoaded([COMMON_BNK, LOOP_BNK]))
        self.assertTrue(set([COMMON_BNK, LOOP_BNK]).issubset(set(self.audioManager.GetLoadedSoundBanks())))

        self.audioManager.Disable()

        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [])
        self.assertEqual(self.audioManager.GetState(), AUDIO_STATE_DISABLED)

    def test_disable_then_enable_can_load_bank_and_play_event(self):
        from audio2.audiomanager import INIT_BANK
        import audio2

        self.audioManager.Disable()
        self.assertEqual(self.audioManager.GetState(), AUDIO_STATE_DISABLED)

        self.audioManager.Enable([COMMON_BNK, LOOP_BNK])
        self.assertEqual(self.audioManager.GetState(), AUDIO_STATE_ENABLED)
        self.pumpUntil(lambda: self.expectedSoundBanksAreLoaded([COMMON_BNK, LOOP_BNK]))

        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK, LOOP_BNK]))
        emitter = audio2.AudEmitter("lifecycleEmitter")
        emitter.SetPlacement((0,0,0), (0,0,0), (0,0,0))
        listener = audio2.GetListener()
        listener.SetPosition((0,0,0), (0,0,0), (0,0,0))
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        playingID = emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID > 0)
        emitter.StopAll()

    def test_stopped_loop_does_not_resume_after_disable_enable(self):
        import audio2

        self.audioManager.LoadSoundBanks([COMMON_BNK, LOOP_BNK])
        self.pumpUntil(lambda: self.expectedSoundBanksAreLoaded([COMMON_BNK, LOOP_BNK]))

        emitter = audio2.AudEmitter("stoppedLoopLifecycleEmitter")
        emitter.SetPlacement((0,0,0), (0,0,0), (0,0,0))
        listener = audio2.GetListener()
        listener.SetPosition((0,0,0), (0,0,0), (0,0,0))
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

        playingID = emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID > 0)
        self.assertEqual(list(emitter.GetPlayingEvents().values()), [LOOP_EVENT])

        self.assertTrue(emitter.StopEvent(LOOP_EVENT, 1000))
        self.audioManager.Disable()
        self.audioManager.Enable()
        self.pumpUntil(lambda: emitter.GetPlayingEvents() == {})

        self.assertEqual(emitter.GetPlayingEvents(), {})

    def test_loop_stopped_by_posted_event_does_not_resume_after_disable_enable(self):
        import audio2

        audioMetadata = GetAudioMetadataFromFile()
        audioMetadata["Events"][LOOP_EVENT]["eventsStoppedBy"] = [ONE_SHOT_EVENT]
        self.audioManager.Disable()
        self.audioManager.Initialize(audioMetadata, defaultSoundBanks=[])
        self.audioManager.Enable([COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK])
        self.pumpUntil(lambda: self.expectedSoundBanksAreLoaded([COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK]))

        emitter = audio2.AudEmitter("postedStopLifecycleEmitter")
        emitter.SetPlacement((0,0,0), (0,0,0), (0,0,0))
        listener = audio2.GetListener()
        listener.SetPosition((0,0,0), (0,0,0), (0,0,0))
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

        loopPlayingID = emitter.SendEvent(LOOP_EVENT)
        self.assertTrue(loopPlayingID > 0)
        self.assertEqual(list(emitter.GetPlayingEvents().values()), [LOOP_EVENT])

        stopPlayingID = emitter.SendEvent(ONE_SHOT_EVENT)
        self.assertTrue(stopPlayingID > 0)
        self.audioManager.Disable()
        self.audioManager.Enable()
        self.pumpUntil(lambda: LOOP_EVENT not in emitter.GetPlayingEvents().values())

        self.assertNotIn(LOOP_EVENT, emitter.GetPlayingEvents().values())

    def test_engine_state_transitions_across_disable_enable(self):
        self.audioManager.Disable()
        self.assertEqual(self.audioManager.GetState(), AUDIO_STATE_DISABLED)

        self.audioManager.Enable()
        self.assertEqual(self.audioManager.GetState(), AUDIO_STATE_ENABLED)

    def test_unload_soundbank_doesnt_remove_init_bank(self):
        from audio2.audiomanager import INIT_BANK
        self.audioManager.UnloadSoundBank(INIT_BANK)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK])

    def test_unload_soundbank_doesnt_remove_default_banks(self):
        from audio2.audiomanager import INIT_BANK
        self.audioManager.Disable()
        self.Initialize(defaultSoundBanks=[ONE_SHOT_BNK])
        self.audioManager.Enable()
        self.assertTrue(
            PumpOSWithTimeout(self.loadedSoundBanksAreEmpty),
            "Timed out waiting for the init soundbank to load."
        )
        self.audioManager.UnloadSoundBank(ONE_SHOT_BNK)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()),set([INIT_BANK, ONE_SHOT_BNK]))

    def test_swap_soundbanks_keeps_default_and_init_soundbanks(self):
        from audio2.audiomanager import INIT_BANK
        self.audioManager.LoadSoundBank(LOOP_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK, LOOP_BNK])

        self.audioManager.SwapSoundBanks([ONE_SHOT_BNK])
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()),set([INIT_BANK, ONE_SHOT_BNK]))
        self.audioManager.SwapSoundBanks([])
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(),[INIT_BANK])

    def test_banks_already_loaded_when_disabled_load_again_when_enabled(self):
        from audio2.audiomanager import INIT_BANK
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK])
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.audioManager.Disable()
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK, ONE_SHOT_BNK]))

    def test_banks_requested_when_disabled_get_loaded_when_enabled(self):
        """Test that any request to load a bank/unload a bank gets tracked while the audio manager is disabled."""
        from audio2.audiomanager import INIT_BANK
        self.audioManager.Disable()
        self.audioManager.LoadSoundBanks([COMMON_BNK, ONE_SHOT_BNK])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK, ONE_SHOT_BNK]))

        self.audioManager.Disable()
        self.audioManager.SwapSoundBanks([LOOP_BNK])
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=2)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, LOOP_BNK]))

    def test_adding_and_remove_default_soundbank(self):
        """Test that adding a soundbank as default doesn't remove it when unloading. Alternatively test removing a default soundbank."""
        from audio2.audiomanager import INIT_BANK

        self.audioManager.AddAndLoadDefaultSoundBank(COMMON_BNK)
        self.audioManager.UnloadSoundBank(COMMON_BNK)

        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(set(self.audioManager.GetLoadedSoundBanks()), set([INIT_BANK, COMMON_BNK]))

        self.audioManager.RemoveAndUnloadDefaultSoundBank(COMMON_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(self.audioManager.GetLoadedSoundBanks(), [INIT_BANK])


if __name__ == "__main__":
    unittest.main()
