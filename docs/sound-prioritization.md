# Sound prioritization

Sound prioritization decides which emitters are awake and which are put to sleep, so that the most relevant sounds keep playing when many are active at once. The blog post [Sound prioritization](https://www.eveonline.com/news/view/sound-prioritization) introduces the system.

It is turned on with `AudioManager.EnableSoundPrioritization()` after `Initialize`, and relies on metadata about the Wwise events the game uses.

## Audio metadata

The metadata is a Python dictionary passed to `AudioManager.Initialize`, which stores it in `AudStaticDataRepository`. It is usually written to a JSON file by a script that uses the [Wwise Authoring API](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=waapi.html) and loaded at startup. Internally, the file is generated with the `generate-sp-metadata` command from audio-scripts.

The file has three top-level sections:

| Section | Keyed by | Used for |
|---|---|---|
| `Events` | Wwise event name | Sound prioritization |
| `SoundBanks` | SoundBank file name | Essential content |
| `WemFileIDs` | Media source ID | Essential content |

### Events

| Field | Type | Description |
|---|---|---|
| `eventID` | integer | The Wwise short ID of the event. |
| `maxRadiusAttenuation` | float | The largest attenuation radius of any sound the event plays. Write it as a float, for example `100.0`. |
| `isLoop` | bool | Whether the event is a loop rather than a one-shot. |
| `is2D` | bool | Whether any sound the event plays is 2D. |
| `isVital` | bool | Whether any sound the event plays must be prioritized as highly as possible. |
| `eventsStoppedBy` | list of strings | Events that stop the sounds this event plays. |
| `soundbanks` | list of strings | SoundBanks that contain the event. |

The generator also writes `wwiseID` (the object GUID) and `playbackDuration` (see [`ak.wwise.core.object.get`](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=ak_wwise_core_object_get.html)). Carbon Audio does not read them.

### Example

```json
{
  "Events": {
    "Play_TestLoop": {
      "eventID": 1483003980,
      "maxRadiusAttenuation": 100.0,
      "isLoop": true,
      "is2D": false,
      "isVital": false,
      "eventsStoppedBy": [],
      "soundbanks": ["TestLoop.bnk"]
    }
  },
  "SoundBanks": {
    "TestLoop.bnk": {
      "EssentialSoundBank": true
    }
  },
  "WemFileIDs": {
    "460136326": {
      "IsEssential": false
    }
  }
}
```

A complete file generated from the test project is in [`tests/python/audiotests/test/soundbanks/SoundPrioritizationMetadata.json`](../tests/python/audiotests/test/soundbanks/SoundPrioritizationMetadata.json).

## Essential content

The essential folder holds a compact set of SoundBanks and media for game builds with limited content. When resolving a file, Carbon Audio's streaming IO checks the metadata and loads essential SoundBanks and media from that folder.

Two Wwise custom properties on SoundBanks mark what is essential:

- `EssentialSoundBank`: the SoundBank file itself is essential. Written to `SoundBanks` in the metadata.
- `EssentialMedia`: the media the SoundBank uses is essential. Written per media file to `WemFileIDs` as `IsEssential`.

To use them in your Wwise project, copy [`ccp.wcustomproperties`](../Wwise/CarbonAudioTest/Add-ons/Properties/ccp.wcustomproperties) into the project's `Add-ons/Properties` folder. See the Wwise documentation on [custom properties](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=defining_custom_properties.html) for details.
