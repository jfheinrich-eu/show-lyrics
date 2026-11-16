# Show Lyrics Copilot Instructions

## Communication

- Write source code, comments, identifiers, documentation, issues, pull requests, and commits in English.
- Use concise technical language. Do not invent completed work, test results, or product requirements.
- Keep Markdown valid in GitHub and Obsidian.

## Product boundary

- Show Lyrics is a native C++17 and JUCE 9.0.2 VST3 plug-in for Cubase Elements 15 on Windows x64.
- The plug-in displays lyrics associated with musical timeline ranges.
- VST3 does not expose Cubase marker-track data. Do not claim that the plug-in reads or changes native Cubase markers.
- Keep the plug-in audio-transparent.
- Do not add cloud, account, analytics, browser, or network dependencies to core operation.

## Engineering rules

- Keep the audio callback allocation-free, lock-free, exception-free, and free from file, network, logging, and UI work.
- Transfer transport-derived state to the UI through bounded lock-free data or atomics.
- Treat project state and imported lyrics as untrusted input. Validate sizes, positions, encoding, and structure before applying state.
- Handle play, stop, seek, loop, backward jumps, meter changes, and missing host position explicitly.
- Use half-open cue ranges: start included, end excluded.
- Add focused tests for every behavior change.
- Never commit generated builds, credentials, tokens, personal data, or copyrighted lyrics.

## Git

- Use imperative English Conventional Commits.
- Keep commits atomic and add a DCO sign-off.
- Do not push, rewrite history, or discard user changes without explicit approval.

## Repository skills

The manual-only skills under `.github/skills/` define the approved workflows:

- `/review` performs an engineering review without modifying files.
- `/sec-review` performs a security review without modifying files.
- `/review-project` combines both reviews and synchronizes documentation only after explicit approval.
- `/commit` prepares atomic commit groups and commits only individually approved groups.
- `/create-pr` creates a template-compliant pull request from verified committed changes.
