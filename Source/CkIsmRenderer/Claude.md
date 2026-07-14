# CkIsmRenderer

**Purpose:** Instanced Static Mesh (ISM) rendering via ECS. ISM proxy entities manage ISM instances and update transforms/params from ECS fragments without going through UE's actor system.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkGraphics`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkUsf`.
**Used by:** Procedural foliage, projectile trails, crowd rendering — any feature that needs many instances without per-actor overhead.

---

## Key API

- `UCk_Utils_IsmProxy_UE` — add ISM proxy to entity, update transform batch, set custom data.
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

## See also

- `CkGraphics/Claude.md` — lower-level graphics utilities.
- `CkEcs/Claude.md` — processor and fragment patterns.
- `CkUsf/Claude.md` — entity-level outline request API + `DESIGN_EntityOutlines.md` (full outline architecture across
  ISM/ISKM Plan-1/ISKM Plan-2).
