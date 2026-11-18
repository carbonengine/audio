# Copyright © 2026 CCP ehf.

from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import RestoreSettings, SaveSettings


SETTINGS = (
    "numberOfPrimaryRays",
    "maxReflectionOrder",
    "maxDiffractionOrder",
    "maxDiffractionPaths",
    "maxGlobalReflectionPaths",
    "maxEmitterRoomAuxSends",
    "maxSoundPropagationDepth",
    "cpuLimitPercentage",
    "loadBalancingSpread",
    "smoothingConstantMs",
    "adjacentRoomBleed",
    "transmissionOperation",
    "transmissionLoss",
)


class TestSpatialAudioSettingsExposure(BaseAudio2TestClass):
    """Tests the Spatial Audio settings: their limits, and that the live ones can change while Wwise runs."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        self.manager = self.audioManager.manager
        self.settings = SaveSettings(self.manager, SETTINGS)

    def tearDown(self):
        RestoreSettings(self.manager, self.settings)
        self.manager.spatialAudioGeometryEnabled = False
        self.audioManager.Disable()

    def test_settings_are_clamped_to_what_wwise_accepts(self):
        self.manager.maxReflectionOrder = 9
        self.assertEqual(self.manager.maxReflectionOrder, 4)

        self.manager.maxDiffractionOrder = 20
        self.assertEqual(self.manager.maxDiffractionOrder, 8)

        self.manager.maxDiffractionPaths = 100
        self.assertEqual(self.manager.maxDiffractionPaths, 32)

        self.manager.maxSoundPropagationDepth = 100
        self.assertEqual(self.manager.maxSoundPropagationDepth, 8)

        self.manager.loadBalancingSpread = 0
        self.assertEqual(self.manager.loadBalancingSpread, 1)

        self.manager.cpuLimitPercentage = 150.0
        self.assertEqual(self.manager.cpuLimitPercentage, 100.0)

        self.manager.transmissionOperation = 7
        self.assertEqual(self.manager.transmissionOperation, 2)

        self.manager.transmissionLoss = 2.0
        self.assertEqual(self.manager.transmissionLoss, 1.0)

        self.manager.adjacentRoomBleed = 0.0
        self.assertGreater(self.manager.adjacentRoomBleed, 0.0)

    def test_nan_leaves_a_setting_as_it_was(self):
        for name in ("cpuLimitPercentage", "smoothingConstantMs", "adjacentRoomBleed", "transmissionLoss"):
            before = getattr(self.manager, name)
            setattr(self.manager, name, float("nan"))
            self.assertEqual(getattr(self.manager, name), before, name)

    def test_live_settings_can_change_while_spatial_audio_runs(self):
        self.audioManager.Enable()
        self.manager.spatialAudioGeometryEnabled = True
        if not self.manager.spatialAudioGeometryEnabled:
            self.skipTest("Spatial audio geometry is not available on this platform.")

        self.manager.numberOfPrimaryRays = 30
        self.manager.maxReflectionOrder = 1
        self.manager.maxDiffractionOrder = 3
        self.manager.maxDiffractionPaths = 4
        self.manager.maxGlobalReflectionPaths = 30
        self.manager.maxEmitterRoomAuxSends = 3
        self.manager.loadBalancingSpread = 4
        self.manager.smoothingConstantMs = 50.0
        self.manager.adjacentRoomBleed = 1.5
        self.manager.transmissionOperation = 0

        self.assertEqual(self.manager.numberOfPrimaryRays, 30)
        self.assertEqual(self.manager.maxDiffractionPaths, 4)
        self.assertEqual(self.manager.loadBalancingSpread, 4)
        self.assertEqual(self.manager.transmissionOperation, 0)
