# CkIsmRenderer

**Purpose:** Instanced Static Mesh (ISM) rendering via ECS. ISM proxy entities manage ISM instances and update transforms/params from ECS fragments without going through UE's actor system.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkGraphics`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkUsf`.
**Used by:** Procedural foliage, projectile trails, crowd rendering — any feature that needs many instances without per-actor overhead.

---

## Key API

- `UCk_Utils_IsmProxy_UE` — add ISM proxy to entity, update transform batch, set custom data.
- `UCk_Utils_IsmProxy_UE::Request_SetCustomInstanceDataValue_Late` — opt-in per-instance value lane
  consumed in `FGroup_DeferredApply` after `FGroup_PostTransform`; the normal request lane is unchanged.
- `FProcessor_IsmProxy_Setup` — creates the ISM component; `FProcessor_IsmProxy_Update` syncs transforms each tick.
- **Entity-level outline** (`FProcessor_IsmProxy_Outline_Sync/_TransformSync/_Suspend/_Remove/_EndPlay`) — driven by
  `CkUsf`'s `UCk_Utils_Usf_Outline_UE::Request_ApplyOutline(Handle, Preset, Scope)`. Custom Depth/Stencil is per-
  *component*, so a shared ISM can't express "outline just this instance" directly — outlined proxies' instances are
  mirrored into a custom-depth-only "shadow ISM", one per (renderer data, preset), created via
  `UCk_IsmRenderer_Subsystem_UE::FindOrCreate_OutlineIsmComponent`. Test getters:
  `UCk_Utils_IsmProxy_UE::Get_IsOutlineApplied` / `Get_OutlineShadowInstanceCount` /
  `Get_CustomInstanceData` / `Get_OutlineShadowCustomData` / `Get_OutlineShadowMaterial`.
- **The shadow ISM inherits the source's materials + `NumCustomDataFloats`, and its per-instance custom data is
  kept mirrored** (seeded by `_Outline_Sync`, mirrored on every write by `FProcessor_IsmProxy_HandleRequests`).
  This is what makes a **WPO-animated** ISM outline correctly: `CkVat` deforms its mesh entirely inside the
  material's World Position Offset, keyed off the per-instance custom data, so a shadow rendering the default
  material with zero custom-data floats would silhouette the *bind pose* while the mesh animates. The custom
  depth pass runs the full material vertex shader whenever the material modifies mesh position, and substitutes
  the position-only default material back in when it doesn't — so this costs ordinary (non-WPO) ISMs nothing.
  Consequence: a masked material now silhouettes *mask-accurately* rather than as full geometry.
  **Translucent-family source materials are deliberately NOT inherited**: the custom depth pass *drops* them
  (`UseDefaultMaterial` → `bIgnoreThisMaterial` → no draw) unless they opt into translucent custom-depth writes,
  which would silently delete the outline. Those slots keep the static mesh's own material, so a translucent
  look (e.g. `M_CkUsf_Look_Glass`) still silhouettes — at bind pose if it also animates via WPO, which beats
  no silhouette at all.

- **Entity-level cel pattern** (`FProcessor_IsmProxy_CelPattern_Sync/_TransformSync/_Suspend/
  _DropAppliedOnOutline/_Remove/_EndPlay`) — driven by `CkUsf`'s
  `UCk_Utils_Usf_CelPattern_UE::Request_SetCelPattern(Handle, Pattern, Scope)`. Structurally the outline
  processors' twin and it uses the same shadow-ISM mechanism, via
  `UCk_IsmRenderer_Subsystem_UE::FindOrCreate_CelPatternIsmComponent`. Two differences worth knowing: the
  shadow is keyed on the stencil VALUE rather than a preset (the cel contract is a direct value, so nothing
  is allocated and nothing is released — two patterns on one renderer are two shadows), and
  `_DropAppliedOnOutline` fully tears the cel shadow instance down rather than merely dropping the cache,
  because here the two features own SEPARATE components and leaving both alive would put two custom-depth
  writers on the same pixels. The two are mutually exclusive per entity; the outline wins. Test getters:
  `UCk_Utils_IsmProxy_UE::Get_IsCelPatternApplied` / `Get_CelPatternShadowInstanceCount` /
  `Get_CelPatternStencilValue`. Rationale: `CkUsf/Claude.md § Cel shade (Stylize)`.
- **Entity-level stylize effect mask** (`FProcessor_IsmProxy_StylizeMask_Sync/_TransformSync/_Suspend/
  _DropAppliedOnOutline/_DropAppliedOnCelPattern/_Remove/_EndPlay`) — driven by `CkUsf`'s
  `UCk_Utils_Usf_StylizeMask_UE::Request_AddToStylizeMask(Handle, Scope)`, via
  `UCk_IsmRenderer_Subsystem_UE::FindOrCreate_StylizeMaskIsmComponent`. The cel-pattern set again, with two
  simplifications and one addition: there is no per-entity payload (the project reserves ONE stencil value,
  so `_Sync` reads `UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue()` instead of consulting a
  subsystem), and the drop-on-higher-claim processor is TWO processors rather than one because a view is a
  conjunction while "a higher claim arrived" is a disjunction. Precedence is outline > cel pattern > mask,
  so this feature's Sync excludes BOTH. Rationale: `CkUsf/Claude.md § Effect mask (Stylize)`.

---

## Pattern

Create an IsmProxy entity per mesh type, not per instance. Each IsmProxy entity manages all instances of one mesh type. Instances are indexed, not entity-per-instance.

---

## Anti-patterns

1. Don't use `CkIsmRenderer` for meshes that need per-instance collision or interactability — those need actors.
2. Don't update ISM transforms outside the Setup/Update processors.
3. Don't touch a shadow ISM component directly — it's owned by the outline processors, which mirror instance
   add/move/remove automatically. To outline an entity, write `FFragment_Usf_OutlineTarget` via `CkUsf`'s
   `Request_ApplyOutline`, never the shadow ISM.

---

## Implementation notes

Rationale that used to live as comments in the source. Read this before "simplifying" any of it.

**Renderer actors are transient, and naming them must not dirty the level.**
`ACk_IsmRenderer_Actor_UE` is a runtime cache derived from `UCk_IsmRenderer_Data` and must never be
saved into the level package — without `RF_Transient` every editor open re-spawns one and dirties the
level, then a save bakes the duplicate in. `UCk_IsmRenderer_Subsystem_UE::DoSweepLeakedRenderers`
exists to clean up actors that a pre-`RF_Transient` version already baked in; it runs once on
subsystem init in both editor and runtime worlds, and dirties editor levels so the user can save a
clean copy. The cache is empty at that point, so later `GetOrCreate` calls spawn fresh replacements.
Renderers are identified via `SpawnInfo.Name` (`.Set_NonUniqueName`), never `.Set_Label()`:
`SetActorLabel` calls `AActor::Modify()` → `MarkPackageDirty()`, and in editor worlds the only package
is the persistent level, so every spawn would dirty it. `RF_Transient` is applied in the post-spawn
callback, too late to undo a dirty flag that already fired. `SpawnInfo.Name` is a construction-time
identifier, does not call `Modify()`, and surfaces as the actor's `FName`.

**One renderer per (data asset, selection owner) in editor worlds.** A viewport click on a preview
instance must select the placed actor whose preview it is; per-instance→actor mapping is impossible at
the actor level, so the renderer split IS the mapping. A shared renderer would make every preview mesh
select the same meaningless transient actor. Editor previews are small, so the lost batching is
irrelevant; runtime worlds always use the shared renderer. The per-owner caches key on
`TWeakObjectPtr`, which compares by object index + serial — lookups keep resolving the SAME component
after the owner actor is destroyed, so teardown removes instances from the component that actually
holds them instead of silently falling back to the shared one.

**Local rotation offset is post-multiplied, never pre-multiplied.**
`InTransform.GetRotation() * InParams.Get_LocalRotationOffset().Quaternion()` composes the offset in
the entity's LOCAL frame, like an Unreal relative transform (`childWorldRot = parentRot * relativeRot`).
Pre-multiplying conjugates any pitch/roll the entity picks up by the offset — a 180-degree yaw offset
would mirror the entity's rock relative to true child entities (e.g. SceneNode-parented doors). Every
site that builds an instance transform (`AddInstance`, `TransformInstance`, `EnsureStaticNotMoved_DEBUG`,
and the outline processors' `Get_TransformWithLocalOffset`) must compose identically or the shadow
instance drifts off the main one.

**`FProcessor_IsmProxy_Outline_TransformSync` is deliberately not keyed on `FTag_Transform_Updated`.**
That dirty marker is shared by writers in other modules (`CkRaySense`) that we cannot declare ordering
against without a dependency cycle — the scheduler's conflict advisory would fire on every world. It
runs per-frame over outlined MOVABLE proxies only (a tiny set), and `UpdateInstanceTransformById` on an
unchanged transform is cheap.

**`UCk_IsmRenderer_Data::_MeshSoft` vs `_Mesh`.** Authoring prefers the soft ref (`assets::Foo()`) over
an eager `assets::load::Foo()` into `_Mesh`: the blocking load behind `assets::load` ensures during cook
(AS-load runs before the engine is load-safe), nulling `_Mesh`. `Get_Mesh()` is hand-written rather than
`CK_PROPERTY_GET` so it resolves `_MeshSoft` lazily at first consumption (post-init) and caches the
result; every consumer routes through it, making resolution order-independent. If both are set, `_Mesh`
wins.

**Known limitations.**
- Per-instance custom data written after setup is only pushed to the component for `Movable` proxies
  (both `SetCustomInstanceData` request handlers gate on mobility). A `Static` proxy's post-setup write
  updates the CPU-side cache but never reaches the GPU.
- The late custom-data lane preserves that mobility contract: every valid value updates the CPU cache;
  movable proxies also update the main ISM and any active outline/cel-pattern shadow instance, while
  static proxies do not issue a GPU write. A disabled movable proxy, or one awaiting re-add, has no live
  instance, so its cache write still succeeds quietly and is pushed when the proxy is re-enabled. An
  otherwise-ready movable proxy still diagnoses a missing renderer component or instance after accepting
  the cache write. Its distinct request fragment is consumed after the complete `FGroup_PostTransform`,
  has the same guarded completion/request-handle cleanup as the general lane, and is cancelled with
  `Failed_Cancelled` during EndPlay if still pending.
  If both lanes target the same index in one frame, the late lane wins by group order.
- Pushing a previous-instance transform (`SetPreviousTransformById`) from `FProcessor_IsmProxy_TransformInstance`
  was tried for TSR dithering and appeared to do nothing while costing CPU; the call was removed. It also
  requires `SetHasPerInstancePrevTransform(true)`, which is not exposed as a renderer param.

---

## See also

- `CkGraphics/Claude.md` — lower-level graphics utilities.
- `CkEcs/Claude.md` — processor and fragment patterns.
- `CkUsf/Claude.md` — entity-level outline request API, plus its *Entity outlines* section (the outline
  architecture across ISM/ISKM Plan-1/ISKM Plan-2).
