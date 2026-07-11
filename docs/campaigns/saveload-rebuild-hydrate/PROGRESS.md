# PROGRESS.md — CkSnapshot Rebuild+Hydrate campaign

> Executor: update this file at the END of every session. Next session trusts THIS file over memory.
> Blockers go in §Blockers — never improvise around one.

## Status board

| Phase | Doc | Status | Session date | Commits (repo: hash) | Gate result |
|---|---|---|---|---|---|
| 0 | PHASE_0.md | DONE | 2026-07-11 | CkF: 68ba192dc (dt==0), 55521d493 (oracle), <docs> (this); CkTests: 14d65ac (harness) | GREEN: Ck.Snapshot 47/47/0 (46 baseline + Oracle.StructuralBaseline), Ck.Attribute.Net 17/17/0, Net 102/101/1 (baseline red only) |
| 1 | PHASE_1.md | NOT STARTED | | | |
| 2 | PHASE_2.md | NOT STARTED | | | |
| 3A | PHASE_3A.md | NOT STARTED | | | |
| 3B | PHASE_3B.md | NOT STARTED | | | |
| 4A | PHASE_4A.md | NOT STARTED | | | |
| 4B | PHASE_4B.md | NOT STARTED | | | |
| 5 | PHASE_5.md + VALIDATION.md | NOT STARTED | | | |

## Phase-0 baseline table (fill in Phase 0; every later phase diffs against THESE names)

Captured 2026-07-11 on `feature/save-load-improvements` @ `bbde1a9dd` (clean tree, no edits), via UnrealToolbox
`--build --test` (Editor/Development). Verdicts read from `CkAuto/logs/p0-baseline-*.log`.

| Pattern | Total | Pass | Fail | Failing test names |
|---|---|---|---|---|
| Ck.Snapshot | 46 | 46 | 0 | (none) |
| Ck.Attribute.Net | 17 | 17 | 0 | (none) |
| Net | 102 | 101 | 1 | Ck.StateMachine.Net.OwningClientAuth_SubSm_AuthorityGatedTask |
| (full suite) | not run this phase | | | (VALIDATION.md runs full suite at Phase 5) |

The lone `Net` red is PRE-EXISTING (zero source edits at capture time). Every later phase re-running `Net` must show
exactly this one failure by name and no others.

Census (invocation-only, Phase 0, 2026-07-11): CK_REGISTER_SNAPSHOTABLE = **127** across **20** modules
(CkAnimation, CkAttribute, CkDynamic, CkEcs, CkEcsExt, CkEntityCollection, CkEntityTag, CkGrid, CkInteraction,
CkInventory, CkLabel, CkObjective, CkPhysics, CkRelationship, CkRenderTarget, CkSnapshot, CkSpatialQuery,
CkStateMachine, CkTagSet, CkTimer); FCk_RepData_* struct decls = **23**. (Spec §2's "119 / 18 modules / 24 RepData"
reconciled to these in the same commit.)
Load-time baseline (Phase 3B, representative fixture): ___ ms.

## Campaign-added tests (protected inventory — grows as phases land)

| Test name | Added in | File |
|---|---|---|
| Ck.Snapshot.Oracle.StructuralBaseline | Phase 0 | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_Oracle_StructuralBaseline.cpp` |

## Decisions made by executors (anything the plan left as A-or-B, with which was taken and why)

- **[P0-D1] Homing dt==0 fix lives in `CkProjectile`, not `CkPhysics`.** PHASE_0 §0.3b cites
  `Source/CkPhysics/.../Homing/CkHoming_Processor.cpp`; the Homing feature actually lives at
  `Source/CkProjectile/Public/CkProjectile/Homing/`. Same fix, real file — a doc path typo, not an architecture
  change. Guard placed at the top of `FProcessor_Homing_Update::ForEachEntity` (covers the finite-diff divide at
  `:236` AND, by returning before `Compute_HomingAcceleration`, prevents ProNav's existing `InDeltaT > 0` ensure
  from firing during a settle pass — so no `CkHoming_ProNav.cpp` edit is needed, matching the plan's "one guard at
  the top covers the ProNav calls it makes").
- **[P0-D2] Oracle `_LabelPath` left empty (tier fence); label-keyed identity deferred to Phase 3B.** PHASE_0 §0.5
  specifies the label path via `UCk_Utils_GameplayLabel_UE`, but the oracle lives in **CkEcs** and that util is in
  **CkLabel**, which *depends on* CkEcs — so the reverse include edge is impossible (would break the tier direction
  the same step's fence insists on: "keep the oracle in CkEcs"). `FFragment_LifetimeOwner` IS in CkEcs, but without
  CkLabel there is no stable per-entity label text to build a path from. Decision: Tier-1 signatures use the
  fragment/tag set (fully implemented) + `_ScriptClassPath` (CkEcs-visible EntityScript fragment); `_LabelPath`
  stays empty. The Phase-0 harness fixture carries no labels, so this does not affect the gate. **Forward note for
  Phase 3B:** when label-keyed cross-rebuild identity actually matters, supply the label via a CkEcs-visible hook
  (registered label-provider callback) or perform the label step from a higher tier — do not add a CkEcs→CkLabel
  edge.
- **[P0-D3] Tier-1 lumps tags into `_FragmentTypeNames`; `_TagTypeNames` left empty** — explicitly permitted by
  §0.5 ("put everything in `_FragmentTypeNames` … Tier-1 only needs stable signatures, not taxonomy"). Unregistered
  storage types are keyed by their entt type-hash (hex) to avoid the non-null-terminated `string_view`→`FString`
  hazard; registered types use their clean `_DisplayName`.

## Blockers

- (none)

## Session log

- 2026-07-11 — package authored (planning session, Fable). Branch `feature/save-load-improvements` @ `bc484d645`.
- 2026-07-11 — Phase 0 execution (Opus). Baselines captured @ `bbde1a9dd` (see table). Census re-derived
  (127/20/23). Decisions P0-D1..D3 recorded. Applied 3 dt==0 guards + `CK_WITH_FIDELITY_ORACLE` define + oracle
  Tier-1 (.h/.cpp) + harness test + spec §2 census reconciliation. **Gate GREEN, delta-zero:** Ck.Snapshot
  46→47 (new `Ck.Snapshot.Oracle.StructuralBaseline` PASSED, `--discover-fresh`), Ck.Attribute.Net 17/17/0,
  Net 102/101/1 (same pre-existing red). Build clean; no AngelScript errors naming campaign files;
  `rg CK_WITH_FIDELITY_ORACLE Source | wc -l` = 8 (≥3). Commits: CkF `68ba192dc`,`55521d493`,`<docs-this>`;
  CkTests `14d65ac` (on `dev`, unpushed — where the sibling snapshot tests live). Nothing pushed. Phase 0 DONE.
