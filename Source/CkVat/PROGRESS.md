# CkVat — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-07-09 (dev `545be1a53` + UNCOMMITTED working tree):** **Gate 0 DONE.** Post-change Iskm
suite **29 passed / 0 failed / 0 skipped** — zero delta vs the recorded baseline (tests_after.log,
final binaries `BusterBlockEditor-CkVat.dll` 21:13). Modules load; AS wrapper generator emitted
`Script/Generated/utils_vat.as` during the test boot (mixin forms well-formed). Nothing committed —
awaiting maintainer's word on branch/commit.
**Baseline on record:** Iskm autotests 29/0/0 (tests_baseline.log, 2026-07-09).
**Next action:** Gate 1 entry — author `Plan/Gate_01_Bake.md`, then the CkVatEditor baker
(vertex+bone atlases via `ck::anim_bake::SamplePoses`, static-mesh build w/ lookup UVs, clip table).
**Blocked on:** nothing (commit decision is the maintainer's).

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-09 | New CkVat+CkVatEditor; both bake modes v1; ISM+CkUsf playback vehicle | Maintainer picks (see PROMPT locked table) | — |
| 2026-07-09 | Shared core home = CkAnimation/AnimBake, namespace `ck::anim_bake` | Semantic host; already a declared (previously unused) Iskm dep — makes it real | If CkAnimation gains heavy editor-only weight, reconsider split |
| 2026-07-09 | No dependency on `perf-iskm-lod` for v1 | Atlas Texture2D + CkUsf auto per-instance slots (both on dev) suffice | Gate 2, if look authoring wants Texture2DArray |
| 2026-07-09 | Iskm refactor keeps dev semantics exactly (skeleton-chain ref pose) | perf-iskm-lod's mesh-bind-pose fix belongs to that branch; porting it early would change dev behavior untested here | When perf-iskm-lod merges — fold its two bake deltas into `ck::anim_bake` call args |

## Dated entries (append-only, newest first)

### 2026-07-09 — Gate 0 implementation (same session, later)
- Ran: baseline Iskm tests → **29 ran, Failed: 0, Skipped: 0** (tests_baseline.log). The module doc
  says 27, the filename count is 28; the pattern additionally matched `SkmcPerf` → 29 is the number
  to diff against.
- Landed: `ck::anim_bake` core (`CkAnimation/Public/CkAnimation/AnimBake/CkAnimBake.{h,cpp}`) —
  BuildSkeletonData / BuildFrameLayout / SamplePoses / ComputeAnimatedBounds / Get_LoopedLocalFrame,
  extracted 1:1 from the Iskm bake (dev semantics kept — perf-iskm-lod's mesh-bind-pose fix NOT
  ported; see decision log).
- Landed: Iskm `Build_BakedPoseData` refactor — body-only delegation, public header untouched,
  `FCk_Iskm_BakedPose` output identical by construction. Compile-verified in isolation:
  build_extraction.log "Result: Succeeded" (35s incremental).
- Landed: CkVat scaffold — collection asset (incl. serialized `FCk_Vat_BakedClip` table + duplicate-
  name validation), typesafe handle, ParamsData, 3 requests, Current/Requests fragments,
  OnClipFinished signal, Setup + HandleRequests processors (playback state machine: crossfade
  bookkeeping, freeze-preserving Stop, position-preserving SetPlayRate), Utils surface, log/module
  plumbing, Claude.md. CkVatEditor scaffold (module plumbing only; baker = Gate 1). Both registered
  in CkFoundation.uplugin (CkVat Runtime/Default, CkVatEditor Editor/Default).
- Confirmed idioms against live code before writing: request enqueue (CkIskmProxy_Utils.cpp:344),
  CopyAndRemove drain (CkAnimPlan_Processor.cpp), CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE
  (CkAnimAsset_Utils.cpp:41), signal define arity (CkAnimPlan_Fragment.h:151-157), FCk_Time ops
  (CkTime.h:37-40,82).

### 2026-07-09 — Gate 0 session start
- Ran: toolbox `--build` (BB root) → "Result: Succeeded", "Target is up to date" (build_baseline.log).
- Ran: toolbox `--test --test-pattern "IskmRenderer" --no-nullrhi` → IN FLIGHT (baseline).
- Confirmed: dev clean at `545be1a53`; no editor process; 28 `CkAutoTest_IskmRenderer_*.as` in CkTests.
- Confirmed: research phase complete (5 digests + synthesis in session scratchpad; absorbed into PROMPT.md).
- Inferred (unconfirmed): Iskm suite currently green (module doc claims it) — the in-flight run confirms or corrects.

## Open items
| Item | Status | Next step |
|---|---|---|
| Baseline Iskm test counts | in flight | read log verdict, record here |
| Doc-drift follow-ups found in research (CkGraphics/Claude.md false claims; CkIskmRenderer Claude.md stale layout/framing; unused declared deps CkPhysics in Iskm Build.cs) | recorded, out of scope | separate chip/session |
**Rule: no completion claim anywhere in this file while any row here is unresolved.**
