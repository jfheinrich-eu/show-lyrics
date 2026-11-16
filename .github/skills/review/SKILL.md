---
name: review
description: Review current changes or the complete clean Show Lyrics repository for engineering defects without modifying files.
disable-model-invocation: true
---
Perform a complete engineering review of Show Lyrics without modifying files.

## Scope

1. Resolve the repository root.
2. Run `git status --short --untracked-files=all`, `git diff`, and `git diff --cached`.
3. If staged, unstaged, or untracked changes exist, review those changes and their impact on surrounding code.
4. Read complete relevant untracked files because Git diff does not include them.
5. If the working tree is clean, review the complete repository.
6. Stop if a merge, rebase, cherry-pick, revert, or bisect is active, or if `HEAD` is detached.

Apply `.github/copilot-instructions.md` and matching path-specific instructions.

## Review criteria

Review:

- correctness, regressions, architecture, maintainability, and public contracts;
- C++17 and JUCE usage, ownership, lifetime, thread safety, and numeric boundaries;
- allocation, locking, I/O, logging, exceptions, or UI work on the audio thread;
- host transport handling for play, stop, seek, backward jumps, cycle playback, missing position, meter changes, and block boundaries;
- cue-range invariants, including finite positions, start-inclusive/end-exclusive behavior, ordering, overlaps, empty lyrics, and unsupported values;
- audio transparency for supported mono and stereo layouts;
- bounded real-time-to-UI state transfer and editor lifetime behavior;
- project-state versioning, restoration, input limits, transactional failure, and backward compatibility;
- Unicode text handling, large lyrics, malformed state, and denial-of-service boundaries;
- tests, documentation, licensing, dependency provenance, and build reproducibility;
- agreement between CMake, presets, compiler generation, architecture, Windows SDK, JUCE version, CI, CodeQL, and documentation.

Distinguish advisory editor configuration from enforced setup. Report a defect only when evidence supports it.

## Verification

Run safe read-only checks and relevant Debug and Release builds or tests when available. Use Steinberg Validator when a VST3 binary is in scope and the tool is available. Do not claim a check passed unless it ran successfully.

## Output

Report only actionable findings ordered by severity:

- P0: destructive or release-blocking;
- P1: high-impact correctness, real-time, compatibility, or security defect;
- P2: material maintainability, test, or reliability issue;
- P3: low-impact improvement.

For each finding, provide a concise title, exact file and line, evidence, impact, and recommended correction. Then list open questions, checks performed, checks not performed, residual test gaps, and a short conclusion. If no findings exist, say so explicitly.
