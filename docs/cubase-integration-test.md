# Cubase Elements 15 integration test

## Scope and evidence boundary

This procedure verifies Show Lyrics transport following and read-only lyric display in Cubase Elements 15. Automated
tests verify the state pipeline, cue boundaries, generation changes, invalid input rejection, repeated fixture loading,
audio pass-through, and fixture isolation. They do not prove Cubase discovery, callback behavior, rendering, project
save/restore, or cycle behavior.

Do not commit or distribute a `.cpr` file created by another tool. Add a project file only after Cubase Elements 15 has
created it and every applicable test below has been recorded against that exact file.

## State-path audit

Before this integration fixture was added, `CueStateSerializer` already implemented the documented bounded `SLST`
format, but the processor did not use it. `getStateInformation()` wrote an empty raw `ValueTree`, and
`setStateInformation()` only checked a raw root type. It neither reconstructed a `CueState` nor published a cue
generation. Test cues therefore could not become active through host state restoration.

The processor now uses this single state path:

1. `setStateInformation()` deserializes and validates the complete bounded `CueState`.
2. A valid state receives the next processor-owned revision.
3. `CueGenerationExchange` publishes the complete immutable generation.
4. Invalid input or unavailable generation capacity leaves the last valid state unchanged.
5. `getStateInformation()` serializes the latest successfully published state with `CueStateSerializer`.

The optional Debug fixture loader does not publish cues directly. It parses the fixture, constructs a validated
`CueState`, serializes it with `CueStateSerializer`, and calls `setStateInformation()`. The normal host-state path is
therefore the only activation path.

## Canonical fixture

The machine-readable fixture is
[`tests/fixtures/CubaseIntegrationCues.json`](../tests/fixtures/CubaseIntegrationCues.json). It defines 120 BPM, 4/4,
and these start-inclusive, end-exclusive cue ranges:

| Cue | PPQ range | Cubase bars at 120 BPM, 4/4 | Lyric text |
|---|---:|---:|---|
| A | `[0, 16)` | 1–4 | `Erste Strophe` |
| B | `[16, 32)` | 5–8 | `Zweite Strophe` |
| C | `[32, 48)` | 9–12 | `Refrain` |
| D | `[48, 64)` | 13–16 | `Outro` |

## Build the fixture-enabled VST3

Use a separate build directory so the normal Debug preset remains fixture-free:

```powershell
cmake --preset windows-x64-debug `
  -B build/windows-x64-cubase-fixture `
  -DSHOW_LYRICS_ENABLE_CUBASE_TEST_FIXTURE=ON
cmake --build build/windows-x64-cubase-fixture `
  --config Debug `
  --target ShowLyricsPlugin_VST3
```

The bundle is generated at:

```text
build/windows-x64-cubase-fixture/ShowLyricsPlugin_artefacts/Debug/VST3/Show Lyrics.vst3
```

The option affects only the Debug configuration. The build runs a post-build binary check: an enabled Debug VST3 must
contain the fixture, while normal Debug and every Release VST3 must not contain it. Do not use the fixture-enabled build
for release packaging or performance.

## Create the Cubase project manually

Use a disposable validation installation with no other `Show Lyrics.vst3` copy visible to Cubase.

1. Close Cubase and install the fixture-enabled Debug bundle in the system VST3 directory.
2. Start Cubase Elements 15 and confirm that `Show Lyrics` is discovered without a plug-in scan error.
3. Create an empty project with a fixed tempo of 120 BPM and a 4/4 signature starting at bar 1.
4. Create one stereo audio track named `Show Lyrics Integration`.
5. Add or record a non-copyrighted continuous audio event covering bars 1–17. This prevents silence-dependent plug-in
   suspension from making callback observations ambiguous.
6. Insert one `Show Lyrics` instance on that track and open its editor.
7. Configure cycle ranges as required by T13 and T14. Use Cubase's transport cycle locators; do not create marker-track
   dependencies.
8. After the protocol passes, save the project from Cubase. Record the exact Cubase version, VST3 commit, project file
   name, and test date below.

## Test record

| Field | Value |
|---|---|
| Cubase Elements version | Not run |
| Show Lyrics commit | Not run |
| Project file | Not created |
| Test date | Not run |
| Tester | Not run |

## T01–T15 protocol

Record `Pass`, `Fail`, or `Blocked` and a concise observation for every test.

| ID | Action | Expected result | Result | Observation |
|---|---|---|---|---|
| T01 | Open the project and the plug-in editor while transport is stopped. | The VST3 loads. No lyric text is visible. | Not run | |
| T02 | Start playback at bar 1 / PPQ 0. | `Erste Strophe` appears; audio remains unchanged. | Not run | |
| T03 | Play through bar 4 up to immediately before bar 5 / PPQ 16. | `Erste Strophe` remains visible through the end-exclusive boundary approach. | Not run | |
| T04 | Cross into bar 5 / PPQ 16. | The display changes once to `Zweite Strophe`. | Not run | |
| T05 | Seek to bar 9 / PPQ 32 and play. | `Refrain` appears without intermediate inferred cues. | Not run | |
| T06 | Seek to bar 13 / PPQ 48 and play. | `Outro` appears. | Not run | |
| T07 | Play or seek to bar 17 / PPQ 64. | No lyric cue is displayed; the neutral no-cue state appears. | Not run | |
| T08 | While playing, seek forward from cue A to cue D. | The display changes directly from `Erste Strophe` to `Outro`. | Not run | |
| T09 | While playing, seek backward from cue D to cue B. | The display changes directly to `Zweite Strophe`. | Not run | |
| T10 | Stop while any cue is visible. | The lyric text clears after an observed stop; stopped is not reported as stale. | Not run | |
| T11 | Move the project cursor across cue ranges while stopped. | No lyric text becomes active. | Not run | |
| T12 | Restart at bar 9, then close and reopen the editor while playback continues. | `Refrain` returns only after fresh audio snapshots; no previously cached lyric flashes. | Not run | |
| T13 | Enable cycle PPQ `[8, 24)` / bars 3–6 and let it wrap twice. | The sequence is A → B → A on each wrap, with no inferred forward continuation. | Not run | |
| T14 | Enable cycle PPQ `[28, 36)` / bars 8–9 and let it wrap twice. | The sequence is B → C → B on each wrap. | Not run | |
| T15 | Enable cycle PPQ `[44, 52)` / bars 12–13, save in Cubase, close, reopen, and replay the cycle. | Cubase restores the plug-in; the sequence is C → D → C and audio remains unchanged. | Not run | |

## Completion rule

A `.cpr` artifact is eligible for the repository or a release attachment only when it was created by Cubase Elements 15,
the test record identifies the exact file and commit, and T01–T15 contain no unexplained failure. Until then, the project
file remains an explicitly manual deliverable.
