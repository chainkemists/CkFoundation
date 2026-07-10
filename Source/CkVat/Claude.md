# CkVat

**Purpose:** Vertex-animation-texture (VAT) support: play skeletal animations baked into textures on
instanced STATIC meshes, tick-lessly — the GPU derives the frame from time; the CPU writes playback
state only on clip change. Two bake modes per collection: **Vertex** (per-vertex offset/normal texels;
cheapest, texture-width-bounded) and **Bone** (per-bone position/rotation texels + mesh-carried
weights; scales up, shareable across meshes on one skeleton). The in-editor baker lives in
`CkVatEditor`; the shared sampling core lives in `CkAnimation/AnimBake` (`ck::anim_bake`, extracted
from CkIskmRenderer Plan-2's bake).

> **Campaign status (2026-07-09):** Gate 0 — data shapes + playback state machine exist; the bake
> (Gate 1), the CkUsf looks/shaders (Gate 2), and the CkIsmRenderer hookup + finish signals (Gate 3)
> are chartered in [PLAN.md](PLAN.md). Until Gate 3, a Vat entity carries playback state without a
> visual. This note dies with the campaign docs.

**Depends on:** `Core,CoreUObject,Engine,GameplayTags,CkCore,CkEcs,CkEcsExt,CkLog`
(Gate 2/3 add `CkIsmRenderer`, `CkUsf`, `CkGraphics`, `CkAnimation` as they are consumed).
**Used by:** crowds/props needing cheap animated instances below the CkIskmRenderer batched tier, and
non-skeletal vertex animation (future).

---

## Key API

- `UCk_VatCollection_Data` — bake inputs (skeleton, source mesh, clip list, sample frequency, mode,
  precision) + bake outputs (baked static mesh, VAT textures, **serialized** clip table, animated
  bounds). Runtime-read-only; the CkVatEditor baker writes it.
- `UCk_Utils_Vat_UE::Add(InHandle, InParams)` — compose Vat on an entity (collection must be loaded
  AND baked; async-load soft refs yourself, mirroring CkIskmRenderer's contract).
- `Request_PlayClip / Request_Stop / Request_SetPlayRate` — deferred playback control. Stop freezes
  the current frame; SetPlayRate preserves the playback position (start-time rebase); rate 0 == Stop.
- `BindTo_OnClipFinished` — fires once per non-looping clip completion (Gate 3).

---

## Pattern

GPU-time-driven playback: `frame = f((WorldTime - PlaybackStartTime) * PlayRate)` evaluated in the
material; per-instance custom data carries (clip row range, start time, rate, crossfade pair). All
mutations go through the request queue — processors write `FFragment_Vat_Current`; nothing per-frame.

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
