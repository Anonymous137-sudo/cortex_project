# Cortex Project v0.1.2 Release Notes

These notes describe the `v0.1.2` school build dated July 16, 2026.

This release focuses on two practical demo issues:

- cross-machine node convergence when two peers sit at the same height on different tips
- desktop startup recovery when the saved backend executable path points to an old deleted build

## What Changed

- Fixed equal-height fork recovery so peers now request headers and reconcile instead of idling on competing tips
- Improved peer height tracking so sync state reflects remote progress more accurately during catch-up
- Updated block handling so accepted side-branch blocks trigger follow-up header sync when the active tip did not actually advance
- Added backend path recovery in the GUI so a stale saved path falls back to the bundled daemon automatically
- Added focused regressions for equal-height fork resolution and missing-block active-tail repair

## Why This Matters For The School Demo

- A Windows machine and a macOS test machine running the same build are less likely to drift onto separate local tips during demo sync tests
- The desktop app is more resilient after moving folders, rebuilding locally, or launching a packaged bundle after using an older development build
- Validation is easier because the test harness now supports targeted test filtering

## Validation

The release validation for this update focused on:

- equal-height fork resolution
- late-node catch-up sync
- missing-block tail repair
- packaged desktop startup using bundled backend binaries

## Release Assets

This school-facing repository still ships the Windows runtime bundle used by school lab systems, and may also include macOS test assets when they are rebuilt during release staging.

## Upgrade Note

- Replace older bundles with the `v0.1.2` assets before testing node-to-node sync
- If a previous run saved a backend path to a deleted build folder, launch the new bundle once and let it rewrite the path automatically
