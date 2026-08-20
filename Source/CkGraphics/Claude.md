# CkGraphics

**Purpose:** Graphics utilities — rendering helpers beyond ISM, including material instance manipulation and GPU query helpers. Thin utility module; not ECS-Record-patterned.

Render targets are NOT here despite what this line used to claim: replicated render-target pixels and draw calls live in `CkRenderTarget`, and this module has no render-target API at all.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkVariables`.
**Used by:** `CkIsmRenderer`, `CkOverlapBody`.

---

## Key API

- `UCk_Utils_Graphics_UE` — dynamic material instance creation, render target ops.

---

## Pattern

Used by modules that need to dynamically change material parameters without a full actor.

---

## Anti-patterns

Don't set material parameters per-frame from a Processor without caching the `UMaterialInstanceDynamic*` — re-creation is expensive.

---

## See also

- `CkIsmRenderer/Claude.md`, `CkVfx/Claude.md`.
