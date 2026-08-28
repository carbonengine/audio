# Copyright © 2026 CCP ehf.

import blue

from audiotests.base_test_class import BaseAudio2TestClass
from audiotests.utils import PumpOSWithTimeout, WaitForEmitterToWake


class TestAwakeEmittersExposure(BaseAudio2TestClass):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.Initialize(cls)

    def setUp(self):
        import audio2
        self.emitter = audio2.AudEmitter("awakeEmitter1")
        self.emitter.SetPlacement((0,0,0), (0,0,0), (0,0,0))
        self.audioManager.Enable()

        self.listener = audio2.GetListener()
        self.listener.SetPosition((0,0,0), (0,0,0), (0,0,0))
        self.assertTrue(WaitForEmitterToWake(self.emitter), "Timed out waiting for the emitter to be woken up.")

    def tearDown(self):
        self.audioManager.manager.ResetCullingSettings()
        self.audioManager.Disable()

    def getAwakeEmitterNames(self):
        return [emitter.name for emitter in self.audioManager.GetAwakeEmitters()]

    def test_awake_emitters_contains_woken_emitter(self):
        """A woken emitter shows up in GetAwakeEmitters and no returned emitter is culled."""
        awakeEmitters = self.audioManager.GetAwakeEmitters()
        self.assertIn("awakeEmitter1", [emitter.name for emitter in awakeEmitters])
        for emitter in awakeEmitters:
            self.assertFalse(emitter.IsCulled())

    def test_awake_emitters_is_subset_of_prioritized_emitters(self):
        """Every awake emitter is also in the full prioritized list."""
        prioritizedNames = [emitter.name for emitter in self.audioManager.GetPrioritizedEmitters()]
        for name in self.getAwakeEmitterNames():
            self.assertIn(name, prioritizedNames)

    def test_forced_culled_emitter_not_in_awake_emitters(self):
        """An emitter forced into the culled state is excluded from GetAwakeEmitters."""
        self.emitter.ForceCullingStateChange()
        self.assertTrue(self.emitter.IsCulled())
        self.assertNotIn("awakeEmitter1", self.getAwakeEmitterNames())

        # Release the forced state and make sure it comes back.
        self.emitter.ForceCullingStateChange()
        self.assertTrue(WaitForEmitterToWake(self.emitter), "Timed out waiting for the emitter to be woken up again.")
        self.assertIn("awakeEmitter1", self.getAwakeEmitterNames())

    def test_awake_emitters_respects_max_awake_game_objects(self):
        """With more emitters than maxAwakeGameObjects, GetAwakeEmitters only returns the awake subset."""
        import audio2
        maxAwake = 2
        self.audioManager.manager.maxAwakeGameObjects = maxAwake

        extraEmitterNames = ["extraEmitter%d" % index for index in range(5)]
        # Keep references so the emitters stay registered for the duration of the test.
        extraEmitters = []
        for name in extraEmitterNames:
            extraEmitter = audio2.AudEmitter(name)
            extraEmitter.SetPlacement((0,0,0), (0,0,0), (0,0,0))
            extraEmitters.append(extraEmitter)

        allTestEmitterNames = set(extraEmitterNames) | {"awakeEmitter1"}

        # Pump a few frames so CullAudio runs with the new maxAwakeGameObjects value.
        PumpOSWithTimeout(self.alwaysTrueBoolean, maxTries=3)

        awakeTestEmitters = [name for name in self.getAwakeEmitterNames() if name in allTestEmitterNames]
        culledTestEmitters = allTestEmitterNames - set(awakeTestEmitters)
        # The culling loop keeps maxAwakeGameObjects + 1 objects awake and the listener takes one of those slots.
        self.assertLessEqual(len(awakeTestEmitters), maxAwake + 1)
        self.assertGreater(len(culledTestEmitters), 0, "Expected sound prioritization to cull at least one test emitter.")

    def test_awake_emitters_match_prioritized_when_culling_disabled(self):
        """With culling disabled every game emitter is awake, so the two lists have the same members (minus built-in system objects)."""
        systemObjectNames = {self.listener.name, "UI", "Music"}
        self.audioManager.DisableSoundPrioritization()
        try:
            awakeNames = sorted(self.getAwakeEmitterNames())
            prioritizedNames = sorted(emitter.name for emitter in self.audioManager.GetPrioritizedEmitters()
                                      if emitter.name not in systemObjectNames)
            self.assertEqual(awakeNames, prioritizedNames)
        finally:
            self.audioManager.EnableSoundPrioritization()
