# PROGRESS.md — CkSnapshot Rebuild+Hydrate campaign

> Executor: update this file at the END of every session. Next session trusts THIS file over memory.
> Blockers go in §Blockers — never improvise around one.

## Status board

| Phase | Doc | Status | Session date | Commits (repo: hash) | Gate result |
|---|---|---|---|---|---|
| 0 | PHASE_0.md | DONE | 2026-07-11 | CkF: 68ba192dc (dt==0), 55521d493 (oracle), <docs> (this); CkTests: 14d65ac (harness) | GREEN: Ck.Snapshot 47/47/0 (46 baseline + Oracle.StructuralBaseline), Ck.Attribute.Net 17/17/0, Net 102/101/1 (baseline red only) |
| 1 | PHASE_1.md | CORE DONE (1.1–1.5) committed+GREEN; **1.6 remaining** (oracle Tier-2 + ProduceDiffBaseline test) | 2026-07-11 | CkF: d7956345a (CkEcs framework), 4a6839afb (6 migrations+deletions) | GREEN: Ck.Snapshot 47/47/0 delta-zero (all 11 Parity_MPReload pass), Net 102/102/0 |
| 2 | PHASE_2.md | NOT STARTED | | | |
| 3A | PHASE_3A.md | NOT STARTED | | | |
| 3B | PHASE_3B.md | NOT STARTED | | | |
| 4A | PHASE_4A.md | NOT STARTED | | | |
| 4B | PHASE_4B.md | NOT STARTED | | | |
| 5 | PHASE_5.md + VALIDATION.md | NOT STARTED | | | |

## Unattended execution protocol (set 2026-07-11 by Adam — OVERRIDES the "STOP on divergence" default below)

Run the campaign UNATTENDED through all remaining phases: finish §1.6, then Phases 2 → 3A → 3B → 4A → 4B → 5 in
order. Per-phase loop: read PHASE_N.md (+ the spec and PHASE_1_RESEARCH.md), implement on Opus, gate via
UnrealToolbox (editor CLOSED; read verdicts from the --output logs), and when the gate is GREEN + delta-zero vs the
baselines, COMMIT that phase (never push; stage only files you changed by name) + update this file, then proceed to
the next phase automatically. Do not stop between green phases.

QUESTIONS / DIVERGENCES → delegate, don't halt. When a step is ambiguous, or reality diverges from the plan, and the
answer is NOT already in the PHASE docs / spec / PHASE_1_RESEARCH.md: launch a **Fable-class agent** (Agent tool,
`model: "fable"`) to research the codebase + decide the question (Fable has the reasoning depth for design/architecture
forks), VERIFY its ruling against the cited code yourself, record it in §Decisions here, then RETURN TO OPUS to
implement. Never improvise architecture on the Opus main loop — route every design fork through a Fable agent.
Reserve a true STOP (→ §Blockers, end the run) ONLY for: (a) a red gate you cannot fix against the plan/research even
after a Fable consult; (b) an irreversible/outward/destructive action (push, force-push, cross-repo merge, deleting
another session's work); or (c) a genuine human-only product/risk/authority decision the Fable agent explicitly flags
as needing Adam. Cross-repo/CkTests-ahead-of-CkFoundation discipline and "no push" still hold absolutely.

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

- **[P1-D1] §1.1 include-surface CORRECTION.** The blessed ".inl.h at the bottom of CkNet_Utils.h" design FAILS —
  CkNet_Utils.h is UHT-reflected and UHT forbids any `#include` after its `.generated.h`. Fix: `RegisterLazyTyped<T>`
  body lives in `CkReplicatedFragmentContainer.inl.h` (no includes), and each migrated registrar `.cpp` includes
  `CkNet_Utils.h` then that `.inl.h`. The Attribute registrars get both via `CkAttribute_RestorePersistence.h`.
- **[P1-D2] Attribute Produce = EMPTY-SEED (option A), not the research recipe's value-emitting option B.** The old
  attribute `ReplicateOnRestore` itself empty-seeds the owner container + re-arms Current/Min/Max
  `MayRequireReplication` (it reads NO values); `FProcessor_Attribute_Replicate` refills. So a behavior-neutral
  Model-A migration is empty-seed (shared `ck::attribute_restore::Produce/SeedContainer` in
  `CkAttribute_RestorePersistence.h`), which also sidesteps the per-owner upsert-merge risk. AnimPlan uses the same
  empty-seed shape. **Consequence for 1.6/Phase 3A:** an empty-seed Produce emits an always-empty payload, so a
  Tier-2 "mutate attribute value → diff line" test would never register a change; the value-emitting Attribute
  Produce is a Phase-3A/save-path concern. The 1.6 `ProduceDiffBaseline` test should exercise a value-emitting
  feature (**Velocity** — `Produce` emits `FCk_RepData_Velocity{Get_CurrentVelocity()}`), not an attribute.
- **[P1-D3] 1.3 re-drive done-marker.** `FTag_Snapshot_JustRestored` is added at `CkSnapshot_Restore.cpp:243` and
  NEVER removed (persists). So the re-drive views on it, populates `FFragment_Persistence_ReDrivePending._Remaining`
  on first sight, drains per-tick, and LEAVES the emptied fragment as the done-marker (does not re-populate). Gate =
  each `SeedContainer`'s own driver/owner check returns `NotAdded`→retry; 5s/2s timeout → loud drop.
- **[P1-D4] 1.5 net dispatcher NOT refactored.** Extracted `ck::persistence_apply::ApplyOne` (resolve+Apply+timeout)
  used by the new dormant `FProcessor_Hydration_Dispatch`; left the tested `FProcessor_ReplicatedFragments_Dispatch`
  inline to avoid regressing the green Net/Parity gate for a dormant feature (a future cleanup can adopt ApplyOne).
  Hydration processor view needs `ck::TReadWrite<FFragment_PendingHydration>` (ck_exp::TProcessor static_assert).

## Decisions — planner rulings

- **[P1-R1] (2026-07-11, planning session — resolves [B1]): option (c), refined to a 6/6 split.** Phase 1 migrates
  ONLY the clean six (Velocity, Acceleration, Attribute×5, TagSet, MontagePlayer, AnimPlan — 10/16 registrations);
  Team + Player join the DEFERRED set alongside Inventory×2, RenderTarget, 2dGridOccupancy (their unconditional
  `FTag_TeamID/PlayerID` re-derive is the same non-container-reconstitution class, just smaller — the research
  table's own "UNCONDITIONALLY, pre-driver-gate" note contradicts a driver-gated SeedContainer home). Deferred
  processors stay VERBATIM: their repair work is Model-A-only and Model B retires it structurally (items→recipes,
  grids→Construct, RT→Construct+Phase-4B re-author, Team/Player tags→normal Assign in hydration Apply); they go
  inert at 3B (v3 never stamps JustRestored) and are deleted in Phase 5. Options (a) slimming and (b) a
  `Reconstitute` hook are REJECTED: churn without end-state value / framework surface that Phase 3B makes dead.
  New participation rule (encoded in PHASE_1.md + PHASE_3A.md §3A.4): `SeedContainer` present ⇒ handler joins the
  Model-A re-drive; `Produce`-without-`SeedContainer` ⇒ capture/oracle-only (how the deferred six gain Produce at
  3A with zero double-seed risk). The research doc's §1.1 `.inl.h`-at-bottom-of-CkNet_Utils.h include decision is
  BLESSED — re-apply verbatim. PHASE_1/3A/3B/5 docs revised accordingly (same commit as this entry).

## Blockers

- **[B1] — RESOLVED 2026-07-11 by [P1-R1] above.** Original text kept below for the record.
- **[B1] (2026-07-11, Phase 1) — Plan's "12 ReplicateOnRestore = deletable container re-seeds" is false for 4
  features; where their non-container reconstitution goes is an unmade architecture decision.** REQUIRES a design
  ruling before Phase 1 can proceed (executor may not improvise architecture — PROMPT line 5).
  - **What diverges:** PHASE_1.md §1.2/§1.4 assume every `*_ReplicateOnRestore` processor only re-seeds the
    replication container from live state, so `Produce`/`SeedContainer` (§1.1) replaces them and §1.4 deletes them
    behavior-neutrally under Model A. Verified against code, that holds for 8 features but **Inventory Spatial,
    Inventory DataOnly, RenderTarget, and 2dGridOccupancy** restore processors ALSO do non-container reconstitution:
    child-entity re-replication (`Request_TryReplicateExisting`), grid re-stamp (`Request_PlaceItemOnGrid`) gated on
    CkGrid's `FProcessor_2dGridSystem_RestoreRecompose`, render-target re-create + repaint (`DoApplyBatch`), and an
    unconditional derived-fragment re-seed for local-only grids. The generic re-drive (§1.3) and Produce/SeedContainer
    cannot house this (cross-processor ordering; Produce-before-Seed circularity for Inventory; unset-Produce-skips-
    Seed for local grids; once-only repaint vs per-retry SeedContainer). Deleting these 4 per §1.4 breaks restore
    under Model A. HAND-VERIFIED: `CkInventory_Spatial_Processor.cpp:62-175`. Others agent-cited (consistent pattern).
  - **Decision needed (maintainer/CTO — do NOT pick unilaterally):** (a) keep slimmed restore processors for these 4
    doing only reconstitution + Produce for the payload; (b) add a `Reconstitute(Entity)` re-drive hook (unconditional,
    retry, separate from the container path); or (c) migrate only the 8 clean features in Phase 1 and DEFER the 4 to
    Phase 3/4 (Model B's Construct-rerun may moot most of it; interacts with CTO-addendum N1 + PHASE_4B RenderTarget).
  - **Full analysis + the 8 ready-to-implement recipes + the (reverted, re-usable) §1.1 design:**
    `PHASE_1_RESEARCH.md`. Once resolved, §1.1 re-applies verbatim; the 8 clean migrations follow the recipe table.
  - **Repo state:** clean at the gated-green Phase-0 boundary (CkFoundation `e5ffe028d`). No Phase-1 code landed;
    the §1.1 scaffolding was implemented then reverted (never build-verified) to keep the boundary clean.

## Session log

- 2026-07-11 — package authored (planning session, Fable). Branch `feature/save-load-improvements` @ `bc484d645`.
- 2026-07-11 — Phase 0 execution (Opus). Baselines captured @ `bbde1a9dd` (see table). Census re-derived
  (127/20/23). Decisions P0-D1..D3 recorded. Applied 3 dt==0 guards + `CK_WITH_FIDELITY_ORACLE` define + oracle
  Tier-1 (.h/.cpp) + harness test + spec §2 census reconciliation. **Gate GREEN, delta-zero:** Ck.Snapshot
  46→47 (new `Ck.Snapshot.Oracle.StructuralBaseline` PASSED, `--discover-fresh`), Ck.Attribute.Net 17/17/0,
  Net 102/101/1 (same pre-existing red). Build clean; no AngelScript errors naming campaign files;
  `rg CK_WITH_FIDELITY_ORACLE Source | wc -l` = 8 (≥3). Commits: CkF `68ba192dc`,`55521d493`,`<docs-this>`;
  CkTests `14d65ac` (on `dev`, unpushed — where the sibling snapshot tests live). Nothing pushed. Phase 0 DONE.
- 2026-07-11 — Phase 1 attempt (Opus, same session). Ran a 10-agent read-only census of all `*_ReplicateOnRestore`
  processors (recipes in PHASE_1_RESEARCH.md); designed + implemented §1.1 registry extension (Produce/SeedContainer/
  Transport/RegisterLazyTyped; include-surface decision made). **STOPPED on divergence [B1]** — 4 of the 12 restore
  processors do non-container reconstitution the plan's model can't house; where it goes is an unmade architecture
  decision (executor may not improvise). Reverted the unbuilt §1.1 scaffolding; tree back at gated-green Phase-0
  boundary. No Phase-1 code committed. Awaiting a design ruling on [B1] (see Blockers). Session ends here per the
  campaign's divergence rule.
- 2026-07-11 — Phase 1 RE-EXECUTION (Opus) after [B1] resolved by [P1-R1]. Implemented + COMMITTED the core:
  §1.1 registry contract, §1.3 generic re-drive, §1.2+§1.4 all SIX clean-feature migrations + deletions, §1.5
  dormant hydration queue. Two commits on `feature/save-load-improvements`: **d7956345a** (CkEcs framework),
  **4a6839afb** (6 migrations + deletions). **Gate GREEN:** Ck.Snapshot 47/47/0 delta-zero (all 11
  Parity_MPReload pass — proves the generic re-drive == the deleted per-feature processors), Net 102/102/0 (the
  baseline's lone red is a pre-existing flake, green this run). Executor decisions [P1-D1..D4] recorded above (esp.
  P1-D2: empty-seed Attribute Produce ⇒ 1.6 ProduceDiffBaseline must use a value-emitting feature like Velocity).
  Hit + fixed: UHT-forbids-include-after-generated.h (.inl.h moved to registrar .cpp), ck_exp::TProcessor needs
  TReadWrite on the hydration fragment. **REMAINING: §1.6** (oracle Tier-2 Capture_Payloads/Diff_Payloads +
  Ck.Snapshot.Oracle.ProduceDiffBaseline test) — the ONLY unfinished Phase-1 step. Nothing pushed. Handed off
  mid-phase (context full) with a continuation prompt.
