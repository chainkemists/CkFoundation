# Progress

Status: Implementation complete; automated gates green; manual PIE visual acceptance pending.

## Completed

- Added gameplay-tag-driven outline definitions and layer priority, with `Outline.*` and `Layer.*`
  validation, atomic configuration rejection, and loud `CK_ENSURE_IF_NOT` diagnostics.
- Added source-owned entity outline claims with deterministic arbitration and live subtree tracking.
- Added actor, primitive-component, static-mesh, skeletal-mesh, ISM, and ISKM consumers with
  physical-component ownership aliasing and state restoration after the final owner releases.
- Integrated Selection overlay outlining and its shared settings layout.
- Migrated BusterBlock presentation and gameplay call sites in `E:\Repos\BusterBlock_Other` to
  semantic debugger, emphasis, interaction, and guidance outline tags.
- Removed the public imperative physical-outline API and rejected forged resolved state before it
  can reach renderer mutation.

## Evidence

- Pre-change `Ck.Usf` baseline: 37 passed, 0 failed, 0 skipped, 0 contaminated.
- Baseline log: `D:\Repos\CkPlugins\Saved\Logs\Codex-OutlineClaims-Baseline.log`.
- CkPlugins incremental Development editor build succeeded:
  `D:\Repos\CkPlugins\Saved\Logs\Codex-OutlineClaims-Contract-PlainMatcherBuild.log`.
- Final CkPlugins physical-ownership/negative-path contract test: 1 passed, 0 failed, 0 skipped,
  0 contaminated:
  `D:\Repos\CkPlugins\Saved\Logs\Codex-OutlineClaims-PhysicalOwnership-Final.log`.
- The compatible Outline siblings passed in both preceding 16/17 runs; the sole failure in those
  runs was the expected-error matcher treating literal `[-1]` as a regular expression. The test now
  uses the plain-text matcher proven by the final focused run.
- BusterBlock_Other incremental Development editor build succeeded:
  `E:\Repos\BusterBlock_Other\Saved\Logs\Codex-OutlineClaims-BusterBlock-ContractBuild.log`.
- Fresh BusterBlock_Other `Ck.Usf.Outline` run: 15 passed, 0 failed, 0 skipped, 0 contaminated:
  `E:\Repos\BusterBlock_Other\Saved\Logs\Codex-OutlineClaims-BusterBlock-ContractTests.log`.
  The first discovery instance requested the expected full reload after generated script discovery;
  UnrealToolbox restarted it internally and the resulting run completed green.
- Final `git diff --check` was clean across the touched CkPlugins and BusterBlock_Other repositories.
- No references to the removed legacy public physical-outline API remain.

## Current ownership

- CkFoundation: `feature/outline-claims`.
- CkTests: `feature/outline-claims`.
- CkGameplayDebugger remains on `feature/selection-debugger`.
- BusterBlock migration target is `E:\Repos\BusterBlock_Other`; its user-updated CkFoundation and
  CkTests heads were preserved, and the pre-existing `Config/DefaultGameplayTags.ini` edit was not
  touched.

## Remaining

- In manual PIE, visually verify Selection outlines for static meshes, skeletal meshes, ISMs, and
  ISKMs, plus BusterBlock debugger/emphasis/interaction/guidance arbitration and restoration.
- No commits or pushes have been made.
