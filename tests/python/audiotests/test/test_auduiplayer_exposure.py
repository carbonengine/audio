# Copyright © 2022 CCP ehf.

from unittest.mock import MagicMock

from audiotests.base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK, ONE_SHOT_EVENT
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout


class TestAudUIPlayerExposure(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK])

    def setUp(self):
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

    def tearDown(self):
        self.audioManager.Disable()

    def test_auduiplayer_creation(self):
        """Test that only one instance of AudUIPlayer can be created and it has an ID of 2."""
        import audio2
        uiPlayer = audio2.GetUIPlayer()
        self.assertIsNotNone(uiPlayer)
        self.assertEqual(uiPlayer, audio2.GetUIPlayer())
        self.assertEqual(uiPlayer.ID, 2)

    def test_auduiplayer_sendevent(self):
        """Test that SendEvent is properly exposed through AudUIPlayer."""
        import audio2
        uiPlayer = audio2.GetUIPlayer()
        uiPlayer.SetPosition((0,0,0), (0,0,0), (0,0,0))
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        playingID = uiPlayer.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID > 0)

    def test_auduiplayer_sendeventwithcallback(self):
        """Test that AudUIPlayer::SendEventWithCallback works as expected."""
        import audio2
        callback = MagicMock()
        uiPlayer = audio2.GetUIPlayer()
        uiPlayer.eventSenderCallback = callback
        # Give time for UIPlayer to be woken up and not be culled because of the sound prioritization system.
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=1) 
        playingID = uiPlayer.SendEventWithCallback(ONE_SHOT_EVENT)
        self.assertTrue(playingID > 0)
        # Wait 7 seconds, because the one shot event is 7 seconds long and the callback is called when it's done.
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=80)
        callback.assert_called_once_with(ONE_SHOT_EVENT) 

    def test_auduiplayer_postdialogueevent(self):
        """Test that AudUIPlayer::PostDialogueEvent allows you to get the current playing position of an event."""
        import audio2
        uiPlayer = audio2.GetUIPlayer()
        uiPlayer.SetPosition((0,0,0), (0,0,0), (0,0,0))
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        playingID = uiPlayer.PostDialogueEvent(ONE_SHOT_EVENT)
        self.assertTrue(playingID > 0)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        playPosition = uiPlayer.GetEventPlayPosition(playingID)
        self.assertTrue(playPosition > 0)

        # Test invalid playingID with GetEventPlayPosition
        invalidPlayPosition = uiPlayer.GetEventPlayPosition(999999)
        self.assertEqual(invalidPlayPosition, -1)
