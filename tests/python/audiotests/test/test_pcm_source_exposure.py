# Copyright © 2026 Fenris Creations

from audiotests.base_test_class import BaseAudio2TestClass, COMMON_BNK, PCM_BNK, PCM_EVENT, PCM_SOURCE
from audiotests.utils import PumpOSWithTimeout, WaitForEmitterToWake, WaitForSoundBanksToLoad

SAMPLE_RATE = 24000


def Silence(seconds, channels=1):
    """16-bit PCM silence, as a WAV's data chunk."""
    return bytes(int(SAMPLE_RATE * seconds) * channels * 2)


class TestPcmSourceExposure(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK, PCM_BNK])

    def setUp(self):
        import audio2
        self.audioManager.Enable()
        self.manager = audio2.GetOrCreateManager()
        self.uiPlayer = audio2.GetUIPlayer()
        self.uiPlayer.SetPlacement((0, 0, 0), (0, 0, 0), (0, 0, 0))
        self.assertTrue(WaitForSoundBanksToLoad([PCM_EVENT]), "Timed out waiting for the test SoundBanks to load.")
        self.assertTrue(WaitForEmitterToWake(self.uiPlayer), "Timed out waiting for the UI player to be woken up.")
        self.assertTrue(self.WaitForHeldPcm(0), "PCM from an earlier post was still held.")

    def tearDown(self):
        self.audioManager.Disable()

    def HeldPcm(self):
        return self.manager.GetHeldPcmSourceCount()

    def WaitForHeldPcm(self, count, maxTries=50):
        return PumpOSWithTimeout(lambda: self.HeldPcm() != count, maxTries=maxTries)

    def AssertPlaysForItsLength(self, channels):
        """A 2 s post is still held after about 1 s and released by about 4 s. Wwise ends an event at once
        when it can't read its source, so this is what tells a playable WEM from a broken one."""
        playingID = self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(2, channels), SAMPLE_RATE, channels)
        self.assertGreater(playingID, 0)
        self.assertEqual(self.HeldPcm(), 1)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=10)
        self.assertEqual(self.HeldPcm(), 1, "The PCM was released before it could have finished playing.")
        self.assertTrue(self.WaitForHeldPcm(0, maxTries=30), "The PCM was not released after its event ended.")

    def test_mono_post_plays_and_is_released_at_end_of_event(self):
        self.AssertPlaysForItsLength(channels=1)

    def test_stereo_post_plays_and_is_released_at_end_of_event(self):
        self.AssertPlaysForItsLength(channels=2)

    def test_pcm_can_be_any_buffer(self):
        playingID = self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, bytearray(Silence(0.2)), SAMPLE_RATE, 1)
        self.assertGreater(playingID, 0)
        self.assertTrue(self.WaitForHeldPcm(0))

    def test_stopped_post_is_released(self):
        playingID = self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(30), SAMPLE_RATE, 1)
        self.assertGreater(playingID, 0)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(self.HeldPcm(), 1)
        self.uiPlayer.StopSound(playingID, 100)
        self.assertTrue(self.WaitForHeldPcm(0), "The PCM was not released after its event was stopped.")

    def test_stop_all_releases_every_post(self):
        for _ in range(3):
            self.assertGreater(self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(30), SAMPLE_RATE, 1), 0)
        self.assertEqual(self.HeldPcm(), 3)
        self.uiPlayer.StopAll()
        self.assertTrue(self.WaitForHeldPcm(0), "The PCM was not released after StopAll.")

    def test_post_cut_off_by_disable_is_released_and_not_played_on_enable(self):
        """Static data calls an External Source event a loop, but culling must not queue a PCM post to play again."""
        self.assertGreater(self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(30), SAMPLE_RATE, 1), 0)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.audioManager.Disable()
        self.assertTrue(self.WaitForHeldPcm(0), "The PCM was not released after audio was disabled.")
        self.audioManager.Enable()
        self.assertTrue(WaitForEmitterToWake(self.uiPlayer), "Timed out waiting for the UI player to be woken up.")
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertEqual(self.uiPlayer.GetPlayingEvents(), {})

    def test_destroyed_emitter_still_releases_its_post(self):
        import audio2
        emitter = audio2.AudEmitter("pcmEmitter")
        emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 0, 0))
        self.assertTrue(WaitForEmitterToWake(emitter), "Timed out waiting for the emitter to be woken up.")
        self.assertGreater(emitter.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(0.5), SAMPLE_RATE, 1), 0)
        del emitter
        self.assertTrue(self.WaitForHeldPcm(0), "The PCM was not released after its emitter was destroyed.")

    def test_culled_emitter_returns_zero_and_holds_nothing(self):
        import audio2
        emitter = audio2.AudEmitter("culledPcmEmitter")  # never placed, so never woken
        self.assertTrue(emitter.IsCulled())
        self.assertEqual(emitter.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(0.2), SAMPLE_RATE, 1), 0)
        self.assertEqual(self.HeldPcm(), 0)
        self.assertEqual(emitter.GetPlayingEvents(), {})

    def test_disabled_audio_returns_zero_and_holds_nothing(self):
        self.audioManager.Disable()
        self.assertEqual(self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(0.2), SAMPLE_RATE, 1), 0)
        self.assertEqual(self.HeldPcm(), 0)

    def test_bank_still_loading_returns_zero_and_is_not_played_later(self):
        self.manager.UnloadBank(PCM_BNK)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=5)
        self.manager.LoadBank(PCM_BNK)
        self.assertEqual(self.uiPlayer.SendEventWithPcm(PCM_EVENT, PCM_SOURCE, Silence(2), SAMPLE_RATE, 1), 0)
        self.assertEqual(self.HeldPcm(), 0)
        self.assertTrue(WaitForSoundBanksToLoad([PCM_EVENT]), "Timed out waiting for the SoundBank to load again.")
        self.assertEqual(self.uiPlayer.GetPlayingEvents(), {})

    def test_unplayable_pcm_returns_zero(self):
        cases = {
            "three channels": (PCM_SOURCE, Silence(0.2, channels=3), SAMPLE_RATE, 3),
            "no channels": (PCM_SOURCE, Silence(0.2), SAMPLE_RATE, 0),
            "not whole 16-bit samples": (PCM_SOURCE, Silence(0.2) + b"\0", SAMPLE_RATE, 1),
            "not whole stereo frames": (PCM_SOURCE, Silence(0.2) + b"\0\0", SAMPLE_RATE, 2),
            "empty": (PCM_SOURCE, b"", SAMPLE_RATE, 1),
            "no sample rate": (PCM_SOURCE, Silence(0.2), 0, 1),
            "not a buffer": (PCM_SOURCE, "not bytes", SAMPLE_RATE, 1),
            "no source name": ("", Silence(0.2), SAMPLE_RATE, 1),
        }
        for case, (sourceName, pcm, sampleRate, channels) in cases.items():
            with self.subTest(case):
                self.assertEqual(self.uiPlayer.SendEventWithPcm(PCM_EVENT, sourceName, pcm, sampleRate, channels), 0)
                self.assertEqual(self.HeldPcm(), 0)

    def test_unknown_source_name_is_released(self):
        """Wwise only finds out the source is missing when the event plays, so the post succeeds and then ends."""
        playingID = self.uiPlayer.SendEventWithPcm(PCM_EVENT, "no_such_source", Silence(5), SAMPLE_RATE, 1)
        self.assertGreater(playingID, 0)
        self.assertTrue(self.WaitForHeldPcm(0, maxTries=20), "The PCM was not released soon after its source failed to resolve.")
