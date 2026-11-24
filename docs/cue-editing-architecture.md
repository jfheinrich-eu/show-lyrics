# Cue editing architecture

Phase 11.1 · [Issue #28](https://github.com/jfheinrich-eu/show-lyrics/issues/28) ·
[Parent #11](https://github.com/jfheinrich-eu/show-lyrics/issues/11)

Verified baseline: `584b28696fd348fe0592a95b8c27121be45666b0`. The local checkout was clean and matched
the remote `main` at inspection. The design below is proposed for owner review.

## Verified implementation and gaps

| Component | Baseline behavior and gap |
|---|---|
| [LyricCue and CueState](../src/model/CueState.cpp) | Whole-state validation covers UUIDs, ranges, overlap, text, count and display order. Validation reports a candidate index and, for overlap, a conflicting index. There is no transactional editing API. |
| [CueStateSerializer](../src/model/CueStateSerializer.cpp) | Bounded `SLST` v1 serializer and restore parser. The processor calls it for host state. The test fixture exercises the same restore routine. |
| [Processor](../src/plugin/PluginProcessor.cpp) and [generation exchange](../src/model/CueGenerationExchange.cpp) | The processor tracks the newest published revision under `cuePublicationMutex_`. Three exchange slots hold immutable states and prepared `CuePlaybackIndex` instances. The audio callback takes the latest ready generation, and the control side reclaims retired slots. `visitCueState()` reads one named generation while protected. A revisioned copy of the latest committed state and a compare-and-publish edit API are missing. |
| [Transport extraction](../src/plugin/HostPositionSnapshot.cpp) and [mailbox](../src/model/TransportSnapshotMailbox.h) | Only the audio callback queries the playhead. PPQ has an independent presence flag and must be finite. A stopped snapshot can carry PPQ while carrying no active cue. The eight-slot mailbox drops a new snapshot when full and latches overflow. The processor drains a bounded batch and clears queued data on prepare. |
| [Editor](../src/plugin/PluginEditor.cpp) and [display model](../src/plugin/LyricsDisplayModel.cpp) | One 30 Hz editor timer consumes transport. Lyrics resolve through the snapshot's revision and cue index. The model handles stop, invalid data, overflow and unavailability, and uses a 500 ms arrival timeout. Consumption time and the first-read guard cannot prove when a queued sample was measured. No position capture or cue management exists. |

The baseline README status describes an earlier scaffold. The source and tests are the basis for the findings above.
The existing [Cubase protocol](cubase-integration-test.md) records owner-reported T01–T15 results for the transport
and display fixture, with incomplete build provenance. Cue editing and capture require their own host evidence.

## Proposed ownership and interfaces

- The processor is the sole authority for committed cues and the revision counter. The newest successfully published
  generation is committed immediately on the control side. Audio activation may follow on a later block or skip
  intermediate revisions. Host save reads the committed generation even when audio has yet to activate it.
- The editor owns a read copy, selection by UUID and one pending draft on the message thread. It may allocate, sort,
  normalize text and produce diagnostics. The view never holds a pointer into exchange storage or writes host state
  directly. The performance view continues to resolve the exact audio revision/index through the protected visitor.
- The audio callback owns host observation and generation activation. It reads the prepared playback index and sends
  compact snapshots. Cue strings, validation, allocation, locks, parsing, logging, file access and UI stay off
  the audio callback. Preserve mono/stereo pass-through and the existing lifecycle contract.

Proposed non-real-time processor calls, expressed as contracts:

| Call | Contract |
|---|---|
| `readCueState()` | Return an owned `{revision, CueState}` value copied together under `cuePublicationMutex_`. At construction in normal builds return revision 0 with a valid empty state. Revision 0 is never published into the exchange. A successful empty restore or deleting the last cue uses a positive revision. |
| `applyCueEdit(expectedRevision, operation)` | Return `{status, committedOrObservedRevision, diagnostic?, affectedUuid?}`. The observed revision is the current revision under the mutex at the decisive check. Success returns the revision assigned to this operation. The caller can obtain a fresh copy separately and must allow for intervening restores. |

Operations are `Create(fields)`, `Update(uuid, fields)`, `Delete(uuid)` and
`Move(uuid, targetDisplayIndex)`. Create appends and allocates a canonical lowercase v4 UUID on the control
side, checking uniqueness in the candidate. A UUID generation failure leaves the draft intact. Updates preserve
UUID and order. Delete compacts order. Move takes a final zero-based index in the unchanged-size list, preserves
relative order of the other cues and rewrites contiguous ranks. Read copies render in display order. Playback retains
timeline order and resolves indices in the same generation that produced them. Identical edits and moves may return
`unchanged` after a successful revision check, without consuming a revision or slot.

## Transaction, conflicts and publication failures

1. Take the control mutex, compare the expected revision (including 0), and copy the committed state. Return
   `revisionConflict` on mismatch. Release the mutex before building the candidate.
2. Apply the operation to the local copy. Reject an unknown target or out-of-range move separately. Normalize input
   and validate with `CueState::create()`. Check the serialized size off the audio thread with the existing
   serializer. No storage or generation changes occur during this work.
3. Reacquire the same mutex and compare the revision again before returning candidate diagnostics or publishing.
   Reject a stale result even when the candidate is otherwise valid. For changed valid candidates, check revision
   capacity, reclaim retired slots and attempt publication with the next processor-owned revision in the same
   critical section. Reuse the locked publication path so restore and editing share sequencing and ownership.
4. Only confirmed publication advances the committed revision and consumes the next revision. Treat the generation's
   ready publication as the handoff and report success without waiting for audio. Keep UI callbacks, notifications
   and any user-facing work outside the mutex. Keep the cue and transport mutexes unnested.

| Result | Editor response |
|---|---|
| `applied` / `unchanged` | Finish the draft and refresh the committed copy. Keep selection by UUID if present. |
| `invalidCandidate` | Preserve the draft. Return field/range diagnostics and a conflict description built from the exact validated candidate. |
| `unknownCue` / `invalidTargetOrder` | Preserve the draft and explain the invalid target. Refresh the committed list when needed. |
| `revisionConflict` | Preserve the draft, refresh the committed list and require deliberate reconciliation. Offer review of latest values alongside the draft. When the original UUID was deleted, allow explicit conversion of an update into a new cue. Any retry uses a newly confirmed revision. |
| `slotsFull` | Keep the draft and current committed state. Offer a manual retry with the original expected revision. Audio may never retire slots while callbacks are suspended. Avoid spin, hidden pending writes and unbounded queues. |
| `revisionExhausted` | Keep the draft and committed state readable. Further changed writes are unavailable in this processor instance. Never wrap or reuse a revision. |
| `publicationFailed` | Keep the draft and last committed state. Give an actionable failure message. Unexpected exchange or candidate-preparation errors must follow the same guarantee. |

Host restore may arrive on a different thread. The existing bounded deserialize/validate step runs before acquiring the
control mutex. Successful host restore acts as a replacement in publication order and changes the revision, even when
the payload matches. If a restore wins during candidate preparation, the edit's second revision check fails. If the
edit wins first, a later successful restore replaces it. Invalid or unpublishable restores leave the committed
revision intact. The JUCE callback remains void. There is no deferred restore queue or editor draft persistence.

## Proposed shared transport capture

One message-thread coordinator owns the existing logical mailbox drain for the editor. Its timer updates a shared
transport observation for the lyrics model and the capture controls. A capture click uses the same refresh routine and
then evaluates the observation synchronously. The controls never drain a second queue or query the playhead. The
processor's lifecycle reset remains serialized with draining. Both consumers receive the same newest snapshot and
overflow/lifecycle status, with different eligibility rules for stopped playback and active cues.

Use a compact confirmation ticket to establish sample freshness without a clock call in the audio callback:

1. On editor open, lifecycle change or observed overflow, invalidate capture immediately. Discard the affected batch
   for capture and request a new nonzero processor-unique ticket. Issue tickets on the message thread, recording
   monotonic issue time **before** making the request visible. One outstanding ticket is sufficient.
2. At block entry, the producer reads the request once using a lock-free atomic acquire load, then samples the
   playhead and includes the observed ticket in that block's snapshot. Release-store the request and use the
   mailbox's existing publication protocol. Verify required atomics are always lock-free on the pinned x64 target.
   No producer clock call, retries, allocation or additional queue.
3. Accept a matching snapshot only in its editor session and current audio lifecycle, with age strictly below 500 ms
   from ticket issue. Ticket age bounds sample age from above because sampling follows the acquire load. A drained
   snapshot cannot renew its own age. After confirmation request the next ticket and keep the confirmed value only
   until expiry or invalidation. Expired requests are replaced. Tickets are never reused and exhaustion disables
   capture. A new editor discards all cached capture state and requests its own ticket.
4. Add a lifecycle identity to the drain result. Serialize prepare, release and drain on the non-audio side after
   confirming the host lifecycle exclusion used by the existing mailbox reset. Advance the lifecycle identity at
   prepare/release and preserve prepare's queue reset. Reject on unavailable audio or lifecycle mismatch. Recheck
   lifecycle at the click refresh, and fail closed if lifecycle identity cannot advance. Capture linearizes at the
   successful observation; a subsequent stop, seek or release does not change the captured draft value.
5. On any overflow, invalidate capture and request a new confirmation after the overflow is observed. Ignore the
   overflowed batch for both consumers. Only a subsequent safe result can resume display. Capture additionally
   requires confirmation of the new ticket. A newer invalid transport result invalidates a prior capture value.

Proposed UI-facing capture result is
`Captured{ppq, playing}` or one of `awaitingFreshSnapshot`, `stale`, `overflow`, `audioUnavailable`,
`invalidHostPosition`. A valid sample has transport status and finite, nonnegative PPQ. Optional tempo, bar and
meter data are irrelevant to capture. Use the exact host PPQ, allowing a fresh stopped sample as well as a playing one.
Copy it only to the requested draft field. A captured value can leave the draft range temporarily invalid. Validation
on Apply still requires `0 <= start < end`. Capture does not require an active cue or a positive cue revision.

Display keeps its current stop, no-cue and timeout behavior. A fresh observed stop clears lyrics, and a later absence
of snapshots can make that observation stale. If Cubase suspends callbacks, capture is unavailable once the last
confirmed sample expires, or immediately on explicit audio release. Cursor moves without callbacks are unobservable,
including within the freshness window. Capture represents the most recent qualifying audio observation, which may lag
the cursor after a seek. Explain this in the UI and suggest briefly playing at the desired position when necessary.
Use monotonic time for age only. Never extrapolate PPQ from time, tempo or samples.

## Proposed editor behavior

- Keep Performance View read-only with existing lyrics and status handling. Cue Management contains a deterministic
  display-order list, selection by UUID, title and multiline lyrics fields, numeric start/end PPQ, Capture Start/End,
  Add, Delete, Move Up/Down, Apply and Cancel. The transport coordinator is independent of which view is visible.
  Keep the separate performance-view work under #13 out of scope.
- Add opens a blank draft. Selecting a cue starts an editable copy. Delete and move stage the selected operation and
  use the same explicit Apply/Cancel path. Stage one operation at a time. Protect dirty edits on selection changes
  or a new operation by offering Apply, Discard or Stay. Switching views preserves the draft. Closing the host-owned
  editor discards the in-memory draft; committed data survives editor reopen. Do not rely on intercepting host close.
- Render ranges as `[start, end)` with labels “Start PPQ (included)” and “End PPQ (excluded)”. Show fractional
  quarter-note values without hidden quantization or bar conversion. Round-trip display/edit formatting without
  changing untouched doubles. Normalize CRLF and CR to LF before cue validation. Use plain Unicode UTF-8, preserve
  the content of valid text, and reject invalid UTF-8, tabs, NUL and remaining disallowed controls.
- Allow empty title and require nonempty lyrics. Enforce 256 UTF-8 bytes per title, 16 KiB per lyric, 512 KiB
  combined, 1,024 cues and 1 MiB serialized state. Preserve the existing `SLST` v1 schema and immutable plug-in
  identifiers. Validate the complete candidate including exact half-open overlap (`a < d && c < b`). Mark affected
  fields and show the conflicting cue's title and range, using its UUID internally and an untitled label if needed.
  Keep attempted input and capture values on validation, conflict or publication failure. Use bounded list rows
  and only render full lyrics for the selected cue to keep the maximum valid state usable.

## Verification plan and acceptance coverage

| Requirement | Planned evidence |
|---|---|
| Editable title, lyrics and half-open range, deterministic create/update/delete/reorder (#11) | Domain and processor tests for empty read/restore, each operation, UUID identity, compaction, adjacent boundaries, unchanged operations and display order independent of playback. Editor tests for mode, selection, dirty draft, Apply/Cancel, staged deletion and moves. |
| Capture start/end and stopped-host behavior (#11, #28) | Fake playhead and injected monotonic clock tests for play, stop with PPQ, seek, backward jumps, cycle wrap, exact timeout, late queued samples, dropped stops, stale cached readings, overflow recovery with concurrent producer, unavailable/old lifecycle, reopen and no callbacks. Verify the timer/click share a drain and capture changes only the selected draft endpoint. |
| Invalid edits, conflicts and publication failures (#11, #28) | Field mapping for UTF-8, line endings, byte/count boundaries, full containment overlap, conflict identity and failed Apply. Controlled interleavings around restore and both revision checks, plus slot saturation, retry after audio retires slots and the control side reclaims them, exhaustion and save/restore of the exact committed state. |
| Reuse, RT boundaries and maximum valid state (#11, #28) | Extend `LyricCueTests`, `CuePlaybackIndexTests`, `RealtimeCommunicationTests`, `LyricsDisplayModelTests` and `PlayHeadIntegrationTests` as applicable. Review callback/lifetime paths, run Debug/Release CTest and relevant CI/TSan coverage, verify bit-preserving pass-through and exercise selection/editing at 1,024 cues and 512 KiB of valid text. |
| Architecture, sequencing and approval (#28) | Review this specification against the issue criteria and obtain owner approval before the dependent implementation phases. |

For #33, use a normal fixture-free VST3 in Cubase Elements 15 and a scratch project with an original audio clip.
Record the exact host build, Windows version, configuration, tested commit/binary hash, project and per-case
observations. Create adjacent cues through capture, edit both fields and text, reject overlap and invalid Unicode or
over-limit text, reorder and delete, and verify boundary playback, unchanged audio, save/reopen and editor reopen.
Exercise stopped cursor moves with callbacks present and absent, fresh and expired capture, seeks in both directions,
cycle wrap and a restart or reactivation. Repeat across a meter change to confirm PPQ semantics. Record which stopped
callback case actually occurs. Use automated coverage for overflow or suspended-callback paths that the host cannot
reliably reproduce, and mark unverified host cases explicitly.

## Implementation boundaries and release considerations

| Issue | Deliverable | Depends on |
|---|---|---|
| [#29](https://github.com/jfheinrich-eu/show-lyrics/issues/29) | Processor-owned state reads and operations, transactional publication, restore interaction and failure tests. | Approved #28 |
| [#30](https://github.com/jfheinrich-eu/show-lyrics/issues/30) | Shared transport coordinator, freshness/lifecycle contract, capture outcomes and regression tests. | Approved #28 |
| [#31](https://github.com/jfheinrich-eu/show-lyrics/issues/31) | Cue Management controls and draft flow using #29/#30, alongside the existing performance display. | #29 and #30 |
| [#32](https://github.com/jfheinrich-eu/show-lyrics/issues/32) | Complete validation feedback, explicit conflict/retry UX and maximum-state usability evidence. | #31 |
| [#33](https://github.com/jfheinrich-eu/show-lyrics/issues/33) | Automated integration evidence and manual Cubase record for all parent criteria. | #32 |

Phase 11.1 changes documentation only. The implementation scope needs no new libraries, JUCE modules, tooling pins,
imports, network access or persistence channels. Treat host state and input as untrusted and bound all preparation.
Keep fixture code out of the Release VST3. [Issue #27](https://github.com/jfheinrich-eu/show-lyrics/issues/27)
remains a separate release gate. Pass forward evidence on lifetimes, malformed input, integer conversions, resource
limits, Release module/fixture contents and CI provenance. The final audit owns dependency inventory/SBOM, advisory
review, installer and binary inspection, and fail-closed release approval bound to the audited commit and artifact hashes.

## Decisions for project-owner approval

- Adopt capture from fresh stopped observations with the same 500 ms bound as playing observations, subject to Cubase
  validation of cursor lag and callback suspension. Confirm the proposed ticket handshake and lifecycle verification.
- Adopt explicit single-operation drafts for create, edit, delete and reorder. Confirm discard on host editor close,
  manual retry on full slots and explicit reconciliation against a new revision after conflict.
- If Cubase returns misleading PPQ while stopped or the lifecycle exclusion cannot be confirmed, record the evidence
  and obtain approval for the revised eligibility or synchronization contract before shipping.
