# CkVat — vertex-animation-texture support (PROMPT.md, mission brief)

> **Written:** 2026-07-09. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships; permanent contribution moves to `Source/CkVat/Claude.md`
> and this file is replaced by a one-line tombstone.

## Goal

CkFoundation gains first-class VAT support: a `UCk_VatCollection_Data` asset baked **in-editor**
from a skeletal mesh + animation list into a static mesh (lookup UVs) + vertex-animation textures
(both **Vertex** and **Bone** modes) with a serialized clip table — and an ECS feature
(`UCk_Utils_Vat_UE`) that plays those clips on instanced static meshes **tick-lessly** (playback
state travels as per-instance custom data; the GPU derives the frame from time). Informed by the
VAMP plugin (docs studied; source unavailable), built from CkFoundation's own precedents.

## Success criteria (each an observation)

1. Baking a `UCk_VatCollection_Data` (editor button/action) from a skeletal mesh + N sequences
   produces: a `UStaticMesh` with a generated lookup-UV channel, mode-appropriate textures
   (Vertex: position+normal atlas; Bone: bone-position+rotation atlas + weights/indices carrier),
   and a **serialized** clip table (name → frame range, sample rate, length). Re-bake overwrites.
2. An entity composed via `UCk_Utils_Vat_UE::Add(...)` renders the baked mesh through
   CkIsmRenderer and `Request_PlayClip(name)` plays it with **zero per-frame CPU writes per
   instance** (verified: custom data written only on state change; animation advances regardless).
3. Clip switch mid-playback crossfades (shader 2-state blend) and frame interpolation smooths
   sub-frame sampling — both toggleable, visually verified in the CkVat gym [EDITOR-VERIFY].
4. `OnVatClipFinished` fires exactly once for a non-looping clip (autotest, headless).
5. The Iskm autotest set (28 tests) shows **zero delta** vs the recorded baseline after the
   shared bake-core extraction (Gate 0 exit evidence in PROGRESS.md).
6. The public surface works in C++, Blueprint, AND AngelScript (non-negotiable #4).

## Constraints & locked decisions

| Decision | Choice | Why |
|---|---|---|
| Module shape | New `CkVat` (Runtime, T4) + `CkVatEditor` (bake) | Maintainer pick 2026-07-09; keeps CkIskmRenderer two-systems-not-three; standard-material path |
| V1 bake modes | **Both** Vertex and Bone | Maintainer pick 2026-07-09 |
| Playback vehicle | CkUsf WPO look on **CkIsmRenderer** instances | No custom VF, no PostConfigInit module, no custom-data contention with the batched crowd layout |
| Shared code home | `CkAnimation/AnimBake` (`ck::anim_bake`) | Maintainer directive: extract the CkIskmRenderer∩CkVat intersection into common utils; CkAnimation is the semantic host and already a declared Iskm dep |
| Playback model | GPU-time-driven (write params on clip change only) | VAMP parity; improvement over Plan-2's per-frame CPU frame push |
| Replication | None — client-local; gameplay owner re-drives | Renderer doctrine (`CkIskmRenderer/CLAUDE.md` anti-pattern #3) |
| Clip identity | `FName` per clip (mirrors `FCk_IskmAnimCollection_SequenceDef`) | Mimicry over invention |
| Branch dependency | **None** on `perf-iskm-lod` | v1 uses plain `Texture2D` atlases + CkUsf auto per-instance slot layout (both on dev); `Texture2DArray`/`_PerInstanceSlot` only if later gates need them |

## VAMP parity / improvement matrix

| VAMP feature | v1 | Notes |
|---|---|---|
| Profile asset + in-editor bake | ✅ | `UCk_VatCollection_Data` + CkVatEditor bake |
| Bone + Vertex modes | ✅ | both |
| High/Low precision | ✅ | `PF_FloatRGBA` vs RGBA8 (bounds-normalized) |
| Tick-less GPU playback, per-instance data on ISM | ✅ | improvement: ECS requests + signals on top |
| Frame interpolation, transitions | ✅ | shader features, per-look switches |
| Fixed-FPS bake rows | ✅ | `_SampleFrequency` (Iskm precedent) |
| Play/pause/rate/select | ✅ | `Request_*` surface |
| Notifications, sockets/bones, AnimBP, ragdoll, root motion, aim-offset, Nanite | ❌ non-goals | see below |
| Improvements over VAMP | — | typesafe handles, deferred requests, `OnClipFinished` signal, AS/BP/C++ parity, async load via soft refs, per-instance phase-offset for crowd variety |

## Non-goals (v1)

Sockets/bone CPU queries; AnimNotify carry-over; tick-less AnimBP/sync markers; ragdoll swap; root
motion; aim-offset/GPU-skeletal-mesh (VAMP 2); Nanite bake; replication/snapshot participation;
an automatic LOD flip driver (game policy, per Iskm precedent). Each is an add-on layer VAMP also
treats as optional; none blocks the bake/playback core.

## Reading list

- Research: scratchpad `RESEARCH_SYNTHESIS_VAT.md` + 5 digests (session 2026-07-09; content
  absorbed into this doc set — the scratchpad copies are disposable).
- Neighbors to mimic: `CkIskmRenderer/AnimCollection/*` (baked-collection asset + GPU lifetime),
  `CkIsmRenderer/Proxy/*` (quartet + custom-data plumbing), `CkTimer` (small quartet),
  `CkParticlesEditor/CkParticles_TextureGenerator.cpp` (editor texture-asset bake),
  `CkUsf` (`Displace` look = WPO exemplar; `CkUsf_Module.cpp` shader-dir mapping).
- Gates: [PLAN.md](PLAN.md).

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| VAT as a mode inside the batched Iskm stack | Skeletal-only; custom-data slots [0]/[1] hard-coded as bone frame indices + BB claims [2..15]; third substrate in a doc-drifted module | `digest_lod_branch.md` §6, maintainer pick 2026-07-09 |
| Custom vertex factory for v1 playback | WPO look needs none; VF path = PostConfigInit engine-only module, no Nanite, no standard materials | `digest_graphics_usf.md` §6/§8 |
| `TOptional` in reflected params | House rule: enum-mode + value pair | root CLAUDE.md |
| Replicating playback state | Renderer doctrine: caller re-drives on remotes | `CkIskmRenderer/CLAUDE.md` anti-pattern #3 |
| Waiting on / merging `perf-iskm-lod` | v1 needs nothing from it (atlas + auto slots suffice); branch owned by a sibling session | PROMPT locked decisions |
