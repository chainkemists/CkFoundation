# Gate 0 — Foundation (shared bake core + module scaffolds)

> **Status:** see [PLAN.md](../PLAN.md) (single home). **Depends on:** — (first gate).
> **Estimate:** 1 session (2026-07-09).

## Goal

After this gate: the anim-sampling core shared by CkIskmRenderer's Plan-2 bake and the future
CkVat bake lives in `CkAnimation/AnimBake` (`ck::anim_bake`), CkIskmRenderer consumes it with
**zero behavioral change** (autotest-diffed), and `CkVat`/`CkVatEditor` exist as compiling,
uplugin-registered modules whose data shapes (collection asset, handle, params, requests,
fragments, signals) are final enough for Gate 1-3 to fill in behavior.

## Entry criteria (run, don't assume)

- [x] dev tree clean at `545be1a53`; no editor running.
- [x] Baseline build: toolbox `--build` → "Result: Succeeded" (up to date), 2026-07-09.
- [x] Baseline tests: toolbox `--test --test-pattern "IskmRenderer" --no-nullrhi` — **29 ran /
      0 failed / 0 skipped**, recorded in PROGRESS.md before any code edit (tests_baseline.log).

## Work items

1. `ck::anim_bake` core in CkAnimation — extraction of `Build_BakedPoseData` steps 1-3/5-7
   (`CkIskmAnimCollection_Fragment_Data.cpp:57-236`): skeleton prep (compaction, ref pose +
   inverse), frame layout, per-frame component-space sampling w/ caller callback, animated
   bounds, looped time→frame math. Pattern replication — not new infrastructure.
2. Iskm refactor — `Build_BakedPoseData` body delegates to the core; `FCk_Iskm_BakedPose` output
   unchanged (public API byte-stable). **Conflict note:** `perf-iskm-lod` edits this file
   (effective-skeleton fallback + mesh-bind ref pose); keeping the refactor body-only and the
   core's inputs explicit (skeleton passed in) makes their two deltas portable into call args.
3. `CkVat` scaffold — Build.cs, module/log plumbing, `Collection/CkVatCollection_Data` (asset
   shape incl. serialized clip table), `CkVat_Fragment_Data.h` (handle/params/requests/delegates),
   `CkVat_Fragment.h` (tags/fragments/signals), processor + utils skeletons (Setup validates and
   defers to Gate 3 via explicit ensure-on-unimplemented, never silent), `Claude.md`.
   NEW INFRASTRUCTURE — data-shape risk lives here (asset shape migrates poorly; get it right now).
4. `CkVatEditor` scaffold — Build.cs, module plumbing, baker entry stub. Type follows the
   `Ck<Feature>Editor` uplugin precedent.
5. Both modules registered in `CkFoundation.uplugin` (standard Win64/Mac/Linux allowlist, Default).

## Expected observations — and branches

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Baseline Iskm test pass | 28/28 pass (doc claims suite green) | pre-existing reds | record names; they are the inherited baseline, not mine to fix; diff against them |
| toolbox `--build` after edits | Succeeded | UHT/link errors | fix; `ck-debugging-playbook` if stuck 2 attempts |
| Post-change Iskm test pass | identical counts + names vs baseline | new red | A/B: stash refactor, re-run; if mine, fix or revert the refactor (never ship a changed bake) |
| `rg 'anim_bake' Source/CkIskmRenderer` | consumption at exactly the bake site | — | — |

## Exit criteria — same commit as the last work item

- [ ] All observations above confirmed; evidence (counts, log verdicts) in PROGRESS.md.
- [ ] No public-API change to CkIskmRenderer (`Fragment_Data.h` header diff = zero or comment-only).
- [ ] CkVat/CkVatEditor compile in the editor target; modules load (verified via the test run's
      editor boot log — module-load lines present, no `Angelscript: Error` naming Ck files).
- [ ] PLAN.md status row flipped; PROGRESS.md dated entry appended.
- [ ] `[EDITOR-VERIFY]` items listed for the human (none expected this gate — no visual surface yet).
