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
**Next action:** Gate 1 (mechanism: pools, acquisition, flip, fades, budgets, whole-query ranking,
EndPlay release) — author its gate contract first.
**Blocked on:** nothing. Maintainer [EDITOR-VERIFY] for Gate 0 (editor boot + BP surface) is open
but non-blocking for Gate 1 code work.

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
| Maintainer review of DESIGN_CkVisualLod.md | Open | Review; amendments folded in before Gate 0 |
| Signal in-tick delivery guarantee (acquire-before-visible) | Open | Verify in Gate 1 against signal machinery |
| Campaign branch + Gate 0 contract | Open | After design approval |

**Rule: no completion claim may be written anywhere in this file while any row here is unresolved.**
