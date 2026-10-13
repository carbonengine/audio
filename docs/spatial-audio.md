# Spatial audio

Carbon Audio gives access to Wwise's 3D audio support, which it calls spatial audio.

## Enabling

Spatial audio is enabled by default when:

- the Wwise project has a system audio device named `System`,
- that device has **Allow 3D audio** enabled, and
- the user has a spatial audio endpoint active, such as Dolby Atmos or Windows Sonic.

Use the `spatialAudioDeviceName` argument of `AudioManager` to use a different device name.

## Disabling

To force stereo output, call `DisableSpatialAudio()` or pass `spatialAudioEnabled=False` to `AudioManager`. This needs a `System_Stereo` audio device with **Allow 3D audio** disabled. Use the `stereoAudioDeviceName` argument to use a different device name.

The test project in [`Wwise/CarbonAudioTest`](../Wwise/CarbonAudioTest) shows how both devices are named and configured.
