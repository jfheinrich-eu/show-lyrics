---
name: create-pr
description: Create a Show Lyrics pull request using the repository template and verified project conventions.
disable-model-invocation: true
---
Create a pull request for the current Show Lyrics branch. Do not make unrelated project changes.

## 1. Preflight and scope

1. Resolve the repository root and confirm that the current directory is inside it.
2. Read `.github/copilot-instructions.md`, `CONTRIBUTING.md`, `.github/PULL_REQUEST_TEMPLATE.md`, and matching path-specific instructions.
3. Confirm that `HEAD` is attached to a feature branch and that no merge, rebase, cherry-pick, revert, or bisect is in progress. Do not create a branch automatically.
4. Inspect `git status --short --untracked-files=all`, `git diff`, and `git diff --cached`. A pull request contains committed changes only. If local changes exist, do not stage, commit, stash, discard, or silently omit them. Explain what lies outside the PR and ask whether to continue with the committed scope or stop.
5. Determine the base branch from explicit direction, branch configuration, and the repository default. Do not guess a non-default target.
6. Compare the branch with the base and inspect every commit and the complete diff.
7. Check whether an open pull request already exists for the head branch. Do not create a duplicate. Ask before updating an existing PR.
8. Review a few recent relevant merged Show Lyrics pull requests when authenticated access exists. If the repository has no suitable history, say so. Do not substitute another repository's pull-request claims for Show Lyrics evidence.
9. Link issues only from explicit context, branch or commit references, or confirmed repository issues. Never invent issue numbers or speculative `Closes` or `Fixes` statements.
10. Check for unrelated changes, generated output, secrets, personal or machine-specific data, accidental binaries, and copyrighted lyrics. Stop on suspicious content without exposing secret values.

## 2. Verify the change

1. Select focused checks from the actual diff and this repository's documented commands.
2. Run safe relevant checks when feasible, including `git diff --check`.
3. For C++ or CMake changes, run the relevant Debug and Release builds and tests.
4. For VST3 changes, run Steinberg Validator when available.
5. For documentation or policy changes, run the Markdown and syntax checks that are available.
6. State exactly which checks ran, their results, and which checks did not run.
7. If required checks fail or remain materially incomplete, ask whether to create a draft or wait.

## 3. Prepare title and body

1. Use an English Conventional Commit-style title matching the branch's main purpose. Do not add a trailing period.
2. Read `.github/PULL_REQUEST_TEMPLATE.md` immediately before preparing the body.
3. Preserve every template heading and checklist item in the original order.
4. Complete the sections accurately:
   - **Summary:** Explain the problem and focused solution.
   - **Verification:** List exact commands, results, and omissions.
   - **Risks:** Cover actual compatibility, real-time, state-restoration, security, licensing, and rollout risks.
   - **Checklist:** Check an item only when supported by evidence.
5. Do not claim Cubase, validator, CI, review, or licensing results that were not verified for the current branch.
6. If the template is missing or unreadable, stop instead of inventing a replacement.

## 4. Create and verify

1. Prefer the authenticated GitHub CLI for this repository.
2. Use `gh pr create --title ... --body-file ...` with a temporary body file outside the repository. Remove only that exact temporary file after the command finishes.
3. If the CLI is unavailable, use an authenticated GitHub integration with the same title and template-derived body. Never create a second PR through another client.
4. Confirm repository, head branch, and base branch before creating.
5. If the branch is not pushed, confirm the intended remote and obtain explicit approval before pushing. Never force-push.
6. After creation, retrieve and verify URL, title, head, base, and body. Report any mismatch.
7. Do not approve, merge, amend, alter staging, or include unrelated branches.

## Finish

Report the URL, title, base and head branches, checks performed and omitted, remaining CI and review steps, and any local changes outside the PR. If creation stops, state exactly where and why.
