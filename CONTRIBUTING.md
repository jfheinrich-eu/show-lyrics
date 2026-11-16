# Contributing

## Development setup

Use the Windows toolchain documented in [README.md](README.md). Keep generated files below `build/`.

## Rules

- Write code, comments, documentation, issues, pull requests, and commits in English.
- Keep Markdown valid in GitHub and Obsidian.
- Keep changes focused and add tests for changed behavior.
- Do not add copyrighted lyrics, credentials, personal data, generated builds, or machine-specific paths.
- Do not perform allocation, locking, file access, network access, logging, or UI work in the audio callback.
- Treat imported text and restored project state as untrusted input.
- Run `git diff --check`, Debug tests, and Release tests before requesting review.

## Commits

Use Conventional Commits 1.0.0 with an imperative English subject. Add a DCO sign-off with `git commit -s`. Keep each commit limited to one coherent purpose.

Do not rewrite history, bypass hooks, or push changes without explicit authorization.

## Pull requests

Explain the behavior, risk, and exact verification. Large architecture, state-format, licensing, or immutable-identity changes require a decision before implementation.

By contributing, you agree that your contribution is licensed under `AGPL-3.0-only` and certify it through the DCO sign-off.

## Repository skills

The repository provides manual-only Copilot skills under `.github/skills/`:

- `/review` for engineering review;
- `/sec-review` for security review;
- `/review-project` for combined review and approved documentation synchronization;
- `/commit` for the interactive atomic commit workflow;
- `/create-pr` for verified pull-request creation.
