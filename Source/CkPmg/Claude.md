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

## Text Shapes (UTF-8)

Arbitrary live UTF-8 strings — including CJK — rendered as debug geometry in both tiers: wireframe glyph contours (line segments) and a filled triangulated procmesh. Glyph outlines come from FreeType; tessellation uses `FConstrainedDelaunay2d` (NonZero winding). A module glyph cache keyed by `(face, codepoint)` in EM units makes live text cheap — glyph extraction and tessellation run once per unique glyph, not per entity tick.

**API (three tiers, same pattern as the other families):**

| Call | Output |
|---|---|
| `UCk_Utils_Pmg_TextShapes::Add_Text(InHandle, Transform, Text, Size, Color, DrawLines, DrawFilled, LineThickness, Align, Axis, FontOverride, Duration)` | Filled procmesh + wireframe on `InHandle`. |
| `UCk_Utils_Pmg_TextShapes::Create_Text(InOwningEntity, ...)` | Same, but spawns a new child entity owned by `InOwningEntity`. |
| `UCk_Utils_Pmg_TextShapes::DrawText(WCO, Center, ...)` | Fire-and-forget (DevelopmentOnly). Spawns a one-off entity. |

`InDrawLines` + `InDrawFilled` select wireframe-only / filled-only / both (default both).

**Live mutation:** `UCk_Utils_Pmg_DebugShape_UE::Request_SetText(handle, NewText)` sets new text and re-arms `FTag_Pmg_DebugShape_NeedsSetup`; the Setup processor rebuilds geometry next tick. Thanks to the glyph cache, only genuinely new codepoints are re-tessellated.

**Shape type:** `ECk_Pmg_DebugShape_Type::Text`.

**Axis default is XZ (upright)** — deliberately different from symbol/icon families' XY default, because flat text reads edge-on and is invisible to a normal camera. For full readability from an arbitrary camera the caller must billboard the entity transform; no auto-billboard is applied.

**Font:** default resolves to **Noto Emoji** + **Noto Sans Symbols 2** as raw `.ttf` files under `Source/CkPmg/Resources/` (staged for packaged builds), applying per-glyph font fallback: primary text font → emoji → symbols. Mixed strings like `"Wave 3 ⚠"` work out of the box. Emoji/symbols render as single-color silhouettes. The primary font falls back to the engine's Roboto TTF (Latin only) unless a `UFontFace*` override is passed. CJK still needs a CJK primary font supplied via `FontOverride`.

**Server:** FreeType is not compiled for dedicated server (`CK_PMG_WITH_FREETYPE=0`); text entities are inert there.

---

## Implementation notes

**Wireframe is BAKED, not re-emitted.** `ck::pmg::Append_Debug*_World` caches entity-local
segments on `FFragment_Pmg_DebugShape_Lines` and stamps the one-shot gate
`FTag_Pmg_DebugShape_LinesNeedBaking`; `FProcessor_Pmg_DebugShape_BakeLines` then triangulates
each segment as a stretched box and writes them as **mesh section 1** on the entity's procmesh
(section 0 is the filled shape). This replaced a per-tick `FProcessor_Pmg_DebugShape_DrawLines`
that re-emitted every line every frame through UE's `TransientLineBatchComponent` and tanked
perf in scenes with many wireframe shapes — **any doc or comment still describing a per-tick
DrawLines processor is stale.** Detail worth knowing before touching the bake:

- Section 1 gets its **own** `UMaterialInstanceDynamic`, spawned lazily off slot 0's parent
  material with alpha forced to 1. Sharing slot 0's MID makes the outline inherit the fill's
  alpha and visually camouflage against it. `Request_SetColor` mirrors RGB to both MIDs.
- Line geometry is a stretched box (8 verts / 12 tris, `Thickness × Thickness` cross-section),
  not a flat quad: the box extends `Thickness/2` in both perpendicular directions at each
  endpoint, so adjacent segments overlap at the join and corner gaps close.
- Each box is biased outward from the entity local origin by `Thickness/2` so it does not sit
  embedded in the filled section-0 mesh and Z-fight it. All basic PMG shapes are centred on that
  origin, which is what makes the midpoint direction a usable "outward".
- Section 1's visibility is set from `Common._RenderMode` at the end of the bake, so an entity
  hidden before any lines were appended does not start showing wireframes when the bake runs.
- **Known gap:** `Request_SetLineThickness` updates the cached `Common._LineThickness` and
  re-stamps the bake gate, but the bake reads the per-line thickness written into each
  `FCk_Pmg_DebugLine` by the `Append_*` call. The uniform override therefore does not take
  effect until the lines are re-appended.

**Setup runs in its own scheduler group.** `FGroup_Pmg_DebugShape_Setup` (`CkPmg_ProcessorGroups.h`)
runs every per-shape Setup ahead of `FGroup_Gameplay_Rendering`. Without that barrier,
`FProcessor_Pmg_DebugShape_UpdateTransform` (and the duration/lifetime processors) can iterate an
entity whose `FTag_Pmg_DebugShape_NeedsSetup` a Setup has already stripped but whose mesh
component that Setup has not yet assigned.

**Circle wireframes must compose the ENTITY rotation.**
`Append_DebugCircle_PlaneAxis_World` applies `EntityRot * AxisRot`, matching the hand-authored
wireframes' `FinalRotation = Rotation * AxisRotation`. It once applied the plane-axis rotation
only — identical for an identity entity rotation (the world→local round trip divides the entity
transform back out either way), but on a **rotated** entity it left the baked wireframe in the
wrong plane while the filled mesh, which gets the entity rotation via the procmesh
`SetWorldTransform`, rotated correctly.

**`Append_Debug*_World` are drop-in replacements for `UCk_Utils_DebugDraw_UE::DrawDebug*`** inside
Setup processors — same world-space endpoint math at the call site. The triangle and polygon
variants append the **outline only**: unlike the debug-draw originals there is no fill, because in
PMG the fill is the procedural mesh section.

**Cone apex orientation is baked into the mesh.** `ECk_Pmg_ConeOrientation` rotates vertices and
normals *before* the per-shape `ECk_Plane_Axis` rotation, so callers no longer need the
"Pitch=-90 in the SceneNode local rotation" workaround that gym agents and crowd debug used to
repeat for an apex-forward cone. `Up` (apex +Z, base on XY) is the default and is unchanged.

**Editor click-selection is opt-in.** `FTag_Pmg_EditorSelectionHandle`, stamped via
`Request_ActAsEditorSelectionHandle`, hosts the shape's mesh component on the per-owner
selection-proxy actor (`ck::FFragment_EditorSelectionOwner`) in editor previews, so a viewport
click on the shape selects the placed actor that owns the preview. Composite shapes honor a tag
stamped anywhere up the lifetime chain. Without the tag the component is owner-less and therefore
click-through — debug overlays never eat viewport clicks.

**Live mutation requests are shape-agnostic.** Every variant (basic / angular / directional /
icon / symbol) shares `FFragment_Pmg_DebugShape_Common`, so the `UCk_Utils_Pmg_DebugShape_UE::Request_*`
family works uniformly against `FCk_Handle_Pmg_DebugShape`.

**Icon shapes are composites of flat shapes**, each Setup spawning child entities via
`UCk_Utils_Pmg_FlatShapes` rather than generating its own procmesh — which is what earns them
`FTag_Pmg_DebugShape_Composite`. What each one draws:

| Icon | Composition |
|---|---|
| `Warning` | Triangle + exclamation bar + dot. Its bar/dot ratios are keyed off the Triangle shape's own vertex centring (top `0.211 × Size`, bottom `-0.539 × Size`), so re-centring Triangle moves them. |
| `Prohibition` | Circle + two diagonal X bars |
| `NoEntry` | Circle + one horizontal bar |
| `InfoCircle` | Circle + "i" (dot on top, bar on bottom) |

---

## See also

- `CkVfx/Claude.md` — Niagara-based VFX on entities.
- `CkIsmRenderer/Claude.md` — high-count static instances.
- `CkDebuggerCommon/CLAUDE.md` — debugger-side guidance for in-world PMG overlays.
