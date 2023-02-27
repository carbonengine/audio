import unittest
import uthread2

import audio2
from base_test_class import BaseAudio2TestClass


class TestAudUIPlayerExposure(BaseAudio2TestClass):
    def setUp(self):
        self.audioManager.SetEnabled(True)

    def test_auduiplayer_creation(self):
        """Test that only one instance of AudUIPlayer can be created and it has an ID of 2."""
        uiPlayer = audio2.GetUIPlayer()
        self.assertIsNotNone(uiPlayer)
        self.assertEquals(uiPlayer, audio2.GetUIPlayer())
        self.assertEquals(uiPlayer.ID, 2)

    def test_auduiplayer_sendevent(self):
        """Test that SendEvent is properly exposed through AudUIPlayer."""
        uiPlayer = audio2.GetUIPlayer()
        playingID = uiPlayer.SendEvent("map_constellation_loop_play")
        self.assertTrue(playingID > 0)

    def test_auduiplayer_sendeventwithcallback(self):
        """Test that AudUIPlayer::SendEventWithCallback works as expected."""
        eventToSend = "ui_button_soft1_play"
        def callback(event):
            self.assertTrue(event == eventToSend)

        uiPlayer = audio2.GetUIPlayer()
        uiPlayer.eventSenderCallback = callback
        uiPlayer.SendEventWithCallback(eventToSend)

    def test_auduiplayer_postdialogueevent(self):
        """Test that AudUIPlayer::PostDialogueEvent allows you to get the current playing position of an event."""
        uiPlayer = audio2.GetUIPlayer()
        playingID = uiPlayer.PostDialogueEvent("voc_aura_2755_1_play")
        self.assertTrue(playingID > 0)
        uthread2.sleep(0.5)
        playPosition = uiPlayer.GetEventPlayPosition(playingID)
        self.assertTrue(playPosition > 0)

        # Test invalid playingID with GetEventPlayPosition
        invalidPlayPosition = uiPlayer.GetEventPlayPosition(999999)
        self.assertEquals(invalidPlayPosition, -1)
        
