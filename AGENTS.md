# Show Lyrics — repository instructions

Follow [CONTRIBUTING.md](CONTRIBUTING.md), [README.md](README.md), `.clang-format`, and `CMakePresets.json`. Those files are authoritative; do not duplicate or override their requirements here.

## Scope and architecture
- This is a Windows x64, C++17, JUCE 9.0.2, VST3-only plugin targeting Cubase Elements 15.
- Keep domain logic under `src/model/` and JUCE plugin integration under `src/plugin/`.
- Preserve audio pass-through unless a task explicitly changes it.
- Do not assume Cubase marker-track data is exposed through VST3.
- Preserve plugin identifiers and serialized-state compatibility; propose a decision before changing either.

## Engineering workflow
- Inspect relevant existing code, tests, and project decisions before editing.
- Prefer the smallest coherent change; avoid speculative features and unrelated refactoring.
- Use the repository's `.clang-format`; do not hand-maintain duplicate formatting rules.
- Retain `SPDX-License-Identifier: AGPL-3.0-only` in applicable C++ source and test files.
- Treat imported lyrics and restored state as untrusted. Enforce existing limits and validation.
- Do not allocate, lock, access files or networks, log, or perform UI work in the audio callback.
- Add focused regression tests for behavior changes. Follow existing test conventions unless a change is explicitly approved.
- Follow repository policy for English artifacts, Conventional Commits, DCO sign-off, and permission before pushing.

## Verification
- Run `git diff --check` and the relevant Debug and Release presets from `CMakePresets.json` when the Windows toolchain is available.
- State precisely which commands ran and which could not run. Do not claim successful compilation, tests, or host verification without evidence.
- For changes affecting the plugin, report whether behavior was verified in Cubase or only by automated tests.
- Do not edit unrelated files to satisfy formatting or style preferences.

## Documentation
- Keep Markdown readable in GitHub and Obsidian.
- Use the established repository documentation conventions; add YAML front matter only to documents intended for the Obsidian knowledge base, not to code or build files.
