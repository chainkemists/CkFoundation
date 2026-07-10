# CkVat — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-07-09 (branch `feature/vat-feature`, base `545be1a53`, NOT pushed):** Gate 0 ✅
(commits `f69f9095a..8649f328c`). **Gate 1 code-complete** (commits `9e238e06c`, `6575ac566`):
full baker for both modes, build green, Iskm suite **29/0/0 post-Gate-1** (zero delta, third
consecutive green). Remaining for Gate 1 exit: the [EDITOR-VERIFY] bake on real content (human —
steps in Plan/Gate_01_Bake.md expected-observations table).
**Baseline on record:** Iskm autotests 29/0/0 (tests_baseline.log, 2026-07-09).
**Next action:** Gate 2 entry — VAT decode looks in CkUsf (WPO entry per mode, second-UV wiring in
the generator, per-instance playback params), against the Gate_01 layout contract.
**Blocked on:** [EDITOR-VERIFY] bake (human) before Gate 1 flips to ✅; Gate 2 can start in parallel.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-09 | New CkVat+CkVatEditor; both bake modes v1; ISM+CkUsf playback vehicle | Maintainer picks (see PROMPT locked table) | — |
| 2026-07-09 | Shared core home = CkAnimation/AnimBake, namespace `ck::anim_bake` | Semantic host; already a declared (previously unused) Iskm dep — makes it real | If CkAnimation gains heavy editor-only weight, reconsider split |
| 2026-07-09 | No dependency on `perf-iskm-lod` for v1 | Atlas Texture2D + CkUsf auto per-instance slots (both on dev) suffice | Gate 2, if look authoring wants Texture2DArray |
| 2026-07-09 | Iskm refactor keeps dev semantics exactly (skeleton-chain ref pose) | perf-iskm-lod's mesh-bind-pose fix belongs to that branch; porting it early would change dev behavior untested here | When perf-iskm-lod merges — fold its two bake deltas into `ck::anim_bake` call args |

## Dated entries (append-only, newest first)

### 2026-07-09 — Gate 0 committed; Gate 1 baker landed (code-complete)
- Committed Gate 0 on `feature/vat-feature` (user-created): `f69f9095a` anim-bake core, `08940084c`
  Iskm refactor, `eb2087ee3` CkVat runtime, `07a437601` CkVatEditor+uplugin, `8649f328c` docs.
  NOT pushed. Note: repo `.gitignore:49` blanket-ignores `*.md` — docs force-added (existing
  tracked module docs prove the precedent).
- Gate 1: engine APIs verified at file:line against the 5.7 fork by a research agent BEFORE coding
  (FMeshDescription authoring, editor source model + FSoftSkinVertex **uint16** influences,
  ConvertMeshesToStaticMesh ceremony, TSF_RGBA16F/TC_HDR). Contract + layout spec:
  Plan/Gate_01_Bake.md.
- Landed + committed (`9e238e06c` bounds/ApplyBakeResults, `6575ac566` baker): full bake flow for
  BOTH modes; entry `UCkVat_BakerSubsystem::Bake_VatCollection`.
- Ran: toolbox --build → "Result: Succeeded" (build_gate1c.log; two fix rounds: missing collection
  include in the subsystem TU; unformattable unnamed-enum `MAX_MESH_TEXTURE_COORDS_MD` in an ensure).
- Ran: Iskm suite regression post-Gate-1 → **29 passed / 0 failed / 0 skipped** (tests_gate1.log) —
  zero delta vs baseline, third consecutive green run on this branch.
- Confirmed: layout contract in Gate_01_Bake.md cross-checked against the shipped encoder code
  (row-0 ref pose, lookup U formula, bone index/weight carriers, precision encodings) — consistent.
- Inferred (unconfirmed, needs [EDITOR-VERIFY]): the bake produces visually-correct data on real
  content. The one claim most likely wrong: **triangle winding** on the baked static mesh (kept
  source index order; if meshes render inside-out, swap corners 0/2 in BuildBakedStaticMesh).
  Also unconfirmed: mesh-bind-pose vs skeleton-ref-pose divergence on reoriented imports (dev
  semantics kept; the perf-iskm-lod fix folds in at merge — decision log).

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
