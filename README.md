# Carbon Audio

Carbon Audio is the audio component of the Carbon engine and is used by EVE Online and EVE Frontier. It wraps the [Wwise](https://www.audiokinetic.com/en/wwise/overview/) sound engine and adds the game-facing systems that sit on top of it, such as sound prioritization and line-of-sight occlusion. Its classes are exposed to Python through Blue and to the rest of the engine through C++ interfaces.

## Features

- **Sound engine lifecycle**: initializes, enables and shuts down Wwise, and loads and unloads SoundBanks.
- **Emitters and listeners**: 3D emitters, a listener, and dedicated UI and music players.
- **Sound prioritization**: puts emitters to sleep and wakes them based on metadata exported from the Wwise project.
- **Spatial audio**: renders to 3D endpoints such as Dolby Atmos and Windows Sonic, with a stereo fallback.
- **Spatial audio geometry**: registers Trinity meshes as Wwise geometry sets.
- **Obstruction and occlusion**: fades obstruction and occlusion per emitter, driven by line-of-sight queries or by values the game sets.
- **Curve-driven audio**: posts events from keys on Trinity curves and drives Trinity curve sets from Wwise parameters.
- **Stretch audio**: emitters that follow the listener along effects spanning two points.
- **Audio input**: streams external audio, such as in-game video, into Wwise.
- **Streaming IO**: resolves SoundBank and media paths through Blue, with a separate folder for essential content.
- **Tooling**: connects to the Wwise authoring tool through WAAPI and logs posted events, states, switches and RTPCs for debugging.

## How it fits into Carbon

Carbon Audio builds as `_audio2`, a Blue module that Python imports. The [`audio2`](python/audio2) Python package wraps it with `AudioManager`, which the game uses to set up audio and manage SoundBanks.

| Component | Role |
|---|---|
| Blue (`carbon-blue`) | Object model, Python exposure and resource paths. Every Carbon Audio class is a Blue class. |
| Trinity audio API (`carbon-trinityaudioapi`) | Interfaces such as `ITr2AudEmitter` and `ITr2AudGeometry` that let Trinity own emitters and push geometry without linking Wwise. |
| Destiny | The game's ballpark implements [`IEveObstructionQuery`](include/IEveObstructionQuery.h) to answer line-of-sight queries for occlusion. |
| Math (`carbon-math`) | Vector and matrix types. |
| exefile (`carbon-exefile`) | Hosts the Python test suite. |
| Wwise | The sound engine and its effect plugins. |

Carbon Audio is published as the `carbon-audio` port in the [Carbon vcpkg registry](https://github.com/carbonengine/vcpkg-registry).

## Requirements

- **Wwise SDK** 2025.1.5.9095. Internal developers get it from the private registry. External contributors need their own Wwise license, which is free for personal use, from [Audiokinetic](https://www.audiokinetic.com/en/wwise/overview/).
- **Windows**: Visual Studio 2026 with the v145 toolset.
- **macOS**: Xcode command line tools.
- **CMake** 3.31 or newer.
- **Git** with submodule support.

The remaining dependencies, including Python 3.12 and TBB, are installed by vcpkg from [vcpkg.json](vcpkg.json) when you configure.

### Building

```bash
git clone --recurse-submodules https://github.com/carbonengine/audio.git
cd audio
cmake --preset x64-windows-release
cmake --build .cmake-build-x64-windows-release --config Release
ctest --test-dir .cmake-build-x64-windows-release -C Release
```

macOS presets, the internal monolith workflow and debugging are covered in [Building and testing](docs/development.md).

## Documentation

- [Getting started](docs/getting-started.md): initializing Carbon Audio from Python and managing SoundBanks.
- [Sound prioritization](docs/sound-prioritization.md): the audio metadata file and the Wwise custom properties.
- [Spatial audio](docs/spatial-audio.md): output device setup for 3D audio and stereo.
- [Building and testing](docs/development.md): presets, local development, tests and the Wwise test project.

## 🤝 Contributing

Contributions are welcome. Please read the Carbon Engine [contributing guide](https://github.com/carbonengine/.github/blob/main/CONTRIBUTING.md) before opening an issue or pull request. It covers the workflow, the CLA and the pull request template, and applies to every `carbonengine` repository. Please also follow the [Code of Conduct](https://github.com/carbonengine/.github/blob/main/CODE_OF_CONDUCT.md), and report security issues privately as described in the [Security Policy](https://github.com/carbonengine/.github/blob/main/SECURITY.md) rather than in a public issue.

By submitting a pull request or otherwise contributing to this project, you agree to license your contribution under the [MIT License](LICENSE.md), and you confirm that you have the right to do so.

## 📄 License and Legal Notices

© 2026 Fenris Creations

This software is provided by Fenris Creations and does not include or distribute any third-party libraries or frameworks.

Trademark Notice: Fenris Creations is a trademark of CCP ehf.

This project is licensed under the [MIT License](LICENSE.md). Nothing in the [MIT License](LICENSE.md) grants any rights to Fenris Creations' trademarks or game content.
