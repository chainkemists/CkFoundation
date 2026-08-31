# CkVisualLod campaign — mission brief (PROMPT.md)

> **Written:** 2026-08-27. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and `Source/CkVisualLod/CLAUDE.md` absorbs the
> permanent contract. On death: delete it, or replace the body with one tombstone line.

## Goal

CkFoundation gains a T4 module, **CkVisualLod**, that owns budgeted skeletal-mesh visual LOD:
per-entity switching between a pooled SKMC `IskmProxy`, a GPU batched crowd member, hidden, and
always-promoted — with ranked promotion, hysteresis, dither crossfades, promote locks, and
per-domain budgets, plus distance-banded render profiles controlling shadow, material, lighting,
animation-update, velocity, geometry-LOD, pass, and culling cost. All policy is configured by a
per-arbiter data asset. BusterBlock's two duplicate
AngelScript systems (`NpcVisualLod`, `AmbientNpc` visual LOD) become thin adopters (arbiter spawn +
signal bindings + game-specific AS helpers) and their duplicated flip logic is deleted.

## Success criteria

1. CkVisualLod compiles in all three environments (C++/BP/AS) and its C++ automation tests pass
   (ranking, budgets, pool invariants).
2. A CkTests gym demonstrates promote/demote crossfades, budget caps, ranked in-view-first
   promotion, promote locks, hide, and suspend/resume with framework-only content.
3. `UCk_Utils_Camera_UE::TryGet_LocalViewInfo` exists and CkVisualLod no-ops on dedicated
   server/editor worlds with no local view.
4. BB roster + ambient run on CkVisualLod arbiters with their existing autotests green and every
   mechanic of the spec-by-example inventory preserved or consciously dropped with sign-off.
5. The BB duplicated flip processors/helpers are deleted.
6. Far GPU members move between hysteretic render-profile bands without changing their stable
   member index, animation phase, custom data, visibility ownership, cosmetics, or highlight state;
   pooled SKMCs and GPU buckets apply the same renderer-data contract.

## Constraints & locked decisions

See the Decisions table in [DESIGN_CkVisualLod.md](DESIGN_CkVisualLod.md) — that table is the
single home (regenerate any review brief from it, never from memory).

## Non-goals

Per DESIGN §Non-goals: no cosmetic generalization (game-side behind signals), no replication, no
CkVisibleRange coupling, and no engine-level per-instance shadow-pass fork. “Cheap shadow” is
expressed through a higher minimum mesh LOD and reduced shadow features in a separate primitive
profile; true shadow-only replacement geometry remains a later measured extension.

## Reading list

- [DESIGN_CkVisualLod.md](DESIGN_CkVisualLod.md) — the approved design.
- Spec-by-example (BusterBlock, branch-local): `Script/ECS/NpcVisualLod/BB_NpcVisualLod_Processor_Flip.as`,
  `BB_NpcVisualLod_Feature.as`, `BB_NpcVisualLod_ViewRank.as`,
  `Script/ECS/AmbientNpc/BB_AmbientNpc_Processor_VisualLod.as` + `BB_AmbientNpc_Feature.as`.
  ⚠️ The view-ranked promotion change there is uncommitted and compile-UNVERIFIED — design intent
  only; the C++ port carries its own tests.
- Modules to mimic: **CkVisibleRange/CkAggro** (quartet layout), **CkCompass** (observer request),
  **CkObjective** (signal macro), **CkTimer** (canonical quartet + request completion).
- Mechanism APIs consumed: `CkIskmRenderer` (`CkIskm_BatchedUtils.h`, `CkIskmProxy_Utils.h`,
  `CkIskmRenderer_Utils.h`), `CkCamera` (`CkCamera_Utils.h`).

## Things ruled out — do not re-investigate

See DESIGN §"Ruled out" — kept in one place.
