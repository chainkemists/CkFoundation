# Gate 3 — Playback (runtime rendering hookup)

> **Status:** see [PLAN.md](../PLAN.md). **Depends on:** Gate 2 (looks + masters). **Estimate:** 1 session (2026-07-09).

## Goal

After this gate: `UCk_Utils_VatProxy_UE::Add` + `Request_PlayClip` on a Transform-bearing entity renders
the baked mesh as a moving, animated ISM instance — per-instance playback floats written only on
clip change; `OnVatClipFinished` fires once per completed Once clip.

## What landed (all verified by build + suite, file references current)

- `UCk_Vat_Subsystem_UE` (world subsystem): one shared MID + one transient ISM renderer per
  collection (`GetOrCreate_RenderState`) — MID from the generated look master
  (`ck::usf::Get_GeneratedMasterObjectPath`, outline-subsystem precedent), uniforms seeded from the
  collection, renderer via the NEW `GetOrCreate_ForMeshWithMaterialsAndCustomData` (custom-data
  count is part of the factory cache key; **Movable** is load-bearing — the ISM proxy pushes
  custom data to the GPU only on the Movable path).
- `FProcessor_VatProxy_Setup`: initial clip state (+ `RandomPerInstance` phase offset for crowd variety),
  then composes the IsmProxy on the SAME entity (requires a Transform — loud ensure otherwise) and
  pushes the initial 12 floats.
- `FProcessor_VatProxy_HandleRequests`: one `Request_SetCustomInstanceData` push per drained batch.
- `FProcessor_VatProxy_FireSignals`: CPU mirror of the GPU clock for Once clips → `OnClipFinished`
  broadcast, `_FinishedDispatched` latch. Negative-rate completion not detected (follow-up).
- Crossfade source now carries its own rate/loop (`_PrevPlayRate`/`_PrevLoopMode`) — captured at
  PlayClip, packed into floats [6..7]/[11].
- Teardown rides the entity: IsmProxy EndPlay removes the instance (stable `FPrimitiveInstanceId`,
  no index shifting).

## Known contract couplings (checked, not guessed)

- Float packing (`ck_vat_proxy_processor::Pack_CustomData`) ⇄ look per-instance declaration order ⇄
  `UCk_Vat_Subsystem_UE::NumPerInstanceFloats == 12` ⇄ Gate_02 contract table. One change = all four.
- CPU clock = `UCk_Utils_Time_UE::Get_WorldTime`; GPU clock = material `Time` node. [EDITOR-VERIFY]:
  clip completion visually coincides with the pose freezing on the last frame (clock alignment).

## [EDITOR-VERIFY] (human, after Gate 1's bake verify)

1. In a test map/gym: spawn an entity with Transform + `UCk_Utils_VatProxy_UE::Add(collection, InitialClip)`.
   Expect: baked mesh instance appears and ANIMATES with zero per-frame CPU writes (verify via
   `stat CkProcessors` — Vat processors near-zero when no requests fire).
2. `Request_PlayClip` with `_TransitionDuration = 0.3s` → smooth crossfade, no pop.
3. `Request_SetPlayRate(0)` freezes mid-pose; `Request_SetPlayRate(2)` resumes at double speed from
   the same pose.
4. A `Once` clip: pose clamps at the last frame AND `OnClipFinished` fires (bind in AS/BP).
5. Instance follows the entity transform when moved (Movable path).

## Exit criteria

- [ ] Build green; Iskm suite 29/0/0; AS wrapper regen clean (`utils_vat.as` regenerates with the
      new UFUNCTIONs, no `Angelscript: Error` naming Ck files).
- [ ] [EDITOR-VERIFY] list above handed to the human.
- [ ] PLAN.md row + PROGRESS.md entry; Claude.md campaign note refreshed.
