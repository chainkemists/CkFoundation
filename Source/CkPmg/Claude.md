# CkPmg

**Purpose:** Procedural mesh generation — runtime-built debug shapes (capsule, sphere, box, cone, cylinder, torus, hemisphere, pyramid + angular/directional/icon/symbol variants) with optional matching wireframe overlay. Attached to entities as `UProceduralMeshComponent` instances.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Ability VFX, AoE indicators, in-world debugger overlays, procedural geometry.

---

## Three distinct API tiers — pick the right one

Mixing these up is a recurring footgun. They produce different output:

| API | Output | When to use |
|---|---|---|
| `ck::pmg::Append_Debug*_World(InHandle, ...)` | **Wireframe lines only.** No procmesh. `FProcessor_Pmg_DebugShape_DrawLines` re-emits each segment via `UCk_Utils_DebugDraw_UE::DrawDebugLine` every tick. | Inherently line-shaped overlays — paths, polylines, drop indicators. Not for "filled" or "real" shapes. |
| `UCk_Utils_Pmg_BasicShapes::Add_*(InHandle, FTransform, ...)` | **Filled procmesh + auto-wireframe** when `InDrawLines=true`. Setup processor builds the mesh AND internally calls the matching `Append_Debug*_World` for the outline. Adds Common + Params + Current + NeedsSetup tag + Transform to `InHandle`. | Default for "I want a debug shape attached to this entity." The entity *becomes* the shape. |
| `UCk_Utils_Pmg_BasicShapes::Create_*(InOwningEntity, FTransform, ...)` | Same as `Add_*`, but spawns a **new child entity** owned by `InOwningEntity` and applies the shape there. | When the shape should sit alongside other geometry on a parent overlay entity, and should cascade-destroy when the owner does. |
| `UCk_Utils_Pmg_BasicShapes::DrawFilled*(WorldContext, FVector, ..., Duration)` | Fire-and-forget. Spawns a one-off entity in the world. | Single-shot debug calls (one tick or short duration). Not for live tracking. |

**Rule:** `Append_Debug*_World` does not draw a "shape" — it draws line segments. If you want a filled capsule on an entity, you want `Add_Capsule` (or `Create_Capsule`), not `Append_DebugCapsule_World`.

---

## Duration sentinel (footgun)

`InDuration` defaults to `0.0f` on `Add_*` / `Create_*` / `DrawFilled*`. **`0` does not mean "no auto-destroy"** — it means "destroy on the first tick after spawn". `FProcessor_Pmg_DebugShape_CheckDuration` early-outs only when `Duration < 0`:

| Value | Behavior |
|---|---|
| `> 0` | Auto-destroy after `Duration` seconds |
| `== 0` (default) | Auto-destroy on the first tick |
| `< 0` (e.g. `-1.0f`) | Persist until explicit `Request_DestroyEntity` |

For live-tracking overlays (e.g. a debugger capsule following a selected entity), pass `-1.0f`. Defaulting to 0 was burned twice in the nav-debugger session — a "single frame and gone" is rarely the actual intent for shape calls.

---

## Fragment architecture

| Fragment / tag | Role | Added by |
|---|---|---|
| `FFragment_Pmg_DebugShape_Common` | Color, thickness, draw-lines toggle, render mode, duration. **Required** by every render path (Setup processors AND `DrawLines` processor). | `Add_*` calls; also `GetOrAddLinesFragment` so `Append_Debug*_World` works on debug-only entities. |
| `FFragment_Pmg_<Shape>_Params` | Per-shape geometry (radius, half-height, segments, axis…). | `Add_*` calls only. |
| `FFragment_Pmg_DebugShape_Lines` | Cached wireframe segments in entity-local space. | `Append_Debug*_World` (and indirectly by `Add_*` when `InDrawLines=true`). |
| `FFragment_Pmg_DebugShape_Current` | Owns the live `UProceduralMeshComponent`. | `Add_*` calls only. |
| `FFragment_Transform` | Required by every render path. | `Add_*` calls; otherwise call site. |
| `FTag_Pmg_DebugShape_NeedsSetup` | One-shot Setup gate; cleared after Setup runs. | `Add_*` calls. |

A line-only entity needs **Common + Lines + Transform**. A filled-shape entity needs **Common + Params + Current + Transform + NeedsSetup**.

The DrawLines processor matches on `Common + Lines + Transform`. Forgetting any of those = silent no-op (no error, no log — geometry just never appears). Historically `Append_Debug*_World` only added Lines; `Common` had to come from a shape Setup. That's why it now AddOrGets Common in `GetOrAddLinesFragment` — to make line-only consumers work without the caller having to know.

---

## Pattern: live-tracking overlay

```cpp
// Once on selection / spawn:
auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(World);
UCk_Utils_Pmg_BasicShapes::Add_Capsule(
    Entity, FTransform{Center}, Radius, HalfHeight, Segments, Rings,
    Axis, Color, /*InDrawLines=*/true, Thickness, /*InDuration=*/-1.0f);
auto TransformHandle = UCk_Utils_Transform_UE::Cast(Entity);

// Each tick:
UCk_Utils_Transform_UE::Request_SetTransform(
    TransformHandle, FCk_Request_Transform_SetTransform{FTransform{NewCenter}});
```

Setup runs once (gated by `FTag_Pmg_DebugShape_NeedsSetup`); per-tick transform updates flow through `FProcessor_Pmg_DebugShape_UpdateTransform`. **Don't call `Add_Capsule` more than once per entity** — it adds Common/Params/Current with `Add<>` (not `AddOrGet<>`) and will ENSURE on duplicate add.

For multi-shape overlays (e.g. capsule + marker spheres), use `Create_*(ParentEntity, ...)` to spawn each as a child entity. Destroying the parent cascade-destroys the children.

---

## Anti-patterns

- **`Append_DebugCapsule_World` expecting a filled capsule.** It draws wireframe segments only. Use `Add_Capsule` / `Create_Capsule`.
- **Leaving `InDuration = 0.0f` on a persistent overlay.** Use `-1.0f`.
- **Adding two shape Params on one entity.** Each entity hosts a single procmesh. For multiple shapes on one logical overlay, use `Create_*` to spawn child entities.
- **Manually `AddOrGet`-ing PMG fragments from client code.** Use the Utils API. `GetOrAddLinesFragment` and the typed `Add_*` calls handle fragment composition.
- **Regenerating procedural mesh topology every tick when only transform changes.** Setup runs once; transform updates ride through `UpdateTransform`.

---

## See also

- `CkVfx/Claude.md` — Niagara-based VFX on entities.
- `CkIsmRenderer/Claude.md` — high-count static instances.
- `CkDebuggerCommon/CLAUDE.md` — debugger-side guidance for in-world PMG overlays.
