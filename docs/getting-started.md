# Getting started

Carbon Audio is initialized and enabled from Python. You can either use the `AudioManager` class in [`python/audio2/audiomanager.py`](../python/audio2/audiomanager.py), which handles setup and SoundBank state for you, or call the exposed Carbon Audio APIs directly.

## Before you start

You need:

1. SoundBanks generated from a Wwise project.
2. An audio metadata JSON file describing the events in that project, in the format described in [Sound prioritization](sound-prioritization.md#audio-metadata).
3. Both of the above in a folder registered as a Blue path, such as `res:/` or `soundbanks:/`. The prefix before `:/` can be any name you choose.

For an example of registering a Blue path, see `setUpClass` in [`tests/python/audiotests/base_test_class.py`](../tests/python/audiotests/base_test_class.py).

## Using AudioManager

This example initializes and enables Carbon Audio from within an EVE or EVE Frontier branch:

```python
import json
import scheduler

import blue
from audio2.audiomanager import AudioManager

def EnableCarbonAudio():
    audio_metadata_filepath = ...  # path to your audio metadata JSON file
    application_name = "CarbonAudio Test"
    base_sound_bank_path = "soundbanks:/"  # the Blue path your SoundBanks are registered under
    language_directory = "English(US)"

    audio_manager = AudioManager(base_sound_bank_path, language_directory, application_name)

    with open(audio_metadata_filepath, "r") as f:
        audio_metadata = json.loads(f.read())

    audio_manager.Initialize(audio_metadata)  # defaultSoundBanks adds SoundBanks that are always loaded
    audio_manager.Enable()  # only Init.bnk is loaded; soundBanksToLoad loads others at startup


if __name__ == "__main__":
    t = scheduler.tasklet(EnableCarbonAudio)()
    while t.alive:
        blue.os.Pump()
```

### Init.bnk

Wwise expects `Init.bnk` to be loaded before any other SoundBank. Carbon Audio loads it when it is enabled, so you never load it yourself.

### Default SoundBanks

A default SoundBank is never unloaded at runtime, even if `UnloadSoundBank` is called on it. Use it for SoundBanks that must always be available, such as one holding events used throughout the game.

Add default SoundBanks when initializing:

```python
audio_manager.Initialize(audio_metadata, defaultSoundBanks=["Common.bnk", "Music.bnk"])
```

or later with `AddAndLoadDefaultSoundBank`. Remove one with `RemoveAndUnloadDefaultSoundBank`.

### SoundBank state while disabled

`AudioManager` keeps tracking `LoadSoundBank` and `UnloadSoundBank` calls while disabled, so that when it is enabled again it loads the SoundBanks the game currently needs.

## Without AudioManager

`AudioManager` is the reference for driving Carbon Audio directly. The steps it follows are:

1. Get the Carbon Audio manager.
2. Create an `AudSettings` with the base SoundBank path, application name and language directory, and pass it to the manager.
3. Get the `AudStaticDataRepository` and initialize it with the audio metadata.
4. Enable the manager. This also loads `Init.bnk`.
5. Create a listener so that rendered sounds can be heard.
