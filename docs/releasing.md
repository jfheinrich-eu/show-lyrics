# Releasing

## Preconditions

Before publishing a release:

1. Update the project version in `CMakeLists.txt`.
2. Update `CHANGELOG.md`.
3. Complete Debug and Release builds and tests.
4. Run Steinberg Validator against the Release VST3.
5. Build and verify the Windows installer locally.
6. Confirm AGPL corresponding-source and third-party notice completeness.
7. Create and push an annotated version tag matching `v<project-version>`.

## Publish

Create a GitHub release for the version tag. The `Release package` workflow runs when the release is published. It:

1. checks out the exact release tag;
2. verifies that the tag matches the CMake project version;
3. downloads the pinned NSIS 3.13 archive and verifies its SHA-256 hash;
4. builds and tests the Windows installer;
5. attaches the installer and checksum to the GitHub release.

GitHub displays these files as release assets. Show Lyrics does not publish an artificial NuGet or container package solely to appear in the GitHub Packages registry.

## Failure behavior

A version mismatch, dependency hash mismatch, packaging failure, installer verification failure, or asset upload failure stops the workflow. Do not replace a failed release asset with an unverified local binary.
