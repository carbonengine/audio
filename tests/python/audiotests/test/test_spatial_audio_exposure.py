from unittest.mock import MagicMock

from audiotests.base_test_class import SOUNDBANK_FILEPATH
from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.base_test_class import GetAudioMetadataFromFile
from audiotests.utils import PumpOSWithTimeout
from audio2.audiomanager import AUDIO_STATE_DISABLED

class TestSpatialAudio(BaseAudio2TestClass):
    """Test spatial audio related features."""

    def _audio_device_supports_spatial_audio(self, deviceChangeCallback):
        """A check to see if the currently active audio device supports spatial audio."""
        numTries = 0
        spatialAudioSupported = False
        while numTries < 10:
            if deviceChangeCallback.called:
                args, kwargs = deviceChangeCallback.call_args
                if args[0] == True:
                    spatialAudioSupported = True
                    break

            PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=1)
            numTries += 1

        return spatialAudioSupported

    def setUp(self):
        self.audioManager.Disable()
        self.deviceChangeCallback = MagicMock()

    def test_spatial_audio_endpoints_with_default_settings(self):
        """Test spatial audio endpoints with default settings to make sure they work"""
        if not self.audioManager.SpatialAudioIsSupported(): 
            print("Skipping this test because Carbon Audio does not support spatial audio on this platform...")
            return

        settings = self.audioManager._CreateAudioSettings(
            "soundbanks:/", 
            "Audio2 Testing", 
            "English(US)", 
        )
        self.audioManager.manager.UpdateSettings(settings)
        self.audioManager.RegisterAudioDeviceChangeCallback(self.deviceChangeCallback)
        self.Initialize()
        self.audioManager.Enable()

        if not self._audio_device_supports_spatial_audio(self.deviceChangeCallback):
            print("Skipping this test because the user's audio output does not support spatial audio...")
            return

        self.assertTrue(self.audioManager.EnableSpatialAudio())

        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertTrue(self.audioManager.DisableSpatialAudio())

        # Give a bit more time for the asynchronous bits to happen
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

    def test_spatial_audio_disable_works_with_wrong_settings(self):
        """Disabling spatial audio with a nonexistant stereo audio device should error out."""
        if not self.audioManager.SpatialAudioIsSupported(): 
            print("Skipping this test because Carbon Audio does not support spatial audio on this platform...")
            return

        settings = self.audioManager._CreateAudioSettings(
            "soundbanks:/", 
            "English(US)", 
            "Audio2 Testing", 
            stereoAudioDeviceName ="fake_system_stereo",
        )
        self.audioManager.manager.UpdateSettings(settings)
        self.audioManager.RegisterAudioDeviceChangeCallback(self.deviceChangeCallback)

        self.Initialize()
        self.audioManager.Enable()

        if not self._audio_device_supports_spatial_audio(self.deviceChangeCallback):
            print("Skipping this test because the user's audio output does not support spatial audio...")
            return

        self.assertFalse(self.audioManager.DisableSpatialAudio())
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

    def test_spatial_audio_enable_works_with_wrong_settings(self):
        """Enabling spatial audio with a nonexistant spatial audio device should error out."""
        if not self.audioManager.SpatialAudioIsSupported(): 
            print("Skipping this test because Carbon Audio does not support spatial audio on this platform...")
            return

        settings = self.audioManager._CreateAudioSettings(
            "soundbanks:/", 
            "English(US)", 
            "Audio2 Testing", 
            stereoAudioDeviceName="System_Stereo",
            spatialAudioDeviceName="fake_system_spatial",
        )
        self.audioManager.manager.UpdateSettings(settings)
        self.audioManager.RegisterAudioDeviceChangeCallback(self.deviceChangeCallback)

        self.Initialize()
        self.audioManager.Enable()

        if not self._audio_device_supports_spatial_audio(self.deviceChangeCallback):
            print("Skipping this test because the user's audio output does not support spatial audio...")
            return

        self.assertTrue(self.audioManager.DisableSpatialAudio())
        # Try and enable spatial audio with an audio device that is non existant which should not work.
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)
        self.assertFalse(self.audioManager.EnableSpatialAudio())
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

    def test_spatial_audio_endpoints_dont_crash_when_audio_not_enabled(self):
        self.Initialize()
        self.audioManager.Enable()
        self.audioManager.Disable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

        self.assertEqual(self.audioManager.GetState(), AUDIO_STATE_DISABLED)
        self.assertFalse(self.audioManager.DisableSpatialAudio())
        self.assertFalse(self.audioManager.EnableSpatialAudio())

    def test_spatial_audio_enabled_when_initializing(self):
        if not self.audioManager.SpatialAudioIsSupported(): 
            print("Skipping this test because Carbon Audio does not support spatial audio on this platform...")
            return

        self.Initialize()
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=1)

        self.assertTrue(self.audioManager.GetSpatialAudioEnabled())

    def test_spatial_audio_disabled_when_initializing(self):
        if not self.audioManager.SpatialAudioIsSupported(): 
            print("Skipping this test because Carbon Audio does not support spatial audio on this platform...")
            return

        settings = self.audioManager._CreateAudioSettings(
            "soundbanks:/", 
            "English(US)", 
            "Audio2 Testing", 
            spatialAudioEnabled=False 
        )
        self.audioManager.manager.UpdateSettings(settings)
        self.Initialize()
        self.audioManager.Enable()
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=1)

        self.assertFalse(self.audioManager.GetSpatialAudioEnabled())
