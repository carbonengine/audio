# CarbonAudio
CarbonAudio (previously audio2) is a wrapper through which EVE Online communicates with Wwise. In addition to interacting with Wwise, CarbonAudio houses its own sound prioritization system that helps protect Wwise from being hammered too hard in larger scenes.

CarbonAudio depends on [Blue](https://github.com/ccpgames/carbon-blue) which is the layer that exposes it to Python. In addition to this there are header only includes that can be used to interact with CarbonAudio through C++.

## Development Setup

### Prerequisites
- **Wwise C++ SDK**: Internally provided through the private registry. External contributors should provide their own personal license (free for personal use). 
- **Windows**: Visual Studio 2026 with the v145 toolset.
- **macOS**: Xcode command line tools.
- **CMake** 3.16 or newer.
- **Git** with submodule support.

### Quick Setup
1. Clone the repository.
2. Initialize submodules:
   ```
   git submodule update --init --recursive
   ```
3. Configure with the preset for your target.
   ```
   cmake --preset x64-windows-internal
   ```

4. Build and test:
   ```
   cmake --build --preset x64-windows-internal --config Internal
   ctest --preset x64-windows-internal -C Internal
   ```

### Installing into an EVE / Frontier branch

```
cmake --preset x64-windows-internal -DINSTALL_TO_MONOLITH=ON -DCMAKE_INSTALL_PREFIX=<branch-root>/vendor/github.com/ccpgames/carbon-audio/develop
cmake --build --preset x64-windows-internal --config Internal --target install
```

## Testing

### Running Tests
CarbonAudio tests are located in `tests/python/audiotests` and can be run through:

1. **VS Code Test Explorer**: When CMake configuration succesfully runs you should be able to see all tests in the "Testing" section of VS Code and run them individually or all at once. 
2. **Command Line**: Use `ctest` from your build directory

### Debug Testing
For debugging C++ code during test execution, you can enable a debug delay:

#### Enable Debug Delay
1. **Run the task**: `Ctrl+Shift+P` → `Tasks: Run Task` → "Enable Debug Delay for Tests"
2. **Run your test**: The test will pause for 7 seconds before execution
3. **Attach debugger**: Use `Ctrl+Shift+P` → `Debug: Select and Start Debugging` → "Attach to Process"
4. **Select process**: Choose `exefile.exe` from the process list

#### Disable Debug Delay
Run the task: `Ctrl+Shift+P` → `Tasks: Run Task` → "Disable Debug Delay for Tests"

### Debugging Configuration
The repository includes pre-configured launch configurations in `.vscode/launch.json`:

- **Attach to Process**: Attach to any running process using Visual Studio debugger
- **Attach to Process (GDB)**: Alternative using GDB debugger

### Wwise Test Project
This repository includes a test Wwise project in the `Wwise/` directory:

1. **Install Wwise**: Download [Audiokinetic Launcher](https://www.audiokinetic.com/en/library/wwise_launcher/) and install Wwise version 2025.1.5.9095
2. **Open Project**: Launch Wwise and open `Wwise/CarbonAudioTest/CarbonAudioTest.wproj`
3. **Generate SoundBanks**: The project outputs to `tests/python/audiotests/test/soundbanks/`

#### Generating SoundBanks from the Wwise CarbonAudio Test Project
If you are updating tests to use new events or soundbanks defined in the test Wwise project, you have to manually generate those soundbanks as well
as generating `SoundPrioritizationMetadata.json` (an example can be found at [tests/python/soundbanks/SoundPrioritizationMetadata.json]())

To generate the soundbanks, go to the soundbanks view in the Wwise authoring program and simply generate soundbanks. The project is set up to automatically 
deposit it into `tests/python/audiotests/test/soundbanks`.

To generate the `SoundPrioritizationMetadata.json` file, you must use the [audio-scripts](https://github.com/ccpgames/audio-scripts/tree/main/standalone) command line tool. Refer to the auido-scripts standalone documentation to learn how to install and use the `generate-sp-metadata` command.

## Spatial Audio (3D Audio)
CarbonAudio provides access to Wwise's 3D audio capabilities, which CarbonAudio refers to as "spatial audio." 

### Enabling Spatial Audio
Spatial audio is enabled by default when:
- Your Wwise project has a system audio device named "System" 
- The device has "Allow 3D audio" enabled
- The user has a spatial audio endpoint active (Dolby Atmos, Windows Sonic, etc.)

You can customize device names using the `spatialAudioDeviceName` parameter in the AudioManager.

### Disabling Spatial Audio
To force stereo output use the `DisableSpatialAudio()` method or pass in `spatialAudioEnabled=False` during initialization. Requires a "System_Stereo" audio device with "Allow 3D audio" disabled. Use `stereoAudioDeviceName` parameter to customize the device name. 

*Refer to the test Wwise project found in `Wwise/CarbonAudioTest` to see how audio devices should be named and configured* 

## Initializing and Enabling CarbonAudio
CarbonAudio must be initialized and enabled using APIs exposed to Python. There are two ways to do this: 
1. By using the `AudioManager` class found in `python/audio2/audiomanager.py` which simplifies the initialization process for you and also exposes
   functionality to help manage SoundBanks better.
2. By directly calling the exposed CarbonAudio APIs directly.

### Prerequisites
In order to run CarbonAudio you must:
1. Have generated soundbanks from a Wwise project.
2. Have generated a JSON file containing metatadata about the events in your Wwise project. This is necessary for sound prioritization. 
   This JSON file must follow the spec found in the "Audio Metatada JSON Spec" section.
3. Placed those soundbanks and JSON file in a location that has been registered as a Blue path and can be found by Blue.

An example of a Blue path is `res:/` or `soundbanks:/` or really whatever word you want to put before the `:/`. 

You can find an example of registering a blue path in the `setUpClass` method in `tests/python/audiotests/base_test_class.py`.

### Using the AudioManager Python class
Here is an example of using the AudioManager Python class from within an EVE or Frontier branch to both initialize and enable CarbonAudio:
```
import json
import scheduler

import blue
from audio2.audiomanager import AudioManager

def EnableCarbonAudio():
    audio_metadata_filepath = ... # put the filepath where your audio metadata JSON file is located. 
	application_name = "CarbonAudio Test"
	base_sound_bank_path = "soundbanks:/" # This is assuming you followed the prerequisites and made your soundbanks discoverable using a Blue path.
	language_directory = "English(US)"

	audio_manager = AudioManager(baseSoundbankPath, languageDirectory, applicationName)

    with open(audio_metadata_filepath, "r") as f:
	    audio_metadata = json.loads(f.read())

    audio_manager.Initialize(audio_metadata) # Use the kwarg defaultSoundBanks if you want to add specific soundbanks as defaults.
    audio_manager.Enable() # The only SoundBank that will be loaded is Init.bnk. Use the kwarg soundBanksToLoad if you want others to load at start up.


if __name__ == "__main__":
    t = scheduler.tasklet(EnableCarbonAudio)()
    while t.alive:
        blue.os.Pump()
```

### Default SoundBanks and Init.bnk
CarbonAudio will implicitly load the Init.bnk before anything else without you needing to tell it to. The loading of Init.bnk
is done when you enable CarbonAudio.

The python `AudioManager` class also has a concept of "default" SoundBanks. A default SoundBank is one that will never 
be unloaded during runtime, even if you call `UnloadSoundBank` on a SoundBank considered default. This is useful
for those SoundBanks that should always be loaded, such as a SoundBank that contains all the events in the game.

#### Adding Default SoundBanks
There are two ways to add a default SoundBank.

The first is when initializing the the `AudioManager`. To do this, use the keyword argument `defaultSoundBanks` with a list of SoundBank names:
```
audio_manager.Initialize(defaultSoundsBanks=["Common.bnk", "Music.bnk", ...] 
```

The second way is to use the `AddAndLoadDefaultSoundBank` method.

#### Removing Default SoundBanks
To remove a default soundbank from the `AudioManager` use the method `RemoveAndUnloadDefaultSoundBank`.

#### A Note about SoundBank State and Enabling/Disabling
When you disable the `AudioManager` it will still keep the state of SoundBanks when calling `LoadSoundBank` and `UnloadSoundBank` so that, when enabled again, 
it will can load SoundBanks for the current state of the game. 

### Initializing CarbonAudio Without the AudioManager
If you want to work with CarbonAudio without leveraging the `AudioManager` python class found in `python/audio2/audiomanager.py`, you can look at said `AudioManager` 
class to see how it does it. However, here is a short rundown of how you would go about it.

The steps needed to initialize and enable CarbonAudio, so that a player is ready to hear sounds, are as follows:
* Get an instance of CarbonAudio's C++ audio manager.
* Create an AudSettings instance with the default SoundBank path, application name and language directory.
* Get an instance of AudStaticDataRepository from CarbonAudio.
* Pass in the AudSettings instance to CarbonAudio's audio manager.
* Initialize the static data repository with audio event metadata in the form of that described in the "Audio Metadata JSON Spec" section of this Readme.
* Enable CarbonAudio's audio manager. 
* Create a listener that can be used to actually "hear" any rendered sounds.

For more detailed programming examples showing how to do that above look at the python `AudioManager`.

#### The Init.bnk
When using CarbonAudio's audio manager directly, you will never have to tell it to load `Init.bnk`. This is a SoundBank that 
Wwise expects to always be loaded first and CarbonAudio's audio manager will do this implicitly when it is enabled.

## Sound Prioritization
For an introduction to what sound prioritization is you can look at the blog found [here](https://www.eveonline.com/news/view/sound-prioritization)

For more in depth info you can check out [this wiki page](https://ccpgames.atlassian.net/wiki/spaces/CAudio/pages/180684261/Sound+Prioritization+System)

### Audio Metadata JSON Spec
The backbone of the sound prioritization system comes from feeding it metadata about Wwise events that are used by the game.
This info can be gathered using the [Wwise Authoring API](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=waapi.html).

This metadata must come in the form of a Python dictionary that is given to CarbonAudio when initializing it. It is easiest to output it 
as a JSON file from a script that uses the Wwise Authoring API and then loading that JSON file in Python.

The following is a JSON specification detailing what CarbonAudio expects:

```
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "description": "Audio metadata necessary for the sound prioritization system to function. Most of these properties come from the Wwise Authoring API: https://www.audiokinetic.com/en/library/edge/?source=SDK&id=waapi_index.html"
  "patternProperties": {
    "^[a-zA-Z0-9_]+$": {
      "description": "The Wwise event you are describing. Example: ship_boosters_play",
      "type": "object",
      "properties": {
        "eventID": { 
          "type": "long",
          "description": "The eventID of the given Wwise event. If it is not cast to long it will not work! You cannot use an int!"
        },
        "maxRadiusAttenuation": { 
          "type": "number",
          "description": "The largest attenuation of any one sound played by this event."
        },
        "isLoop": { 
          "type": "boolean",
          "description": "Whether this event is a loop or a one shot."
        },
        "eventsStoppedBy": { 
          "type": "array", 
          "items": {} 
          "description": "An array of Wwise events that stop sounds played by this event."
        },
        "isVital": { 
          "type": "boolean",
          "description": "Whether any of the sounds played by this event is considered vital and should be prioritized as highly as possible."
        },
        "playbackDuration": {
          "type": "object",
          "description": "The playback duration of this event. See this Waapi documentation: https://www.audiokinetic.com/en/library/edge/?source=SDK&id=ak_wwise_core_object_get.html".
          "properties": {
            "playbackDurationMin": { "type": "number" },
            "playbackDurationMax": { "type": "number" },
            "playbackDurationType": { 
              "type": "string",
              "enum": ["oneShot", "infinite", "unknown"],
            }
          },
          "required": ["playbackDurationMin", "playbackDurationMax", "playbackDurationType"]
        },
        "is2D": { 
          "type": "boolean",
          "description": "Whether any sound played by this event is 2D."
        },
        "wwiseID": { 
          "type": "string",
          "description": "A GUID determined by Wwise in the form of {aabbcc00-1122-3344-5566-77889900aabb}."
        }
      },
      "required": ["eventID", "maxRadiusAttenuation", "isLoop", "eventsStoppedBy", "isVital", "playbackDuration", "is2D", "wwiseID"],
      "additionalProperties": false
    },
  },
  "additionalProperties": false
}
```

Here is an example AudioMetadata.json with two events, a one shot and a loop:
```
{
  "Play_TestOneShot": {
    "eventID": 1669951674,
    "maxRadiusAttenuation": 100.0,
    "isLoop": false,
    "eventsStoppedBy": [],
    "isVital": false,
    "playbackDuration": {
      "playbackDurationMin": 7.440604209899902,
      "playbackDurationMax": 7.440604209899902,
      "playbackDurationType": "oneShot"
    },
    "is2D": false,
    "wwiseID": "{A6FA0F98-6EAB-49CA-9417-79A6B8C2AAF3}"
  },
  "Play_TestLoop": {
    "eventID": 1483003980,
    "maxRadiusAttenuation": 100.0,
    "isLoop": true,
    "eventsStoppedBy": [],
    "isVital": false,
    "playbackDuration": {
      "playbackDurationMin": 3.4028234663852886e+38,
      "playbackDurationMax": 3.4028234663852886e+38,
      "playbackDurationType": "infinite"
    },
    "is2D": false,
    "wwiseID": "{7FE9B34E-E023-4F28-8541-1FEC4AEEFB68}"
  }
}
```

This JSON was generated using the `generate_audio_metadata.py` file from the [audio-scripts monolith repo](https://github.com/ccpgames/audio-scripts/tree/main/monolith). Follow the documentation there to run it.

### Audio Metadata Custom Properties

Wwise's Custom properties allow you to store additional information in Wwise objects. In Carbon-Audio, custom properties such as `EssentialSoundBank` and `EssentialMedia` are used to indicate if a soundbank or a source file (`.wem` files) should be included in the essential music directory. The essential directory is a precache folder containing vital audio files necessary for a game version with limited content.

These custom properties are defined in the EVE Wwise project and are also reflected in the `AudioMetadata.json` file. Here is an example of how it looks:

```
    "WemFileIDs": {
        "460136326": {
            "IsEssential": false
        }
    },
    "SoundBanks": {
        "TestLoop.bnk": {
            "id": "{5B6959F0-EE85-40E6-822B-90DC0157584F}",
            "path": "\\SoundBanks\\Default Work Unit\\TestLoop",
            "name": "TestLoop",
            "shortId": 3918824947,
            "parent": {
                "id": "{4B764402-C2B1-488E-A768-588CB657A0CD}",
                "name": "Default Work Unit"
            },
            "EssentialMedia": false,
            "EssentialSoundBank": true
        },
      }
```
        
In `AudStaticDataRepository`, all the above audio metadata is stored. Then, in the low-level file handling logic of Carbon Audio, it decides whether to pull audio assets from this "Essentials" folder.

To use CarbonAudio with your Wwise project, copy the [`ccp.wcustomproperties`](Wwise/CarbonAudioTest/Add-ons/Properties/ccp.wcustomproperties) file to your project's `Add-ons/Properties` folder. This defines custom properties required by CarbonAudio. For more information on custom properties in Wwise, refer to the [Wwise documentation](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=defining_custom_properties.html).
