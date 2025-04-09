# CarbonAudio
CarbonAudio (previously audio2) is a wrapper through which EVE Online communicates with Wwise. In addition to interacting with Wwise, CarbonAudio houses its
own sound prioritization system that helps protect Wwise from being hammered too hard in larger scenes.

CarbonAudio depends on [Blue](https://github.com/ccpgames/carbon-blue) 
which is the layer that exposes it to Python. In addition to this there are header only includes that can be used to interact with CarbonAudio through C++.

## Building CarbonAudio locally
To build CarbonAudio locally, you must first set the environment variable `CCP_EVE_PERFORCE_BRANCH_PATH` to point to the the root of the EVE or Perforce
branch you are developing against. Then, open the project up in Visual Studio, choose one of the build flavors defined in `CMakeSettings.json` and choose `Build`. 
The most commonly used flavor of CarbonAudio during development is the Internal one.

## Requirements for your Wwise project.
### Requirements for Spatial Audio
In order to be able to use what Carbon Audio calls Spatial Audio (and what Wwise calls "3D audio") your Wwise project must have the following:
* A system audio device named "System" which has the "Allow 3D Audio" enabled.
* A system audio device named "System_Stereo" that __does not__ have "Allow 3D Audio" enabled.

If you do not have an audio device correctly configured to "Allow 3D audio" then you will not be able to use Spatial Audio when the user has 
a spatial audio endpoint active (e.g. Dolby Atmos, Windows Sonic for Headphones, Apple Spatial Audio, etc.). In addition, if you do not have the "System_Stereo" 
audio device it will not be possible to turn off Spatial Audio when the user has a spatial endpoint active.

If you want to use different names for your 3D and stereo audio devices, refer to the next section for more details on how to do so.

## Spatial Audio (sometimes known as 3D audio)
Carbon Audio exposes some functionality directly tied to Wwise's 3D audio features. What Wwise calls 3D audio Carbon Audio refers to as "Spatial Audio" as that is the more 
widely used industry term. 

### Enabling Spatial Audio
Spatial audio is enabled by default as long as your Wwise project has a system audio device with "Allow 3D audio" enabled and this device has the same name as that defined in 
the C++ class `AudSettings.spatialAudioDeviceName`. The default name for this system audio device is `System`. So, if your project has a system audio device named "System" and 
it has "Allow 3D audio enabled" then you should not need to do any further work enable spatial audio when you initialize Carbon Audio.

If you want to change the name of the spatial audio device you can pass in the keyword argument `spatialAudioDeviceName` when initializing the Python audio manager (refer to `python/audio2/audiomanager.py`).

If you want to manually enable spatial audio after initializion you can use the `EnableSpatialAudio()` method on the Python audio manager.

In order for a user to hear spatialized audio they will need to have a spatial audio endpoint activated on their Windows system such as Dolby Atmos or Windows Sonic for Headphones.
If they do not have any of these active then the user will continue to hear stereo even if Carbon Audio is told to enable spatial audio.

### Disabling Spatial Audio
You can force a change to stereo, whether the user has a spatial audio endpoint active or not, by using the `DisableSpatialAudio()` method in the Python audio manager or 
by passing in the keyword argument `spatialAudioEnabled=False` when initializing the Python audio manager (refer to `python/audio2/audiomanager.py`).

This function only works if you have a valid system audio device in your Wwise project with the name "System_Stereo" that has "Allow 3D audio" disabled.

If you want to change the name of the stereo audio device you can pass in the keyword argument `stereoAudioDeviceName` when initializing the Python audio manager. 

## Testing
CarbonAudio is currently tested through the python package located at `tests/python/audiotests`.

### Running CarbonAudio tests
You can run CarbonAudio tests using ctest. In Visual Studio you can do this by first building and installing the project and then choosing `Test->Run CTests for CarbonAudio`
from the top toolbar. This will copy all the binaries needed to the installation directory and run all Python tests through EVE's Exefile.exe binary.

### Updating the Wwise Test Project
This repository includes a Wwise project specifically meant to be used for testing CarbonAudio found in the `Wwise` directory. This project
acts as a helpful way to test specific Wwise capabilities in CarbonAudio. 

The Wwise project outputs its SoundBanks into the directory `tests/python/audiotests/test/soundbanks` which is the same directory the CarbonAudio tests look
for SoundBanks in. In addition to this, the tests depend on a file named `AudioMetadata.json` which provides metadata that is required for CarbonAudio's sound prioritization
system to function.

#### Opening the Wwise CarbonAudio Test Project
To open the test Wwise project, you must install the [Audiokinetic Launcher](https://www.audiokinetic.com/en/library/wwise_launcher/?source=InstallGuide&id=the_wwise_launcher),
then install the Wwise authoring application for Wwise version 2021.1.6.7774.

Once done you can launch the Wwise authoring application and point it to `Wwise/CarbonAudioTest/CarbonAudioTest.wproj`.

#### Generating SoundBanks from the Wwise CarbonAudio Test Project
If you are updating tests to use new events or soundbanks defined in the test Wwise project, you have to manually generate those soundbanks as well
as generating `AudioMetadata.json`.

To generate the soundbanks, go to the soundbanks view in the Wwise authoring program and simply generate soundbanks. The project is set up to automatically 
deposit it into `python/audio2/test/soundbanks`.

To generate the `AudioMetadata.json` file, you must use a script found in either an EVE or Frontier branch. This script is located at 
`<BRANCH-ROOT>/carbon/tools/audiotools/scripts/generate_metadata_for_fsd.py` where `<BRANCH-ROOT>` is the root of the EVE or Frontier branch you are using.
You must run this script using `<BRANCH-ROOT>/eve/client/pythonInterpreter.bat`. When running this script you can point it to the test project and tell it to 
output in any directory using the `--wwise-project-path` and `--output-directory` flags. 

The following is an example of running this script from a PowerShell terminal on my machine from the root of an EVE branch:
```
.\eve\client\pythonInterpreter.bat .\carbon\tools\audiotools\scripts\generate_metadata_for_fsd.py --wwise_project_path="C:\Users\eric\code\carbon-audio\Wwise\CarbonAudioTest\CarbonAudioTest.wproj" --output-directory="C:\Users\eric\code\carbon-audio\python\audio2\test\soundbanks"
```

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

You can find an example of registering a blue path in the `setUpClass` method in `python/audio2/test/base_test_class.py`.

### Using the AudioManager Python class
Here is an example of using the AudioManager Python class from within an EVE or Frontier branch to both initialize and enable CarbonAudio:
```
import json
import uthread2

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


if "__init__" == "__main__":
    t = uthread2.StartTasklet(EnableCarbonAudio)
    while t.is_alive():
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

This JSON was generated using the `generate_metadata_for_fsd.py` script found in an EVE or Frontier branch at `carbon/tools/audiotools/scripts/generate_metadata_for_fsd.py`.

### Audio Metadata Custom Properties

Wwise's Custom properties allow you to store additional information in Wwise objects. In Carbon-Audio, custom properties such as `EssentialSoundBank` and `EssentialMedia` are used to indicate if a soundbank or a source file (`.wem` files) should be included in the essential music directory. The essential directory is a precache folder containing vital audio files necessary for a game version with limited content.

These custom properties are defined in the EVE Wwise project and are also reflected in the `SoundPrioritizationMetadata` JSON file. Here is an example of how it looks:

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
In `AudStaticDataRepository`, all the above audio metadata is stored. Then, in the low-level file handling logic of Carbon Audio, it decides whether to pull audio assets from this "Essentials" folder.

To use carbon-audio with a Wwise project, you need to add a `.wcustomproperties` file to your project. This file should be named `ccp.wcustomproperties` and placed in the `Add-Ons\Properties` folder. For more information on defining custom properties in Wwise, refer to the [Wwise documentation](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=defining_custom_properties.html).

Here are the exact contents needed in this file:

```
<?xml version="1.0" encoding="UTF-8"?>
<PluginModule>
	<WwiseObject Name="SoundBank" CompanyID="1" PluginID="18">
		<Properties>
			<Property Name="CCP:EssentialSoundBank" DocId="CCP:EssentialSoundBank" DisplayName="EssentialSoundBank" Type="bool">
				<DefaultValue>True</DefaultValue>
				<AudioEnginePropertyID>1</AudioEnginePropertyID>
				<Restrictions>
					<ValueRestriction>
						<Range Type="bool">
							<Min>False</Min>
							<Max>True</Max>
						</Range>
					</ValueRestriction>
				</Restrictions>
			</Property>
		</Properties>
	</WwiseObject>
    <WwiseObject Name="SoundBank" CompanyID="1" PluginID="18">
		<Properties>
			<Property Name="CCP:EssentialMedia" DocId="CCP:EssentialMedia" DisplayName="EssentialMedia" Type="bool">
				<DefaultValue>False</DefaultValue>
				<AudioEnginePropertyID>1</AudioEnginePropertyID>
				<Restrictions>
					<ValueRestriction>
						<Range Type="bool">
							<Min>False</Min>
							<Max>True</Max>
						</Range>
					</ValueRestriction>
				</Restrictions>
			</Property>
		</Properties>
    </WwiseObject>
</PluginModule>
```

