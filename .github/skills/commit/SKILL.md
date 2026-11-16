---
name: commit
description: Prepare reviewed atomic Conventional Commit groups and create only commits explicitly approved by the user.
disable-model-invocation: true
---
Prepare and create reviewed atomic commits for the current repository. Follow all repository instructions. Do not modify project files while running this command.

## Safety rules

- Run only inside the current Git repository and resolve its root before inspecting files.
- Stop if a merge, rebase, cherry-pick, revert, or bisect is in progress, or if `HEAD` is detached. Explain the state without changing it.
- Never discard or overwrite working-tree changes. Never stash, amend, rebase, reset, clean, push, or force-push.
- Preserve existing staging unless the user explicitly authorizes non-destructive regrouping.
- Never use `git add .`, an unrestricted `git add -A`, or another command that can stage paths outside the approved group.
- Never bypass hooks or checks with `--no-verify`.
- Do not expose secret values. If credentials, keys, tokens, personal data, generated output, copyrighted lyrics, or suspicious binaries may be included, stop before staging and report only the affected paths and risk.
- Do not commit until the user has approved the exact group, file list, and complete message.
- If the interface cannot pause for interactive decisions, stop without changing the index or creating commits.

## 1. Inspect repository state

1. Run `git status --porcelain=v1 -z --untracked-files=all`, `git diff`, and `git diff --cached`.
2. Use null-delimited Git output or equivalent argument-safe handling so spaces and special characters remain intact.
3. Detect staged, unstaged, partially staged, renamed, deleted, and untracked files.
4. Read relevant contents and diffs before proposing groups.
5. Run safe secret, generated-file, oversized-file, binary, and repository-policy checks before staging.

If changes are already staged, show their exact paths and ask the user to choose one action:

- preserve the existing index as one indivisible first commit group; or
- explicitly authorize non-destructive unstaging so the files can be regrouped.

Do not alter existing staging without that authorization. If regrouping is approved, unstage only the exact approved paths with `git restore --staged -- <exact-file-paths>`. If a file is partially staged, explain that the one-file-one-group rule requires the complete file to belong to one group and ask how to proceed.

## 2. Select untracked files interactively

1. Obtain every untracked, non-ignored file with `git ls-files --others --exclude-standard -z`.
2. Present a numbered list with repository-relative path, file type, and size. Do not print contents that may contain secrets or copyrighted lyrics.
3. Ask the user to select individual numbers, `all`, or `none`.
4. Confirm the resolved selection once. Exclude every unselected file from all groups and staging commands.
5. If no untracked files exist, continue without asking an unnecessary question.

## 3. Build atomic groups

The candidate set consists of tracked changes plus only the selected untracked files.

1. Group files by one coherent purpose, such as product behavior, tests, documentation, build configuration, CI, or repository policy.
2. Order groups so each intermediate commit is understandable and buildable where practical.
3. Assign every candidate file to exactly one group.
4. Do not use hunk-level splitting to bypass the one-file-one-group rule.
5. If one file contains unrelated concerns, stop and ask the user to split it manually or explicitly assign the complete file to one group.
6. Keep tests with the behavior they verify when the one-file rule permits it.
7. Do not create an empty or catch-all commit.

For every group, prepare:

- a short name and rationale;
- the exact repository-relative file list;
- a concise diff summary and risk assessment;
- relevant checks;
- one English Conventional Commit message.

Use this structure:

```text
<type>[optional scope][!]: <imperative description>

[optional body]

[optional footers]
```

Use lowercase types such as `feat`, `fix`, `docs`, `test`, `build`, `ci`, `refactor`, `perf`, `style`, `chore`, and `revert`. Omit a trailing period from the subject. For an incompatible change, use `!` and a `BREAKING CHANGE:` footer.

Do not include a `Signed-off-by` trailer in a proposed message when the commit will use `git commit -s`; Git adds the configured committer's sign-off.

## 4. Obtain approval and commit

Process one group at a time. Show its rationale, exact files, complete message, risks, and checks. Ask the user to approve or reject it.

- On approval, stage only the exact paths with argument-safe handling.
- On rejection, offer message revision or skipping the complete group.
- If the file list changes, recompute group uniqueness and show the revised group again.
- Before committing, compare staged paths with the approved list and show `git diff --cached --stat` plus a staged diff summary.
- Run `git diff --cached --check` and the relevant focused tests. Run broader tests when risk or repository instructions require them.
- Inspect the message input for duplicate `Signed-off-by` trailers before using `git commit -s`.
- If a check or hook fails, keep the state, report the failure, and ask how to proceed.
- After each commit, verify the hash, subject, exact changed paths, and one correct sign-off matching `git config user.name` and `git config user.email`.

Refresh repository status after every commit because hooks may change files. Treat newly created or modified files as new work requiring review.

## 5. Finish

Report created hashes and subjects, skipped groups, remaining staged or unstaged changes, excluded untracked files, and checks performed. Do not push.
