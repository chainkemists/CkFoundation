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
- **`FCk_MontagePlayer_State::_Montage` is soft too, since 2026-08-26 — the ruling this file used to defer.**
  It had stayed a hard `TObjectPtr` pending a maintainer decision, because it is replicated AND save-serialized
  (`Register_NetAndSave_SharedApply`) so flipping it is a wire/save-format change. That deferral is what QA's
  2026-08-26 crash landed on: the field lives in an ECS fragment, GC never walks the EnTT registry, so it rooted
  nothing and **dangled instead of nulling** when the montage was collected — and the snapshot capture reads
  `IsAsset()` through it (`Audit_DurableObjectRefs`) and `GetPathName()` through it (`Serialize_OwnedStruct`, which
  sets `ArIsSaveGame = false`, so every non-Transient object property is serialized). The same fragment already
  held the montage weakly in `_ActiveMontage`; the hard sibling was the outlier.
  - **The saved bytes still name the montage by path**, as the archive already did for a hard ref — but the
    property TAG changes `ObjectProperty` → `SoftObjectProperty`, so an older save loads through
    `FSoftObjectProperty::ConvertFromType`'s `ObjectProperty` branch, which exists for exactly this migration
    (*"used to be a raw FObjectProperty Foo\* but is now a TSoftObjectPtr<Foo>"*). **No fixture pins that**, so it
    is supported-by-design rather than measured — a committed pre-change blob + load test is the honest follow-up.
    What definitely changes is the LOAD: the hard ref came back through `FObjectAndNameAsStringProxyArchive` with
    `LoadIfFindFails = true`, i.e. a SYNCHRONOUS load during hydration, which the preload batch now replaces.
  - **A montage that cannot be resolved now ENSURES rather than silently not playing.** The batch reports
    `Get_HasFailed` and the drain ensures. That is the intended direction (fail loud, not silent), but it IS a
    semantic change on the wire and on restore: with a mismatched cook or an unmounted pak chunk, what used to be
    a cosmetic miss is now an ensure — and ensures stay live in Shipping in this codebase.
  - **The wire form did change** — it travels as a path rather than a NetGUID. A client already needs the montage
    resident to play it, and the `"MontagePlayer.Play"` batch is what makes it so.
  - **A second consumer id came with it: `"MontagePlayer.ReplicatedState"`.** `DoDispatchReplicatedState` enqueues
    its Play request straight onto the queue instead of calling `Request_Play`, so it kicks its own batch. Without
    one the drain resolves the soft ref resident-or-null and a replicated or RESTORED montage that is not already
    loaded silently does not play — which the hard ref used to hide, because the save archive resolves with
    `LoadIfFindFails` and therefore synchronously LOADED the montage on restore. The batch is that loader made
    explicit and asynchronous; a cold one stalls that entity's queue, order preserved, exactly as an authored Play.
  - The rule is enforced now rather than remembered: `Ck.Snapshot.Meta.FragmentPostureCoverage` reds any registered
    Durable payload carrying a hard object ref, through `ck::Get_DurablePayloadObjectRefPolicy`.

---

## See also

- `CkRecord/Claude.md` — Record entity pattern.
- `CkLabel/Claude.md` — naming anim slots.
- `CkEcsExt/Claude.md` — Meta Fragment infrastructure used by AnimAsset.
