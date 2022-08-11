import audio2
from base_test_class import BaseAudio2TestClass


class TestAudEmitterMultiExposure(BaseAudio2TestClass):
    def tearDown(self):
        self.audioManager.StopAll()

    def setUp(self):
        self.audioManager.SetEnabled(True)

    def test_audlistener_getlistener(self):
        """Test that the AudListener always returns the same instance."""
        listener1 = audio2.GetListener()
        self.assertEquals(listener1.ID, 4)
        self.assertEquals(listener1, audio2.GetListener())

    def test_audlistener_setposition(self):
        """Test that AudListener::SetPosition works."""
        listener1 = audio2.GetListener()
        listener1.SetPosition((1, 0, 0), (0, 1, 0), (0, 0, 0))