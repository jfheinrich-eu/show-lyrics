<p align="center">
  <img src="assets/header.png" alt="ShowLyrics logo">
</p>

# Show Lyrics

Show Lyrics is an early-stage VST3 plug-in for displaying lyrics associated with musical timeline ranges in Cubase Elements 15.

[![CI](https://github.com/jfheinrich-eu/show-lyrics/actions/workflows/ci.yml/badge.svg)](https://github.com/jfheinrich-eu/show-lyrics/actions/workflows/ci.yml)
[![CodeQL](https://github.com/jfheinrich-eu/show-lyrics/actions/workflows/codeql.yml/badge.svg)](https://github.com/jfheinrich-eu/show-lyrics/actions/workflows/codeql.yml)
[![Codecov](https://codecov.io/gh/jfheinrich-eu/show-lyrics/graph/badge.svg)](https://codecov.io/gh/jfheinrich-eu/show-lyrics)

## Status

The repository contains the initial Windows x64 project scaffold. The plug-in currently passes audio through unchanged and shows a placeholder editor. Cue editing, host-transport tracking, project-state persistence, and performance display behavior are not implemented yet.

Do not use this build in a live performance.

## Technical scope

- C++17
- JUCE 9.0.2 pinned to an immutable commit
- VST3 only
- Windows 10 x64 target
- Visual Studio Build Tools 2026 and Windows SDK `10.0.26100.0`
- CMake 3.25 or newer with checked-in presets
- Cubase Elements 15 as the reference host

The VST3 API provides musical transport position but does not expose Cubase marker-track contents. Native Cubase cycle markers can be used as a separate visual aid, but cue data must be stored by the plug-in.

## Build

Open a Developer PowerShell for Visual Studio 18, then run:

```powershell
cmake --fresh --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug

cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release
```

The VST3 bundle is generated below:

```text
build/windows-x64-release/ShowLyricsPlugin_artefacts/Release/VST3/Show Lyrics.vst3
```

The build does not install the plug-in into a system directory.

## Windows installer

Build the verified NSIS installer from Developer PowerShell:

```powershell
.\scripts\build-windows-installer.ps1
```

The installer and SHA-256 checksum are written below `build/windows-x64-release/package/`. See [Windows installer](docs/windows-installer.md) for scope and verification.

Publishing a GitHub release with a tag matching the project version, such as `v0.1.0`, builds the same installer and attaches both files to the release. See [Releasing](docs/releasing.md).

## Coverage

The Coverage workflow collects native MSVC coverage for the domain tests and uploads the normalized Cobertura report to Codecov. Coverage is informational until a stable baseline exists. See [Code coverage](docs/code-coverage.md).

## Project decisions

The initial scope, identifiers, constraints, and unresolved decisions are recorded in [PROJECT_KICKOFF.md](PROJECT_KICKOFF.md).

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before proposing a change. Security reports belong in the private channel described in [SECURITY.md](SECURITY.md).

## License

Show Lyrics is licensed under the [GNU Affero General Public License v3.0 only](LICENSE). JUCE is used under its AGPLv3 option. Binary distribution must include the corresponding source and comply with all applicable notice and installation-information obligations.
