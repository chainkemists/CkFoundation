# CkVisualLod campaign — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-27 (CkFoundation @ 5d1ac9e83, detached — no campaign branch cut yet):**
Design written ([DESIGN_CkVisualLod.md](DESIGN_CkVisualLod.md)) and awaiting maintainer review of
the written spec. No code exists.
**Baseline being diffed against:** none yet — captured at Gate 0 entry (full-suite counts before
scaffold lands).
**Next action:** maintainer reviews the design doc; on approval cut `feature/ckvisuallod` in
CkFoundation, author `Plan/Gate_00_*.md`, scaffold.
**Blocked on:** maintainer spec review.

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

## Dated entries (append-only, newest first)

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
