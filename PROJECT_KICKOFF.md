# Show Lyrics Project Kickoff

## Purpose

Show Lyrics displays the lyric text assigned to the musical range currently reached by the Cubase transport.

## MVP

The first usable version will:

1. Run as a Windows x64 VST3 audio insert in Cubase Elements 15.
2. Pass audio through without intentional modification.
3. Let the user create, edit, order, and delete lyric cues.
4. Store each cue as a half-open musical range with a title and lyric text.
5. Follow play, stop, seek, backward jumps, and cycle playback.
6. Display the active cue in a resizable performance view while the editor is open.
7. Save and restore all cue data with the Cubase project.

## Explicit limits

- The plug-in will not read, create, or modify Cubase marker-track events.
- The plug-in will not force Cubase to open its editor window during playback.
- The MVP has no cloud service, user account, analytics, browser view, or network dependency.
- The repository will not contain copyrighted lyrics or example song texts.
- Import and export formats are not part of the initial scaffold.

## Timing model

- Store cue boundaries as quarter-note positions relative to the project timeline.
- Treat ranges as start-inclusive and end-exclusive.
- Request musical project position, bar position, time signature, and transport state from the host.
- Never derive playback position from a wall clock.
- Treat missing or invalid host position as no active cue.
- Capture positions from the current transport position to avoid ambiguous numeric bar conversion across meter changes.

## Threading boundary

The audio processor may observe host timing in the real-time callback. It must publish only compact state through a bounded lock-free mechanism. The editor owns rendering, text editing, allocation, and persistence preparation on non-real-time threads.

## Confirmed identity

The project uses these immutable identifiers:

| Field | Value |
|---|---|
| Company | `jfheinrich` |
| Product | `Show Lyrics` |
| CMake target | `ShowLyricsPlugin` |
| Manufacturer code | `Jfhe` |
| Plug-in code | `ShLy` |
| Bundle identifier | `eu.jfheinrich.showlyrics` |

These identifiers were confirmed on November 16, 2026. Changing them later can make existing Cubase projects fail to locate the plug-in.

## License

Show Lyrics is licensed under `AGPL-3.0-only`. JUCE is used under its AGPLv3 option. Public binary distribution requires the complete corresponding source, license notices, and the build and installation information required by the license.

## Confirmed decisions

### UX-001: Cue overlap behavior

- Cue ranges must not overlap.
- Two half-open ranges `[a, b)` and `[c, d)` overlap exactly when `a < d && c < b`.
- A candidate range must be checked against every other cue. Checking only whether a candidate boundary lies inside another range is insufficient because the candidate may completely contain an existing range.
- Adjacent ranges are valid. For example, `[15, 23)` and `[23, 30)` do not overlap.
- A create or edit operation that would introduce an overlap is rejected. The last valid cue state remains unchanged.
- The editor marks the affected range fields and identifies the conflicting cue by title and range.
- Cue ordering affects editor presentation only. Playback does not use ordering as a tie-breaker because the non-overlap invariant permits at most one active cue at any musical position.

The approved decision and required examples are tracked in GitHub issue #5.

## Open decisions

| ID | Decision | Required before |
|---|---|---|
| UX-002 | Define text formatting allowed in lyrics. | State schema implementation |
| UX-003 | Decide whether stopped transport shows the cue at the cursor. | Transport implementation |
| DATA-001 | Define state schema, versioning, and size limits. | Project persistence |

## Delivery sequence

1. Confirm licensing and immutable identity.
2. Define and test the versioned cue-state model.
3. Implement host-position observation and real-time-safe UI notification.
4. Implement cue editing and project-state persistence.
5. Implement the performance view.
6. Validate Debug and Release bundles with Steinberg Validator and Cubase Elements 15.
