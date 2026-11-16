---
name: review-project
description: Run a complete Show Lyrics engineering and security review, then synchronize project documentation after explicit approval.
disable-model-invocation: true
---
Perform a complete engineering review, security review, and documentation synchronization for Show Lyrics.

## Scope and repository state

1. Resolve the repository root.
2. Stop without changing anything if a merge, rebase, cherry-pick, revert, or bisect is active, or if `HEAD` is detached.
3. Run `git status --short --untracked-files=all`, `git diff`, and `git diff --cached`.
4. Review current changes and their surrounding impact when the working tree is dirty. Read complete relevant untracked files.
5. Review the complete repository when the working tree is clean.
6. Never discard, reset, stash, amend, rebase, push, or force-push changes.

Apply `.github/copilot-instructions.md` and matching path-specific instructions before reviewing or editing files.

## Engineering review

Perform everything required by `/review`, including:

- correctness, regressions, architecture, maintainability, and public contracts;
- C++17 and JUCE ownership, lifetime, thread safety, and numeric boundaries;
- audio transparency and real-time safety;
- host transport discontinuities, loops, meter changes, block boundaries, and missing context;
- cue-range invariants, overlap policy, ordering, text handling, and active-cue selection;
- bounded processor-to-editor communication and editor lifetime;
- versioned state, input limits, failure behavior, restoration, and compatibility;
- tests, build reproducibility, dependency provenance, licensing, and documentation;
- agreement between CMake, presets, compiler generation, architecture, Windows SDK, JUCE, CI, CodeQL, and written requirements.

## Security review

Perform everything required by `/sec-review`, including a concise threat model for:

- malformed or oversized project state and hostile Unicode lyric text;
- invalid positions, overflow, NaN, infinity, excessive cue counts, and denial of service;
- unsafe state transfer, data races, memory corruption, lifetime defects, and audio-thread violations;
- untrusted paths and file races if import or export exists;
- dependency, build-chain, workflow, pull-request, and artifact compromise;
- secrets, credentials, copyrighted lyrics, provenance, JUCE licensing, and AGPL compliance.

Do not expose secret values. Run safe read-only security checks when available.

## Documentation synchronization

Review:

- all files below `docs/`, recursively, when that directory exists;
- every root-level `*.md` file;
- repository policy documents under `.github/` when their claims affect public workflows.

Exclude generated build output and the result file created by this command from content comparison. Include any pre-existing result file in Git-state reporting, but do not treat it as authoritative documentation.

Compare documentation with source, tests, CMake, presets, workflows, dependency versions, and actual repository status. Update documentation only when it is stale, contradictory, incomplete for implemented behavior, or claims unfinished behavior works.

Before changing documentation, present the proposed documentation file list and corrections and obtain explicit user approval. If approval is not available, complete the review report without modifying documentation.

Documentation changes must:

- remain in English and valid in GitHub and Obsidian;
- use relative repository links;
- preserve accurate historical changelog entries;
- avoid marketing language and unsupported guarantees;
- keep README, kickoff, build instructions, security policy, changelog, and workflows consistent.

Do not modify source code, tests, CMake files, workflows, or other non-documentation files in this workflow. Report code defects instead of fixing them. The only allowed changes are documentation corrections and the required result file.

## Verification

After documentation edits, run the narrowest relevant checks first, then broader checks when practical:

- `git diff --check`;
- Markdown and syntax linters;
- relevant Debug and Release builds and tests when documented commands or paths changed;
- Steinberg Validator when VST3 behavior is in scope;
- safe policy, secret, generated-file, binary, and oversized-file checks.

Do not claim a check passed unless it ran. Record omissions and reasons.

## Findings and output

Report only actionable findings ordered by P0 through P3 severity. For each finding include:

- severity and concise title;
- exact file and line or precise location;
- evidence and impact;
- relevant CWE for security findings where applicable;
- concrete correction.

Then include documentation reviewed and changed, open questions, assumptions, trust boundaries, checks performed and omitted, residual risks, and a concise status conclusion. If no findings exist, say so and identify remaining gaps.

## Required result artifact

Create or update exactly one root Markdown document using the local execution date:

`YYYY-MM-DD_review-project-results.md`

The file must contain the complete final report. Display the same report in chat. Do not create other result files, commits, or pushes. End with final Git status and every file modified or created by this workflow.
