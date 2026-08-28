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
        self.emitter.SetPlacement((1,0,0), (0,1,0), (0,50,0))

    def assertVectorAlmostEqual(self, actual, expected):
        self.assertAlmostEqual(actual[0], expected[0], places=4)
        self.assertAlmostEqual(actual[1], expected[1], places=4)
        self.assertAlmostEqual(actual[2], expected[2], places=4)

    def test_setplacement_updates_orientation_and_position(self):
        """Blue placement exposure accepts front, top, and position."""
        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 50, 0))
        self.assertVectorAlmostEqual(self.emitter.front, (1, 0, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 1, 0))

    def test_front_and_top_readback_match_corrected_wwise_orientation(self):
        self.emitter.SetPlacement((2, 0, 0), (1, 1, 0), (0, 50, 0))

        self.assertVectorAlmostEqual(self.emitter.front, (1, 0, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 1, 0))

    def test_setposition_is_not_exposed(self):
        """AudEmitter does not expose transform-style SetPosition in Blue."""
        with self.assertRaises(AttributeError):
            self.emitter.SetPosition((0, 60, 0))

    def test_translation_is_not_exposed(self):
        """AudEmitter has authored rotation, but no standalone transform position."""
        self.assertFalse(hasattr(self.emitter, "translation"))

    def test_rotation_adds_to_incoming_placement_direction_and_survives_wake(self):
        """Upstream placement supplies the parent orientation and emitter rotation is additive."""
        self.emitter.rotation = (0, 1, 0, 0)

        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 50, 0))
        self.assertVectorAlmostEqual(self.emitter.front, (-1, 0, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 1, 0))

        self.emitter.Cull()
        self.assertTrue(self.emitter.IsCulled())
        self.emitter.Wake()
        self.assertFalse(self.emitter.IsCulled())

        self.assertVectorAlmostEqual(self.emitter.front, (-1, 0, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 1, 0))

    def test_rotation_attribute_updates_existing_placement(self):
        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 50, 0))

        self.emitter.rotation = (0, 1, 0, 0)

        self.assertVectorAlmostEqual(self.emitter.front, (-1, 0, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 1, 0))

    def test_rotation_attribute_updates_default_orientation_before_placement(self):
        self.emitter.rotation = (0.7071068, 0, 0, 0.7071067)

        self.assertVectorAlmostEqual(self.emitter.front, (0, -1, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 0, 1))

    def test_rotation_attribute_adds_to_stretchaudio_placement_convention(self):
        self.emitter.rotation = (0.7071068, 0, 0, 0.7071067)

        self.emitter.SetPlacement((0, 1, 0), (0, 0, 1), (0, 50, 0))

        self.assertVectorAlmostEqual(self.emitter.front, (0, 0, 1))
        self.assertVectorAlmostEqual(self.emitter.top, (0, -1, 0))

    def test_repeated_parent_placement_updates_do_not_compound_rotation(self):
        self.emitter.rotation = (0, 1, 0, 0)

        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 50, 0))
        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 50, 0))

        self.assertVectorAlmostEqual(self.emitter.front, (-1, 0, 0))
        self.assertVectorAlmostEqual(self.emitter.top, (0, 1, 0))

    def test_listener_does_not_expose_emitter_orientation_readback(self):
        import audio2
        listener = audio2.GetListener()

        self.assertFalse(hasattr(listener, "front"))
        self.assertFalse(hasattr(listener, "top"))

    def test_emitter_has_no_usable_world_position_before_placement(self):
        """A fresh emitter still sits at the spawn sentinel, which is not a usable world position."""
        self.assertFalse(self.emitter.HasUsableWorldPosition())

    def test_emitter_has_usable_world_position_after_placement(self):
        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 50, 0))

        self.assertTrue(self.emitter.HasUsableWorldPosition())
        self.assertVectorAlmostEqual(self.emitter.position, (0, 50, 0))

    def test_emitter_at_origin_has_usable_world_position(self):
        """(0, 0, 0) is a legitimate world position, not a sentinel."""
        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, 0, 0))

        self.assertTrue(self.emitter.HasUsableWorldPosition())

    def test_emitter_with_non_finite_position_has_no_usable_world_position(self):
        self.emitter.SetPlacement((1, 0, 0), (0, 1, 0), (0, float("nan"), 0))

        self.assertFalse(self.emitter.HasUsableWorldPosition())

    def test_listener_has_usable_world_position_after_placement(self):
        import audio2
        listener = audio2.GetListener()
        listener.SetPosition((1, 0, 0), (0, 1, 0), (10, 20, 30))

        self.assertTrue(listener.HasUsableWorldPosition())
        self.assertVectorAlmostEqual(listener.position, (10, 20, 30))

    def test_ui_and_music_players_never_have_usable_world_position(self):
        """UI and music game objects get a position for Wwise's sake but are not placed in the world,
        so world-space systems such as line-of-sight must ignore them."""
        import audio2
        uiPlayer = audio2.GetUIPlayer()
        musicPlayer = audio2.GetMusicPlayer()
        uiPlayer.SetPlacement((1, 0, 0), (0, 1, 0), (0, 0, 0))
        musicPlayer.SetPlacement((1, 0, 0), (0, 1, 0), (0, 0, 0))

        self.assertFalse(uiPlayer.HasUsableWorldPosition())
        self.assertFalse(musicPlayer.HasUsableWorldPosition())
