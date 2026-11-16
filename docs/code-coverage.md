# Code coverage

## Scope

The Coverage workflow collects native MSVC line coverage for `ShowLyricsDomainTests` and the linked lyric-cue domain implementation. It normalizes source paths before uploading a Cobertura report to Codecov.

Coverage excludes generated build output, documentation, and test source. It does not currently measure the JUCE plug-in wrapper or editor.

## Policy

Project and patch coverage statuses are informational. No blocking percentage is configured because the pre-alpha repository has no stable baseline. Tests must retain behavioral value; do not weaken assertions or add meaningless execution solely to raise a percentage.

## Authentication

The public repository uses Codecov OpenID Connect through GitHub Actions. The workflow requests `id-token: write` only for the upload job and does not require a repository upload token.

## Artifacts

The intermediate Cobertura report is retained as a private workflow artifact for one day. The normalized report contains repository-relative source paths only.
