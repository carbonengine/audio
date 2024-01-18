import audio2
import blue

from base_test_class import COMMON_BNK, LOOP_BNK, LOOP_EVENT, ONE_SHOT_BNK, ONE_SHOT_EVENT
from base_test_class import BaseAudio2TestClass
from utils import PumpOSWithTimeout


class TestAudUIPlayerExposure(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super(TestAudUIPlayerExposure, cls).setUpClass()
        cls.Initialize(cls, defaultSoundBanks=[COMMON_BNK, LOOP_BNK, ONE_SHOT_BNK])

    def setUp(self):
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

    def tearDown(self):
        self.audioManager.Disable()

    def test_auduiplayer_creation(self):
        """Test that only one instance of AudUIPlayer can be created and it has an ID of 2."""
        uiPlayer = audio2.GetUIPlayer()
        self.assertIsNotNone(uiPlayer)
        self.assertEquals(uiPlayer, audio2.GetUIPlayer())
        self.assertEquals(uiPlayer.ID, 2)

    def test_auduiplayer_sendevent(self):
        """Test that SendEvent is properly exposed through AudUIPlayer."""
        uiPlayer = audio2.GetUIPlayer()
        playingID = uiPlayer.SendEvent(LOOP_EVENT)
        self.assertTrue(playingID > 0)

    def test_auduiplayer_sendeventwithcallback(self):
        """Test that AudUIPlayer::SendEventWithCallback works as expected."""
        def callback(event):
            self.assertTrue(event == LOOP_EVENT)

        uiPlayer = audio2.GetUIPlayer()
        uiPlayer.eventSenderCallback = callback
        uiPlayer.SendEventWithCallback(LOOP_EVENT)

    def test_auduiplayer_postdialogueevent(self):
        """Test that AudUIPlayer::PostDialogueEvent allows you to get the current playing position of an event."""
        uiPlayer = audio2.GetUIPlayer()
        playingID = uiPlayer.PostDialogueEvent(ONE_SHOT_EVENT)
        self.assertTrue(playingID > 0)
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        playPosition = uiPlayer.GetEventPlayPosition(playingID)
        self.assertTrue(playPosition > 0)

        # Test invalid playingID with GetEventPlayPosition
        invalidPlayPosition = uiPlayer.GetEventPlayPosition(999999)
        self.assertEquals(invalidPlayPosition, -1)
        
