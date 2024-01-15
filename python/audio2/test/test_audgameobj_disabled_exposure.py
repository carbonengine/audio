import audio2
from base_test_class import BaseAudio2TestClass
from const import LOOP_BNK, LOOP_EVENT


class TestDisabledAudGameObjExposure(BaseAudio2TestClass):
    def tearDown(self):
        self.audioManager.StopAllPlayingSounds()

    def setUp(self):
        # Even though this is a test for AudGameObjResource exposure, it is
        # not directly exposed through blue, but only exposed as a parent class
        # through other audio emitter classes. That is why these tests use AudEmitter.
        self.emitter = audio2.AudEmitter("emitter1")
        self.Initialize(defaultSoundBanks=[LOOP_BNK])
        self.audioManager.Enable()
        self.audioManager.Disable()

    def test_disabled_audgameobjresource_sendevent(self):
        """Test that AudGameObjResource::SendEvent returns 0 when audio is disabled"""
        self.assertEquals(self.emitter.SendEvent(LOOP_EVENT), 0)

    def test_disabled_audgameobjresource_stopevent(self):
        """Test that AudGameObjResource::StopSound while audio is disabled."""
        self.assertFalse(self.emitter.StopEvent(LOOP_EVENT, 10))
        self.assertEquals(len(self.emitter.GetPlayingEvents()), 0)

    def test_disabled_audgameobjresource_stopsound(self):
        """Test that AudGameObjReseource::StopSound while audio is disabled."""
        self.assertFalse(self.emitter.StopSound(1, 10))
        self.assertEquals(len(self.emitter.GetPlayingEvents()), 0)

    def test_disabled_audgameobjresource_stopall(self):
        """Test that AudGameObjResource::StopAll() works while audio is disabled."""
        self.emitter.StopAll()
        self.assertEquals(len(self.emitter.GetPlayingEvents()), 0)

    def test_disabled_audgameobjresource_setattenuationscalingfactor(self):
        """Test that AudGameObjResource::SetAttenuationScalingFactor works."""
        self.assertFalse(self.emitter.SetAttenuationScalingFactor(0.5))

    def test_disabled_audgameobjresource_setswitch(self):
        """Test that AudGameObjResource::SetSwitch works while audio is disabled."""
        self.assertFalse(self.emitter.SetSwitch("wormhole_mass", "large"))

    def test_disabled_audgameobjresource_setrtpc(self):
        """Test that AudGameObjResource::SetRTPC works while audio is disabled."""
        self.assertFalse(self.emitter.SetRTPC("anything_works", 10))

    def test_disabled_audgameobjresource_seekoneventms(self):
        """Test that AudGameObjResource::SeekOnEventMs works while audio is disabled."""
        self.assertFalse(self.emitter.SeekOnEventMs(5000, 10))

    def test_disabled_audgameobjresource_seekoneventpercent(self):
        """Test that AudGameObjResource::SeekOnEventPercent works while audio is disabled."""
        self.assertFalse(self.emitter.SeekOnEventPercent(5000, 0.5))