# CkAnimation

**Purpose:** Animation assets on entities. Each entity can hold a Record of AnimAsset entities; each AnimAsset carries animation params (asset reference, slot, play mode). The Utils class follows the standard Add / Has / Cast / Record pattern.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`.
**Used by:** Character movement, ability systems, any feature that plays animations on an entity.

---

## Key API

- `UCk_Utils_AnimAsset_UE::Add(InHandle, InParams)` — add a new animation asset to the entity's Record.
- `UCk_Utils_AnimAsset_UE::AddMultiple(InHandle, InParams)` — batch add.
- `UCk_Utils_AnimAsset_UE::Has(InHandle)` — entity has an anim asset Record.
- Standard Cast / CastChecked / InvalidHandle helpers.

---

## Pattern

All anim asset entities live in a Record on the owning entity. Use `CkLabel` to distinguish named anim slots (e.g., `Tag_Anim_Attack`, `Tag_Anim_Idle`).

```cpp
auto TrackHandle = UCk_Utils_AnimAsset_UE::Add(InOwnerHandle, AnimParams);
UCk_Utils_GameplayLabel_UE::Add(TrackHandle, Tag_Anim_Attack);
```

---

## Anti-patterns

1. Don't drive animations from Processors directly — route through the AnimAsset entity so the animation system's own processors manage lifecycle.
2. Don't skip labeling anim entries when an entity needs multiple anim slots.

---

## `ck::anim_bake` — shared CPU animation-sampling core

`AnimBake/CkAnimBake.h` is the sampling core shared by every frame-baked renderer. It was extracted from
CkIskmRenderer's `Build_BakedPoseData` (itself a Skelot port): the parts of a bake that are independent of
the OUTPUT ENCODING — render-bone compaction, the component-space reference pose (+ its inverse), the frame
layout (frame 0 = reference pose, sequences in contiguous ranges after it), fixed-frequency pose sampling,
and the conservative animated bounds. Consumers supply a per-frame callback that encodes the pose into
their own target:

- **CkIskmRenderer** — transposed 3x4 bone matrices, flat `Buffer<float4>`-ready array (bone-palette skinning).
- **CkVat** — bone/vertex animation TEXTURES; vertex mode additionally CPU-skins per vertex.

CPU-only, no RHI, so it is safe headless under `-nullrhi`. Editor-time and load-time callers both use it.

**`FCk_AnimBake_SampleParams::UseMeshBindRefPose`** (port of Skelot's `RefPoseOverrideMesh`) inverts the
DefaultMesh bind pose instead of the skeleton ref pose when building `RefPoseInverse`. It exists because the
skeleton ref pose and the anims both carry the +X import reorientation while the mesh binds facing -Y:
inverting the skeleton pose cancels the reorientation, so skinned output faces -Y while moving +X (the
"strafing" signature). Mesh-bind matches the engine SKMC contract (`GetRefBasesInvMatrix() * pose`). It is
**off by default** (skeleton ref pose) so existing bakers are unaffected — CkVatEditor's baker deliberately
leaves it off; the skeleton-chain inverse is correct for that content, and a mesh-bind override must not be
re-added there without clean-build evidence.

---

## MontagePlayer

`FProcessor_MontagePlayer_Replicate` resolves its driver through `FFragment_ContainerRef_MontagePlayer` and
builds the payload through the registered `Produce` handler; the two are byte-identical by construction, so
do not re-inline the payload build.

## Soft asset refs (fragment-GC sweep)

UE GC never walks the EnTT registry, so a fragment-held `TObjectPtr<UAsset>` roots nothing. This module's
authored asset refs are therefore soft:

- **AnimAsset** (`FCk_AnimAsset_Animation::_Animation`/`_BlendSpace`) is **pure path data** — it kicks no
  loads, roots nothing, and has no processor. Consumers resolve: resident-or-null via `.Get()`, or through
  their own CkResourceLoader consumer id for a rooted async load.
- **MontagePlayer** `FCk_Request_MontagePlayer_Play::_Montage` is soft; `Request_Play` kicks a
  CkResourceLoader `RootedAssetBatch` riding the request (consumer id `"MontagePlayer.Play"`, flip to
  Synchronous per-project in the ResourceLoader settings for debugging). Resident montages keep the
  synchronous pre-flight byte-identical; an authored-but-not-resident montage defers pre-flight to the
  drain (which re-runs `Get_CanPlayMontage` once resolved and mirrors the OnFinished failure shape). A
  cold-loading Play stalls the whole per-entity request queue (order preserved) and marks
  `FTag_MontagePlayer_PendingAssetLoad`; teardown fires `Failed_Cancelled` for stalled entries. The played
  montage is rooted by the AnimInstance's montage instance after `Montage_Play`, so the batch dies with
  the consumed request.
- **`FCk_MontagePlayer_State::_Montage` stays a hard `TObjectPtr` deliberately** — it is replicated AND
  save-serialized (`Register_NetAndSave_SharedApply`), so flipping it is a wire/save-format change gated
  on a maintainer ruling. A save restored when the montage is not resident still reads null there.

---

## See also

- `CkRecord/Claude.md` — Record entity pattern.
- `CkLabel/Claude.md` — naming anim slots.
- `CkEcsExt/Claude.md` — Meta Fragment infrastructure used by AnimAsset.
