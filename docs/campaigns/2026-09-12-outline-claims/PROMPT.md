# Gameplay-tag outline claims

Implement first-class, source-owned entity outline claims in CkFoundation and use them for the
Selection overlay and BusterBlock gameplay.

## Stable product contract

- Callers claim an `Outline.*` semantic tag; numeric priorities and caller-selected presets are not
  part of the public API.
- Project settings map each configured `Outline.*` tag to one `Layer.*` tag and one outline preset,
  and define layer precedence highest-first.
- Claims are keyed by exact target, source `FCk_Handle`, and outline tag. Identical valid sets are
  idempotent. Clears release only the exact owned claim.
- Invalid handles, tags, scopes, definitions, layers, presets, duplicate rows, and unowned clears
  diagnose with `CK_ENSURE_IF_NOT` and fail without mutation. Validation control flow must remain
  effective when ensure diagnostics are compiled out.
- Configuration validation is atomic: no partially valid table may be used.
- One deterministic resolved outline is published for renderer consumers. Same-layer definitions
  must use the same preset; deterministic source/tag ordering breaks co-owner ties.
- `EntityAndDependents` is live over the current lifetime-owner chain. Late children and reparented
  entities update without restamping descendants; explicit descendant claims arbitrate normally.
- Actor, ISM, and ISKM consumers use only the resolved fragment and retain their renderer-owned
  applied-state teardown.
- Component removal restores prior custom-depth state only while the outline system still owns the
  values it wrote.
- Selection outlines the focused entity and live subtree by default, is user-toggleable in the
  shared Shift+P / ECS overlay settings panel, and releases on focus loss, overlay deactivation,
  teardown, or invalid selection.
- BusterBlock migrates its local outline-source arbitration to this contract. Backward compatibility
  with the old single-target API is not required.

## Repositories

- Upstream implementation host: `D:\Repos\CkPlugins`
- Downstream migration only: `E:\Repos\BusterBlock_Other`
- Do not modify `D:\Repos\BusterBlock`.

## Guardrails

- Preserve unrelated dirty files and stage/commit nothing unless separately requested.
- Do not create worktrees, clones, or duplicate checkouts.
- Build and test only through `CkAuto/UnrealToolbox.exe`.
- Existing CkUsf baseline: `Ck.Usf` 37/37 passed before implementation.
