# Windows installer

## Scope

The Windows package installs:

- the 64-bit `Show Lyrics.vst3` bundle under the system VST3 directory;
- project, JUCE, and VST3 SDK license and notice files under `Program Files\Show Lyrics`.

The installer does not write to user preset or development plug-in directories.

## Requirements

- Developer PowerShell for Visual Studio 18
- configured Windows x64 Release build
- NSIS 3.13

The build script locates NSIS from `SHOW_LYRICS_NSIS_EXECUTABLE`, `PATH`, the documented local tool directory, or the standard Program Files location. It rejects other NSIS versions.

## Build

```powershell
.\scripts\build-windows-installer.ps1
```

Expected files:

```text
build/windows-x64-release/package/Show-Lyrics-0.1.0-windows-x64.exe
build/windows-x64-release/package/Show-Lyrics-0.1.0-windows-x64.exe.sha256
```

## Automated verification

The packaging test verifies:

- VST3 bundle and manifest presence;
- installer PE header and version metadata;
- SHA-256 checksum integrity;
- administrator execution level;
- required, visible, read-only components;
- system-wide destinations;
- project and dependency license files;
- absence of known user-scoped installation paths.

This does not replace installation, discovery, project restoration, and clean uninstall testing in Cubase on a disposable validation system.
