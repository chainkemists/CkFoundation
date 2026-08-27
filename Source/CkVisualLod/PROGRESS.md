# CkVisualLod campaign — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-27 (CkFoundation @ feature/ckvisuallod, docs commit a9283eed9):**
Design approved without line-review (maintainer: "proceed with implementation"). Gate 0 scaffold
fully written (module + data surface + utils + processor skeletons + uplugin entry + tier row);
compile gate in flight.
**Baseline being diffed against:** build `Result: Succeeded` (754s, full rebuild of stale worktree
binaries) at a9283eed9 / code tip 5d1ac9e83, 2026-08-27. **No test baseline** — maintainer
directive (see decision log): the agent runs no test suites this campaign; compile gates +
maintainer visual checks replace them.
**Next action:** Gate 2 remainder — flip-lifecycle AS autotests (CkTests) — then Gate 4
(BB adoption). The maintainer's one-stop visual pass: PIE `TestGyms_CkTests_Level`, Tab →
"Visual Lod", walk the band; `ck.DebugOverlay 1` for the member/arbiter cards.
**Blocked on:** nothing agent-side. Maintainer items in the 2026-08-27 entries below.

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

## Dated entries (append-only, newest first)

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
