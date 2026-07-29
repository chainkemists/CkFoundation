# CkVat

**Purpose:** Vertex-animation-texture (VAT) support: play skeletal animations baked into textures on
instanced STATIC meshes, tick-lessly — the GPU derives the frame from time; the CPU writes playback
state only on clip change. Two bake modes per collection: **Vertex** (per-vertex offset/normal texels;
cheapest, texture-width-bounded) and **Bone** (per-bone position/rotation texels + mesh-carried
weights; scales up, shareable across meshes on one skeleton). The in-editor baker lives in
`CkVatEditor`; the shared sampling core lives in `CkAnimation/AnimBake` (`ck::anim_bake`, extracted
from CkIskmRenderer Plan-2's bake).

> **Campaign status (2026-07-09):** Gates 0-3 code-complete (bake, looks/shaders, runtime hookup,
> finish signals). Visual [EDITOR-VERIFY] passes pending — see [PROGRESS.md](PROGRESS.md). This
> note dies with the campaign docs.

**Depends on:** `Core,CoreUObject,Engine,GameplayTags,Projects,RenderCore,CkCore,CkEcs,CkEcsExt,CkGraphics,CkIsmRenderer,CkLog,CkResourceLoader,CkUsf`.
**Used by:** crowds/props needing cheap animated instances below the CkIskmRenderer batched tier, and
non-skeletal vertex animation (future).

---

## Key API

- `UCk_VatCollection_Data` — bake inputs (skeleton, source mesh, clip list, + all knobs grouped in
  `FCk_Vat_BakeSettings _BakeSettings`: sample freq, mode, precision (High/Low/**Ultra** RGBA32F),
  **bone influences 1/2/4**, **bone-weight storage MeshChannels/WeightTexture**, source LOD,
  lookup-UV channel, max texture width/rows, root-motion/retargeting) + bake outputs (all grouped
  in `FCk_Vat_BakedData _BakedData`: baked static mesh, VAT textures incl. bone index/weight
  textures, **serialized** clip table + texel dims, animated bounds, bake-inputs hash, `IsBaked`).
  Both structs `ShowOnlyInnerProperties` on the collection. Runtime-read-only; the CkVatEditor
  baker writes `_BakedData` (friend). Bake via the **details-panel Bake button** (or
  `UCkVat_BakerSubsystem`). Editing inputs after a bake flips `Get_IsBakeStale()` — the button
  relabels and asset validation errors until rebaked. Deliberately NO auto-bake: the bake saves
  packages, which must never be a property-edit side effect.
  - Accessor shape: `Collection->Get_BakeSettings().Get_BakeMode()` /
    `Collection->Get_BakedData().Get_IsBaked()` (was flat `Get_BakeMode()`/`Get_IsBaked()` pre-
    2026-07-10 grouping).
  - **Bone-weight storage**: `MeshChannels` (indices in UV1/UV2, weights in vertex color —
    sRGB-pre-decoded) vs `WeightTexture` (indices+weights in two data textures by one lookup UV —
    frees UV channels + color, linear end-to-end, the Nanite prerequisite). Verified bit-identical
    by `Ck_Vat_DebugVerifyBake` (runs BOTH storages).
- `UCk_Utils_VatProxy_UE::Add(InHandle, InParams)` — compose Vat on an entity. `_Collection` is a
  soft ref for authoring, but the LOAD CONTRACT is unchanged: it must be resident AND baked before
  Add (async-load it yourself, mirroring CkIskmRenderer's contract) — Add ensures residency and
  pins the resolved collection on Current via an inline-completing rooted batch (consumer id
  `VatProxy.CollectionPin`).
- `Request_PlayClip / Request_Stop / Request_SetPlayRate` — deferred playback control. Stop freezes
  the current frame; SetPlayRate preserves the playback position (start-time rebase); rate 0 == Stop.
- `BindTo_OnClipFinished` — fires once per non-looping clip completion (Gate 3).

---

## Pattern

GPU-time-driven playback: `frame = f((WorldTime - PlaybackStartTime) * PlayRate)` evaluated in the
material; per-instance custom data carries (clip row range, start time, rate, crossfade pair). All
mutations go through the request queue — processors write `FFragment_VatProxy_Current`; nothing per-frame.

**Modular characters** (one collection = one baked mesh + skeleton): compose several VatProxy
entities as children of a parent entity (body + head + outfit, each its own collection), sharing
the parent's transform. There is no single-VatProxy multi-mesh path — that's deliberate (each
collection bakes exactly one mesh). Drive the pieces in lockstep by issuing the same `Request_*`
to each child, or bind them to one owner's state. (Not yet exercised in a gym — future.)

---

## Implementation notes

Rationale that used to live as comments in the source. The code now points here.

- **Never derive texture dimensions from `UTexture2D::GetSizeX/Y`.** `_TextureWidth`/`_TextureRows`
  are SERIALIZED on `FCk_Vat_BakedData` for exactly this reason: `GetSizeX/Y` read PLATFORM data and
  return 0 while a freshly-baked texture is still async-compiling. That seeded `BoneCount`/`TotalRows`
  = 0 into the shared MID and collapsed every bone lookup onto one texel (found via
  `Ck_Vat_DebugVerifyBake`). Both the collection and `UCk_Vat_Subsystem_UE::GetOrCreate_RenderState`
  read the serialized values only.
- **One MID per collection, never per entity.** Per-instance playback rides custom-data floats, so all
  instances of a collection must share ONE `UMaterialInstanceDynamic` — a per-entity MID would key a
  separate ISM component in the transient factory and defeat instancing. `GetOrCreate_RenderState`
  returns `FCk_Vat_RenderState` **by value** (a two-pointer copy), never a pointer into
  `_RenderStates`: TMap element addresses are not stable across `Add`, so an interior pointer would
  dangle as soon as a later collection's `Add` rehashes the map. The map stays the UPROPERTY store so
  the `TObjectPtr`s remain GC-traced.
- **Normals bake into the vertex's BIND-POSE TANGENT frame.** Tangent-space is invariant under the
  per-instance transform (the TBN co-rotates), so the pixel shader feeds the material's tangent-space
  Normal pin directly — there is no per-instance basis in the PS (only `IS_NANITE_PASS` exposes
  `InstanceId` there).
- **The VAT bake IS the shipped asset**, unlike CkIskmRenderer's transient bake: cooked builds never
  re-sample sequences, so the clip table and texel dims are serialized on the collection.
  `UCk_VatCollection_Data` otherwise mirrors `UCk_IskmAnimCollection_Data`'s shape where the concerns
  overlap.
- **`ApplyBakeResults` is the only write path that flips the serialized `_IsBaked` bit**, and it
  ensures on its inputs first — a collection stamped as baked from a partially-failed baker's results
  would persist the corruption to disk and only surface at first render, far from the cause.
- **`WeightTexture` storage is the Nanite prerequisite**: mesh data channels are limited under Nanite;
  a texture fetch by lookup UV is not.
- **Known gap:** `FProcessor_VatProxy_FireSignals` does not detect reverse (negative-rate)
  once-completion.

---

## Anti-patterns

1. Don't replicate playback state from this module — the gameplay owner replicates its state and
   re-issues `Request_*` on remotes (same doctrine as CkIskmRenderer anti-pattern #3).
2. Don't read `_PlaybackStartTime` as "when Play was called" — rate changes rebase it.
3. Don't hand-edit the `Baked` category fields on the collection — re-bake instead.

---

## See also

- `CkAnimation/AnimBake/CkAnimBake.h` — the shared sampling core (compaction, ref pose, frame
  layout, pose sampling, animated bounds, looped time→frame math).
- `CkIskmRenderer/Claude.md` — the batched bone-palette sibling (higher fidelity, custom VF).
- `CkIsmRenderer/CLAUDE.md` — the instance-management substrate Gate 3 composes.
- `PROMPT.md` / `PLAN.md` / `PROGRESS.md` — campaign docs (die at ship).
