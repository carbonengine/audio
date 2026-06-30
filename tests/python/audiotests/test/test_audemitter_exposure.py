# Copyright © 2022 CCP ehf.

from audiotests.base_test_class import BaseAudio2TestClass


class TestAudEmitterExposure(BaseAudio2TestClass):
    def setUp(self):
        import audio2
        self.emitter = audio2.AudEmitter("emitter1")
        self.Initialize()
        self.audioManager.Enable()


    def test_audioemitter_debug(self):
        """Test that all methods having to do with debugging in AudEmitter work."""
        self.emitter.SetPosition((1,0,0), (0,1,0), (0,50,0))
