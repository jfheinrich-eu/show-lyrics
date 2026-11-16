---
name: sec-review
description: Review current changes or the complete clean Show Lyrics repository for security risks without modifying files.
disable-model-invocation: true
---
Perform a security-focused review of Show Lyrics without modifying files.

## Scope

1. Resolve the repository root.
2. Use `git status --short --untracked-files=all`, `git diff`, and `git diff --cached` to determine scope.
3. Review changed files and their security impact when the working tree is dirty. Otherwise review the complete repository.
4. Read complete relevant untracked files.
5. Apply repository and path-specific instructions.

## Threat model

Cover at least:

- malformed, oversized, deeply nested, or incompatible project-state data;
- hostile Unicode, control characters, extremely long lyric text, and rendering denial of service;
- invalid floating-point positions, overflow, NaN, infinity, overlapping cues, and pathological cue counts;
- unsafe real-time-to-UI publication, data races, lifetime errors, and use-after-free;
- allocation, locks, waits, file access, network access, logging, UI calls, and exceptions on the audio thread;
- invalid or discontinuous host timing, seeks, loops, meter changes, and missing context;
- untrusted paths, traversal, symbolic links, reparse points, and file races if import or export exists;
- dependency, CMake FetchContent, GitHub Actions, build-chain, validator, and artifact compromise;
- memory corruption, integer conversion, denial of service, and unsafe state restoration;
- credentials, workflow permissions, pull-request trust, secret exposure, and artifact integrity;
- copyrighted lyrics, contributor provenance, JUCE licensing, and AGPL corresponding-source obligations.

Do not expose secret values. Run safe read-only security checks when available. Do not modify files.

## Output

Report actionable findings by P0 through P3 severity. Include exact file and line, attack or failure scenario, impact, relevant CWE where applicable, and concrete remediation. Then list trust boundaries, checks performed, checks not performed, and residual risks. If no findings exist, say so explicitly and identify untested attack surfaces.
