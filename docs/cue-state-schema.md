# Cue State Schema

## Scope

This document defines version 1 of the persistent Show Lyrics cue-state format. The format is internal to the plug-in and is stored by the VST3 host as opaque component state.

## Binary envelope

The serialized state starts with a fixed eight-byte envelope:

| Offset | Size | Content |
|---|---:|---|
| `0` | 4 bytes | ASCII magic `SLST` |
| `4` | 4 bytes | Unsigned little-endian payload size |
| `8` | Payload size | Binary JUCE `ValueTree` |

The payload size must match the remaining input exactly. The complete envelope and payload must not exceed 1 MiB.

## ValueTree schema

The root has type `ShowLyricsState` and exactly one property:

| Property | Type | Required value |
|---|---|---|
| `schemaVersion` | Integer | `1` |

Each direct child has type `Cue`, has no children, and contains exactly these properties:

| Property | Type | Rule |
|---|---|---|
| `id` | String | Unique canonical lowercase RFC 4122 version 4 UUID |
| `startQuarterNotes` | Double | Finite and at least zero |
| `endQuarterNotes` | Double | Finite and greater than the start |
| `title` | String | Valid plain UTF-8, at most 256 bytes |
| `lyrics` | String | Valid non-empty plain UTF-8 with LF line endings, at most 16 KiB |
| `displayOrder` | Integer | Unique contiguous value from zero to cue count minus one |

Numeric bar numbers are not persisted. Cue ranges are start-inclusive and end-exclusive.

## State invariants

- The state contains at most 1,024 cues.
- Cue identifiers are unique.
- Cue ranges do not overlap. Adjacent ranges are valid.
- Combined title and lyric text does not exceed 512 KiB in UTF-8.
- Display order affects editor presentation only.
- Playback lookup uses musical start position and never uses display order as a tie-breaker.
- Tabs, NUL, CR, DEL, C0 controls other than LF, and C1 controls are invalid.

## Restore procedure

1. Reject a null, undersized, or oversized input block.
2. Validate the envelope magic and exact payload length.
3. Scan the binary payload with bounded reads. Reject unexpected types, properties, child nodes, counts, string lengths, or trailing data before invoking the JUCE parser.
4. Parse the structurally safe payload into a temporary `ValueTree`.
5. Convert every cue into a temporary domain model.
6. Validate identifiers, text, positions, order, limits, and overlap rules.
7. Publish the complete immutable state only after every validation step succeeds.

Any failure rejects the complete candidate state. Callers must retain the last valid state. Parsing, allocation, validation, and publication preparation must not occur in the audio callback.

## Version policy

Version 1 is the only supported version. Missing, malformed, or unknown versions are rejected. A future version requires an explicit migration with dedicated boundary, invalid-input, and round-trip tests. Unknown versions are never loaded on a best-effort basis.
