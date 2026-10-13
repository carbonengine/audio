# Building and testing

See [Requirements](../README.md#requirements) for the tools you need.

## Building

Clone with submodules. On an existing clone, run `git submodule update --init --recursive` instead.

```bash
git clone --recurse-submodules https://github.com/carbonengine/audio.git
```

Configure, build and test with a preset:

```bash
cmake --preset x64-windows-release
cmake --build .cmake-build-x64-windows-release --config Release
ctest --test-dir .cmake-build-x64-windows-release -C Release
```

Output goes to `.cmake-build-<preset-name>/`. Presets exist for `x64-windows`, `x64-osx` and `arm64-osx`, each with `-debug`, `-release`, `-internal` and `-trinitydev` variants.

## Local development in the monolith

To work on Carbon Audio against a game branch, generate a Visual Studio solution that installs into the branch's vendor folder:

```powershell
cmake --preset x64-windows-internal -A x64 -T v145 `
  -DINSTALL_TO_MONOLITH=ON `
  -DCMAKE_INSTALL_PREFIX="<vendor-folder>"
```

If configure fails on a missing `/scripts/toolchains/windows.cmake`, set `PATH_TO_VCPKG_ROOT` to `<repo>/vendor/github.com/microsoft/vcpkg`.

## Running tests

The tests are Python tests in [`tests/python/audiotests`](../tests/python/audiotests), run inside exefile. Run them:

- from the VS Code **Testing** view once CMake has configured, individually or all at once, or
- with `ctest` from the build directory.

## Debugging tests

To debug C++ code while a test runs, turn on the debug delay:

1. Run **Tasks: Run Task** → **Enable Debug Delay for Tests**. Each test now waits 7 seconds before it starts.
2. Run the test.
3. Run **Debug: Select and Start Debugging** → **Attach to Process** and pick `exefile.exe`.

Turn it off with **Tasks: Run Task** → **Disable Debug Delay for Tests**. macOS has its own variants of both tasks.

[`.vscode/launch.json`](../.vscode/launch.json) provides two attach configurations: **Attach to Process** for the Visual Studio debugger and **Attach to Process (GDB)** for GDB.

## Wwise test project

The tests use the Wwise project in [`Wwise/CarbonAudioTest`](../Wwise/CarbonAudioTest).

1. Install Wwise 2025.1.5.9095 with the [Audiokinetic Launcher](https://www.audiokinetic.com/en/library/wwise_launcher/).
2. Open `Wwise/CarbonAudioTest/CarbonAudioTest.wproj`.

When you add events or SoundBanks to the project for a test, regenerate both the SoundBanks and the audio metadata:

- **SoundBanks**: generate them from the SoundBanks view in Wwise. The project writes them to [`tests/python/audiotests/test/soundbanks`](../tests/python/audiotests/test/soundbanks).
- **Metadata**: regenerate [`SoundPrioritizationMetadata.json`](../tests/python/audiotests/test/soundbanks/SoundPrioritizationMetadata.json) with the `generate-sp-metadata` command from audio-scripts. See the audio-scripts documentation for installing and running it.
