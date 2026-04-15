# CkPmg

**Purpose:** Procedural mesh generation — donut/ring and other procedural shape meshes generated at runtime and attached to entities as procedural mesh components.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Ability VFX, AoE indicators, procedural geometry.

---

## Key API

- `UCk_Utils_Pmg_Donut_UE::Add(InHandle, InParams)` — generate a donut/ring mesh attached to entity.
- Standard Has, Cast helpers.

---

## Pattern

Add a PMG entity as a child of the ability/AoE entity; processors update mesh params (inner radius, outer radius, segments) each tick.

---

## Anti-patterns

Don't regenerate procedural meshes every frame when only transform changes — separate mesh topology from transform updates.

---

## See also

- `CkVfx/Claude.md` — for Niagara-based VFX on entities.
- `CkIsmRenderer/Claude.md` — for high-count static instances.
