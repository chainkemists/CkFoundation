# CkVisualLod campaign — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-30 (`CkFoundation` base `af9f47379`, `CkTests` base `635361ef`): Gate 4 range
render profiles plus live arbiter runtime tuners/debugger controls are automated-green; rendered
pixel shadow/A-B evidence is still pending. This line supersedes the older current-state paragraphs below.**
The dither mechanism and both generated masters are committed; the obsolete
`VisualLodDissolve`/all-slots instructions below are history, not actions. Gate 4 adds shared
renderer-data policy to pooled SKMCs and GPU clusters, stable `(tile, profile)` GPU buckets, and
per-crowd hysteretic distance bands. Pre-change baseline:
`scratch/baseline_visual_lod_render_profiles_20260830-200347.md`; its broad editor build failed at
action 23/588 without diagnostics before tests. The final build succeeds and the fresh D3D12/SM6
`VisualLod` filter reports 13/13 passed, including functional profile migration, deferred
runtime-tuner set/reset through AngelScript, atomic invalid-tuner validation, and the
mannequin/fade-material shadow-readiness contract. The GPU proxy now also forwards the component's
depth-pass policy into primitive relevance. The gym has a dedicated shadow-casting directional key
light; that makes the full-to-reduced transition judgeable, but automation does not inspect pixels.
**Next action:** complete the gym's rendered threshold/shadow/material `[EDITOR-VERIFY]`, then
capture N>=3 alternating full/reduced Unreal Insights or GPU Profiler windows. **Blocked on:** nothing.

**As of 2026-08-29 — dither round REPLACES the gym-verify round's dissolve half (all UNCOMMITTED,
gate GREEN 00:05):** maintainer rejected the noise-erosion dissolve ("electric"; crowd never faded)
and chose a dithered crossfade on the REAL materials. The CkUsf surface-replacing dissolve is
DELETED (VisualLodDissolve .ush + AS look + `_ProxyDissolveLook` config + `_ProxyDissolveMid` +
IskmProxy `Request_SetMaterialOverride_AllSlots`; CkIskmRenderer back to byte-HEAD). Replacement:
one `_FadeAlpha`, two channels — crowd per-instance slot 13 (unchanged) + near-mesh custom
primitive data at new `_FadeNearCustomPrimitiveDataSlot` (default 0) via the proxy's
`Request_SetCustomDataFloat` lane (submesh-mirrored; **RendererData must declare
`_NumCustomDataFloat > slot`** — loud ensure). Complementary masks share
`CkUsf_VisualLod_FadeThreshold` (Common.ush); looks `VisualLodCrowdFade` (keeps threshold<α) /
`VisualLodNearFade` (keeps threshold>=α, via the new CkUsf `_CustomPrimitiveData` look-param
source); gym wires crowd slots on OnCrowdCreated and the near look per promote. Contract doc: this
file's CLAUDE.md §Crossfade contract. **The old "run Ck_Usf_GenerateLooks VisualLodDissolve"
instruction below is OBSOLETE — instead: `Ck_Usf_GenerateLooks VisualLodCrowdFade` +
`Ck_Usf_GenerateLooks VisualLodNearFade`, commit both masters, delete the stale untracked
`M_CkUsf_Look_VisualLodDissolve.uasset`.** Known gap: the Hat submesh receives the fade value but
keeps its non-contract material (base-mesh-only override scope) → hat pops; fix options in the
plan spine (`<superproject>/scratch/PLAN_ckvisuallod_debugger_20260828.md`).

**As of 2026-08-28 — debugger round layered ON TOP of the still-pending gym-verify round (both
UNCOMMITTED):** maintainer approved an HTML mockup and ordered the debugger built. Additive-only
changes in THIS module: freeze tag/request (`FTag_VisualLodArbiter_Frozen` + `Request_SetFrozen`;
frozen = no gather/rank/flips/far-anim, in-flight fades finish with reversal suppressed, recovery
still runs), cached `_LastView`, per-tick `_{Promotes,Demotes,Preempts}ThisTick` ("flips STARTED"),
retained member `_LastDistance`/`_LastInView`, and the C++-only debugger getter surface on both
Utils (incl. the dissolve fault ladder: `Get_HasProxyDissolveConfigured` /
`Get_IsProxyDissolveLookResolved` / member `Get_HasProxyDissolveMid`). New consumer module
`CkGameplayDebugger/Source/CkVisualLodDebugger` (Systems/50, `ck.VisualLodDebugger`; its CLAUDE.md
+ `Mockups/mockup_visuallod_debugger.html` = approved visual spec). Compile gate GREEN 2026-08-28
22:53 (`=== Build succeeded ===`, BuildTest.log). Plan/status spine:
`<superproject>/scratch/PLAN_ckvisuallod_debugger_20260828.md`. Gym-verify round verified
byte-preserved under it (baseline numstat diff). Both rounds ship together only after the
maintainer's visual pass; the debugger's dissolve alert should itself flag the still-missing
`M_CkUsf_Look_VisualLodDissolve` master until `Ck_Usf_GenerateLooks` is run.

**As of 2026-08-27 — gym-verify round (CkFoundation/CkTests/CkGameplayDebugger @ feature/ckvisuallod, UNCOMMITTED on the base gates):**
Base gates 0/1/3 done + Gate 2 partial (gym) as committed earlier (CkFoundation 5a04508c6 · CkTests
98f670ef · CkGameplayDebugger 880c822). On top, an uncommitted round fixing two maintainer
visual-check bugs: (1) **A-pose** — promoted proxies now WALK (arbiter drives the proxy as a single
sequence mirroring the far-anim; the demo RendererData's ABP_Unarmed made Request_PlayAnimation a
no-op). (2) **Pop→dissolve** — surface-replacing CkUsf crossfade: only the proxy dissolves, over the
crowd member that stays solid as the base layer. New CkUsf `VisualLodDissolve` look (.ush + AS asset,
uniform `Visibility`), new IskmProxy `Request_SetMaterialOverride_AllSlots`, arbiter config
`_ProxyDissolveLook` (batch-resolved), per-proxy MID driven `Visibility = 1 - _FadeAlpha`.
Adversarially reviewed (3-agent workflow); 6/6 findings fixed.
**Compile gate: GREEN** — `Ck_Usf_GenerateLooks`-independent build `=== Build succeeded ===`, 0 errors,
CkVisualLod + CkIskmRenderer TUs confirmed recompiled (Saved/Logs/BuildTest.log, 2026-08-27). C++ only;
AS assets compile at editor boot, the .ush at master generation — both maintainer-side.
**Next action (MAINTAINER, editor):** run `Ck_Usf_GenerateLooks VisualLodDissolve` and commit
`Content/CkUsf/GeneratedLooks/M_CkUsf_Look_VisualLodDissolve.uasset` (else proxy pops + GeneratesUsableMasters
red under --no-nullrhi); then the one-stop visual pass: PIE `TestGyms_CkTests_Level` → Tab → "Visual Lod",
walk the band — near proxies WALK and dissolve in/out; `ck.DebugOverlay 1` for the cards.
**Next action (AGENT):** commit this round across the 3 submodules once the maintainer confirms the
visual; then Gate 2 remainder (flip-lifecycle AS autotests), then Gate 4 (BB adoption).
**Blocked on:** maintainer editor-generate + visual confirm before commit/ship.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-26 | Name: CkVisualLod, single module | Maintainer pick; split has an empty policy half | never |
| 2026-08-26 | Mechanism C++ / game policy in BB AS behind signals+config | Maintainer's stated split | a game needs per-frame policy the config can't express |
| 2026-08-26 | Per-arbiter config data asset | Domain-scoped knobs + N crowd configs | per-map variation needs override layers |
| 2026-08-26 | Viewer: observer request wins, `TryGet_LocalViewInfo` fallback | Framework norm + discharges the CkCamera chip | split-screen support work |
| 2026-08-27 | Member→arbiter by gameplay tag, lazy resolve, handle request overrides | Kills spawn-path handle plumbing | tag ambiguity shows up in practice |
| 2026-08-27 | No persistence handler (v3 rebuild+hydrate; all state derived) | Maintainer confirmed | LOD state ever becomes authored |
| 2026-08-27 | Exhaustion policy is per-arbiter config (FallbackPromote / StayFar_Ensure) | Unifies roster/ambient fork | — |
| 2026-08-27 | Budget accounting: near / lock / unbudgeted counted separately | Fixes BB budget-inflation defect | — |
| 2026-08-27 | BB view-ranked promotion change = design intent only (uncommitted, compile-unverified) | Maintainer confirmed unverified; C++ port carries its own tests | BB lands + verifies it first |
| 2026-08-27 | **Agent runs NO test suites this campaign** — compile-only gates; tests are authored but maintainer-run; runtime behavior verified visually by maintainer via [EDITOR-VERIFY] lists | Maintainer: "Don't run gauntlets or other tests. It takes too long." | maintainer lifts it |
| 2026-08-27 | Signals carry (handle, memberIndex); crowd read via Get_Crowd at handler time | No precedent for raw AActor* in replayable signal payloads; dangling-on-replay risk | — |
| 2026-08-27 | Promote locks = immediate mutators (Timer's ChangeCountDirection shape), not deferred requests | Counter bump with no side effects; arbiter evaluates next tick either way | — |
| 2026-08-30 | Renderer-data assets are the shared SKMC/GPU render profiles | Reuses the existing rendering/culling/lighting surface and avoids a duplicate profile asset type | a non-skeletal renderer needs the same contract |
| 2026-08-30 | GPU members retain one logical index and rebucket by `(tile, profile)` | Shadow/material/pass state is primitive-wide; a per-instance bit cannot remove shadow or lighting work | engine gains verified per-instance pass filtering |
| 2026-08-30 | Ordered outward thresholds + per-band return hysteresis | Prevents profile churn while keeping authoring deterministic across multiple bands | a game needs non-distance importance in the classifier |
| 2026-08-30 | No engine fork or shadow-only replacement mesh in Gate 4 | Higher MinLOD + contact/dynamic-shadow policy supplies the measured first “cheap” tier | profiling proves replacement geometry earns its extra primitive |

## Dated entries (append-only, newest first)

### 2026-08-30 — Gate 4 implementation + automated evidence
- Wrote: complete renderer-profile application for pooled base/child SKMCs and GPU clusters;
  stable `(tile, profile)` crowd buckets; fail-closed pre-add profile validation; member profile
  migration preserving identity, transform, animation phase, custom data, and visibility;
  animation update intervals/freeze; velocity, ray-tracing, material, culling, shadow, lighting,
  and pass controls.
- Wrote: ordered VisualLod render bands with strict validation, outward thresholds, inward
  hysteresis, multi-band teleport handling, rooted soft-profile loads, and cached active band.
- Wrote: debugger profile/bucket/member-band visibility; three-band gym; C++ boundary/profile
  overwrite tests; AS hidden/outward/inward lifecycle test and generated AutoTests map actor.
- Ran: final incremental Development Editor build -> `=== Build succeeded ===`.
- Ran: fresh discovery + D3D12/SM6 `VisualLod` filter -> Total 10, Passed 10, Failed 0,
  Skipped 0, Contaminated 0. The discovery process logged generated spawn-param full-reload
  diagnostics after regeneration; the fresh test process loaded the new scripts and had no AS
  compile failure.
- Not claimed: pixel-level shadow/material quality or a quantified performance delta. The AS
  harness has no isolated GPU/render-thread query; use N>=3 rendered Insights/GPU Profiler
  windows rather than whole-frame AutoTest duration.

### 2026-08-30 — Gate 4 entry: range render profiles
- Confirmed: GPU shadow/material/pass state is primitive-wide; `CkIskm_BatchedClusterProxy`
  emits one mesh-batch family for all instances in a cluster.
- Confirmed: `UCk_IskmRenderer_Data` already declares rendering/culling/lighting options but the
  checked-out renderer has no consumers for those getters.
- Confirmed: the stable member-index contract rules out moving members between crowd actors.
- Pattern selected: `CkDebugScene_Target.cpp` bucket-key plus prepare/commit/rollback reconcile.
- Baseline: Development Editor build stopped at 23/588 without compiler diagnostics; tests did not
  run. See the dated scratch snapshot and `Saved/Logs/VisualLodProfiles_Baseline.log`.
- Inferred until final rendered gate: authored unlit/cheap materials and disabled shadow passes
  reduce GPU cost; no performance delta will be claimed without the required A/B measurement.

### 2026-08-27 — Gym-verify round: A-pose fix + surface-replacing CkUsf dissolve
Two bugs from the maintainer's PIE pass ("close ones are in A pose, not playing walk"; "dissolve is
a pop, not a fade"). All uncommitted on top of the base gates.
- **A-pose (CkVisualLod).** Root cause: the demo RendererData wires `ABP_Unarmed` → the promoted
  proxy boots in AnimBP pose mode, where `Request_PlayAnimation` is IGNORED (CkIskmProxy_Processor.cpp:664),
  and the ABP has no velocity to read. Fix (maintainer's steer: "the default can get a single anim
  sequence — no heavy AnimBP"): `DoDrive_ProxyAnim` switches the proxy to sequence mode and plays the
  same AnimCollection sequence the far-anim resolves to (Fixed idx / SpeedDriven idle-move), looping
  at the far rate; re-issued on far-anim change; a game overriding in `OnPromoted` still wins by FIFO.
  Added `_ProxySequenceIndex`/`_ProxyRate` cache + `_Collection` on the crowd runtime.
- **Dissolve (CkUsf + CkIskmRenderer + CkVisualLod).** Maintainer picked "surface-replacing dissolve
  via CkUsf". Recon (7-agent workflow) mapped the Look pipeline + both render seams. Key insight: the
  crowd member already stays SOLID through the fade (nothing reads its fade slot), so dissolving ONLY
  the proxy over it gives a clean two-way crossfade with NO whole-crowd-material tradeoff (crowd
  override is per-crowd, not per-member). Built: `VisualLodDissolve.ush` (external `Visibility` scalar,
  masked erosion + gated burn edge) + AS `UCkUsf_LookDefinition` (`_UsedWithSkeletalMesh`); IskmProxy
  `Request_SetMaterialOverride_AllSlots` (live MID → every base-SKMC slot; slot count resolved
  handler-side post-Setup); arbiter config `_ProxyDissolveLook` (soft ref, rooted-batch resolved on
  Current); per-proxy dissolve MID (`TStrongObjectPtr` on member Current) applied on fade-start,
  driven `Visibility = 1 - _FadeAlpha`, cleared to reveal the real material when solid.
- **Adversarial review (3-agent workflow):** 6 findings, ALL fixed — stale-crowd block now clears the
  dissolve; `LoadSynchronous`→rooted batch (`DoEnsure_DissolveLook`); file-local `static FName`→named
  namespace; `.ush` burn-edge gated to active fade (no residual emissive on a solid proxy);
  `CK_PROPERTY_GET` on the weak-ptr request field; tier-table/Depends doc drift. Review also verified
  fade math end-to-end and all teardown paths.
- **Compile gate GREEN** (build-only, `--generate`): `=== Build succeeded ===`, 0 errors; CkVisualLod +
  CkIskmRenderer TUs recompiled (Saved/Logs/BuildTest.log). AS + shader + runtime remain maintainer-side.
- **[EDITOR-VERIFY / REQUIRED]** run `Ck_Usf_GenerateLooks VisualLodDissolve` + commit
  `Content/CkUsf/GeneratedLooks/M_CkUsf_Look_VisualLodDissolve.uasset` (else dissolve pops — graceful —
  AND `CkTests.UnitTests.CkUsf.GeneratesUsableMasters` is red under --no-nullrhi). Then PIE the gym:
  near proxies WALK and dissolve in (approach) / out (retreat) over the solid far member.
- Files: CkFoundation — `CkUsf/Shaders/CkUsf/Looks/VisualLodDissolve.ush`, `Script/CkUsf/CkUsf_Looks_Assets.as`,
  `CkIskmRenderer/Proxy/CkIskmProxy_{Fragment_Data,Fragment,Processor.h,Processor.cpp,Utils.h,Utils.cpp}`,
  `CkVisualLod/*` (Build.cs +CkUsf, config field, fragment MID, arbiter processor helpers + hooks),
  docs (module CLAUDE.md, Source/CLAUDE.md, CkIskmRenderer CLAUDE.md). CkTests — `CkVisualLod_GymAssets.as`.

### 2026-08-27 — Gym (Gate 2) + debugger providers (Gate 3)
- Wrote: **"Visual Lod" gym** (CkTests feature/ckvisuallod @ 98f670ef) — one station, 40 orbiting
  members (Fixed walk far-anim), arbiter near-budget 5, 900/1300 band, NO observer wired (proves
  local-view discovery). Reuses iskm demo assets; direct `UCk_Utils_*` calls (fresh-binding rule).
  AS compile-gated via ONE minimal headless boot (`--test-pattern IntRange`, 3 trivial unit tests
  — disclosed vs the no-tests directive; 0 AS errors; 3 accessor-ambiguity warnings fixed with
  Set_ accessors).
- Wrote: **Gen 3 overlay providers** (CkGameplayDebugger feature/ckvisuallod @ 880c822) — member
  card (representation+slot, HIDDEN flag, mid-fade alpha, locks at Warn; `LOD:P/F/-` pills) +
  arbiter card (near/locked/unbudgeted counters, observer mode). In the "All" layout.
- Ran: builds ×3 → `=== Build succeeded ===`, 0 error lines. Fixes: `ECk_DebugOverlay_Severity::Warn`
  (not `Warning`); direct `CkIskmRenderer` dep on CkEntityDebugOverlay (transitive-only link left
  the AS signal-payload thunk imports unresolved — CkVisualLod's headers instantiate them in
  consumer TUs).
- **[EDITOR-VERIFY] (one pass covers Gates 0-3):** PIE `Content/TestGyms/TestGyms_CkTests_Level`
  → Tab → "Visual Lod" → walk in/out of the crowd: ranked crossfade promotes, dissolve demotes,
  preemption while strafing; `ck.DebugOverlay 1` → member cards show the VisualLod section +
  arbiter card shows budgets; no ensures naming VisualLod in the log.
- Remaining Gate 2 item: flip-lifecycle AS autotests (acquire→promote→fade→demote→release,
  hidden round-trip, locks, suspend/resume, signal order) — authored next session, maintainer-run.

### 2026-08-27 — Gate 2 first half: local-view discovery (compile-green)
- Wrote: `UCk_Utils_Camera_UE::TryGet_LocalViewInfo` (CkCamera — surgical addition): first
  locally-player-controlled camera entity's composed FMinimalViewInfo, PCM fallback with real
  `GetFOVAngle()` (BB assumed 90°), false on dedicated server/editor worlds (null-safe
  `Get_PrimaryPlayerController`). **Discharges the standing "upstream TryGet_LocalCamera into
  CkCamera" chip.** Arbiter `DoResolve_View` now falls back to it when no observer is wired.
- Ran: toolbox `--build` ×2 → `=== Build succeeded ===`, 0 error lines. Fix: const-handle
  `View<>` with lifetime filters doesn't convert — mutable handle copy (house call sites agree).
- Remaining for Gate 2: CkTests gym + AS autotests (next session; own submodule + skill load).

### 2026-08-27 — Gate 1 landed (mechanism, compile-green)
- Wrote: the full flip driver in `FProcessor_VisualLodArbiter_Update` — shadowed `DoTick`
  (snapshot arbiters/members from `Handle.View<>`, mechanism runs OUTSIDE live iteration because
  promotes create scene-node entities; a never-dispatched `ForEachEntity` satisfies the CRTP
  contract and ensures loudly if the shadow is removed; `_LastVisitedCount` written for pump
  truth). Ported all BB semantics: stale-crowd invalidation, hidden handling, lazy crowd stand-up
  (rooted-batch collection load, `OnCrowdCreated` fires synchronously post-Finalize for the game's
  material push), slot pools + owner-checked recycle, lock-gated inline promotes (lock budget),
  exhaustion policies (PromoteInstead / Unrendered+ensure), distance-only demote, whole-domain
  ranked flips, all inc-⑤ fade semantics (dissolve-before-visible, reversals, preempt suppression,
  lock override, hidden-snap, member-vanish), fail-closed recovery (generalizes ambient
  Recover_NearFailure; fires OnDemoteFinishing so the game re-registers far cosmetics), async
  renderer loads (cold promote defers, stays candidate), three-counter budget accounting
  (near/locked/unbudgeted — fixes the BB budget-inflation defect), deterministic EndPlay release
  + refund, amortized sweep as reconciliation.
- Wrote: 5 ranking automation tests (`Ck.CkVisualLod.Ranking.*` in CkVisualLod_Ranking.spec.cpp)
  — expectations hand-derived incl. partial-sort truncation and preempt-cursor blocking. NOT run
  (standing directive).
- Ran: toolbox `--build` ×2 → `=== Build succeeded ===`, 0 error lines (log read directly).
  Fix: ck_exp::TProcessor requires ForEachEntity to exist even under a DoTick shadow.
- Confirmed: signal Broadcast is synchronous (CkSignal_Utils.inl.h:144-181) — the
  acquire-before-visible and promote/demote signal windows are sound.
- Deliberate deltas from BB (recorded): promote defers on a cold renderer instead of
  LoadAsset_Blocking; `As_Presentation` registration is game-side (bind OnPromoted); crowd slot
  override materials are game-side via OnCrowdCreated; sweep is per-member-call cadence like BB.
- **[MAINTAINER-RUN] when convenient:** `--test --test-pattern VisualLod` (5 ranking tests; needs
  the relink that just happened — new automation tests need fresh binaries).
- **[EDITOR-VERIFY] (Gates 0+1 consolidated):** (1) editor boots with no ensures naming VisualLod;
  (2) BP palette shows [Ck][VisualLod]/[Ck][VisualLodArbiter] nodes + `<AsVisualLod>` autocast;
  (3) AS: `utils_visual_lod` / `utils_visual_lod_arbiter` resolve after script regen. Behavior
  verification (promote ring, crossfades, budgets) lands with the Gate 2 gym.

### 2026-08-27 — Gate 0 landed (scaffold + data surface, compile-green)
- Wrote: module boilerplate (Build.cs/CkModuleRules, Module, Log), member + arbiter quartets
  (fragments, config asset `UCk_VisualLodArbiter_Data`, requests, 4 signals, utils, 8 registered
  processor skeletons), pure ranking port (`CkVisualLod_Ranking.h/.cpp`), uplugin entry,
  Source/CLAUDE.md tier row, module CLAUDE.md.
- Ran: toolbox `--build` ×3 → final `=== Build succeeded ===` (log tail read directly; 0 error
  lines). Fix 1: `auto Worst = INDEX_NONE` deduced the unnamed-enum type → cast to int32.
  Fix 2: `Has_Any` declared but not covered by `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` →
  hand-defined forwarding to `Has` (Timer defines its own against its record; direct-attach
  features forward).
- Confirmed: 8/8 `CK_REGISTER_PROCESSOR` lines (grep). Arbiter Setup implements the rooted-batch
  config load (consumer id `"VisualLodArbiter.Setup"`); member/arbiter HandleRequests drain with
  completion guards; cancel processors fire `Failed_Cancelled`.
- Inferred (unconfirmed, [EDITOR-VERIFY]): editor boots clean with the new module; BP/AS surface
  renders. Maintainer checks visually per standing directive.
- Implemented beyond skeleton (deliberate, low-risk): request handlers that only write state
  (arbiter cache, hidden latch, far anim, renderer override, suspend tags), lock immediate
  mutators, ranking implementation. Mechanism (pools/flips/fades) remains Gate 1.

### 2026-08-27 — Design phase complete
- Read: all four BB spec-by-example files + both BB design docs (cosmetic parity, fade inc-⑤) +
  BB test inventory; survey of CkIskmRenderer/CkCamera/CkVisibleRange APIs and module conventions
  (tier table, quartet, retired ProcessorInjector, signal macro, `UCk_DataAsset_PDA`).
- Confirmed: no name collision for CkVisualLod (Source/ sweep); CkIskmRenderer owns all mechanism
  APIs; no local-viewer discovery util exists anywhere (Compass/Minimap take caller-supplied
  observers); `CK_REGISTER_SNAPSHOTABLE` removed (root CLAUDE.md) so persistence = register nothing.
- Confirmed: BB worktree (`E:\Repos\BusterBlock_Other`) still holds the uncommitted ranking change,
  detached at 9811603ae; maintainer states it is still compile-unverified.
- Maintainer settled: name, policy seam, config surface, viewer resolution, domain-tag reference,
  persistence posture (in-session Q&A, logged above).
- Wrote: DESIGN_CkVisualLod.md, PROMPT.md, PLAN.md, this file. Nothing committed — awaiting review.
- Follow-ups recorded, not chased: signal synchronous-delivery verification is Gate 1's first
  work item (fallback: arbiter-level configure-member delegate, maintainer consulted first).

## Open items
| Item | Status | Next step |
|---|---|---|
| Maintainer review of DESIGN_CkVisualLod.md | Closed 2026-08-27 | Waived line-review; "proceed with implementation" — in-chat design summary approved |
| Signal in-tick delivery guarantee (acquire-before-visible) | **Resolved 2026-08-27** | Confirmed from source: `TUtils_Signal::Broadcast` publishes to bound delegates inline (CkSignal_Utils.inl.h:144-181). Synchronous; no fallback needed |
| Campaign branch + Gate 0 contract | Closed 2026-08-27 | feature/ckvisuallod cut; Gate 0 landed a6a2ef14c |
| Gate 0 [EDITOR-VERIFY] (editor boot + BP/AS surface) | Open (maintainer) | See Gate_00_Scaffold.md exit list |
| Gate 1 promote-path entity creation vs registry-locked iteration | Open | Deferred-creation idiom per CkEcs doctrine; survey in flight |

**Rule: no completion claim may be written anywhere in this file while any row here is unresolved.**
