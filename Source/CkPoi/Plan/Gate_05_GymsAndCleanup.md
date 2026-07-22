# Gate 5 — Gyms + cleanup + full gate

> **Status:** ✅ Done (2026-07-22 — actual: 1 session, as estimated)
> **Depends on:** Gate 4 ✅ (2026-07-22)
> **Estimate:** 1 session

## Goal

After this gate: a gym demonstrates the Player composed as a Poi via direct-attach (PROMPT success
criterion 5 — the last unproven criterion); the FULL toolbox test gate is green and diffed against
the campaign-start reference; every PROMPT success criterion is walked with named evidence; the
campaign is COMPLETE (pending only the human `[EDITOR-VERIFY]` items and the ship/push, which stay
user-gated).

## Entry criteria (verified 2026-07-22)

- [x] HEADs: CkFoundation `324e681d9`, CkTests `c25d0d5`, CkGameplayDebugger `3be8b64`, root `529c225`.
- [x] Baseline = Gate 4 exit: Poi 46/46, Compass 13/13, Minimap 15/15, VisibleRange 4/4.
- [x] Full-suite reference for the campaign-level diff: `Saved/Logs/Regate2-PostRebase-Full.log`
      (the pre-campaign full run) — deltas must be explained by this campaign's recorded reshapes.

## Locked design

### Player-as-Poi gym (criterion 5)

- Home: **CkMinimapGym** (the design doc's own example — "unlimited — always shown to self"; a
  compass bearing-to-self is degenerate, so the compass gym is NOT the demonstration vehicle).
- Composition (design doc "Direct-attach onto an existing entity" example): resolve the PLAYER's
  entity handle (not a spawned child), then `utils_poi::Add(PlayerHandle, {Category})` +
  `utils_poi_display_definition::Add(PlayerHandle, {Consumer=Poi.Consumer.Minimap, Priority high})`
  + `utils_visible_range::Add(PlayerHandle, MaxRange 0)` (unlimited).
- **Branch on the unknown:** if the gym pawn does not already host an entity, compose the
  actor↔entity bridge the house way (`Ck Entity Script (With Actor)` /
  `Request_SpawnEntityScript_OnActor` — authority-gated) — and if that path turns out heavier than
  one helper call, STOP and surface options rather than inventing gym-only bridging (gym-vs-game
  tooling rule).
- Direct-attach acceptance is ALREADY pinned by automation (`Poi_Add_CreatesValidHandle` composes
  onto an existing entity); the gym adds the human-visible demonstration on the REAL player.
  Verification is `[EDITOR-VERIFY]` (gyms are manual by design): exact steps recorded at exit.

### Cleanup scope

- Campaign docs (PROMPT/PLAN/PROGRESS/Gate_0*.md): **NOT deleted this gate.** PLAN.md's post-ship
  cleanup says delete once Gate 5 + full gate are green, but the campaign is UNPUSHED — these docs
  are the review/evidence trail until the user ships. Deletion is recorded as part of the ship
  change (ck-ship-dev), not before. `REFACTOR_MultiProjectorPoi.md` is KEPT regardless (its
  Follow-ups section is live: cadence retrofit list + bucketed-cadence design).
- `CONTINUATION_PROMPT_Gate2PoiDisplayDefinition.md` (untracked, user-created, stale): flagged for
  the USER to delete — untracked deletion is irreversible and it is not this session's file.
- Code cleanup: none outstanding — Gates 3-4 left no orphans (`Get_RangeFadeAlpha` still used by
  the inline fade path; residue greps zero at both gate exits).

### Full gate

- `--test --no-nullrhi` with NO pattern (full suite) — `--no-nullrhi` per the standing rule (Iskm
  BatchedBake + shader tests fail under nullrhi and their ensure storm poisons later tests).
- Diff against `Regate2-PostRebase-Full.log`; every delta must map to a recorded campaign change
  (Poi suite reshape 7→9 incl. 1 rename, +4 PDD, +3 VisibleRange from Gate 1, +2 Gate 4
  integration tests, Minimap red fixed). Unexplained deltas = STOP and investigate.

## Work items

1. **[AS / Opus agent]** Player-as-Poi in CkMinimapGym per Locked design (+ the branch).
2. **[Fable]** Audit the gym edit.
3. **[Fable]** FULL suite run (`--test --no-nullrhi --discover-fresh`); this boot also proves the
   gym AS compiles. Diff vs the reference; explain every delta.
4. **[Fable]** Campaign close-out: walk PROMPT success criteria 1-6 with evidence in PROGRESS.md;
   update PLAN row + this Status; final commits.

## Expected observations — branches

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Full suite | Green modulo pre-existing non-Poi flakes (crowd PathRefresh is load-flaky; net AS stubs count per environment) | New reds in campaign-touched suites | STOP — regression; root-cause before closing |
| Full suite AS compile | Clean (gym edit compiles) | AS errors in gym | Fix forward; gym edits are small |
| Full-suite totals vs reference | Explainable via the recorded reshape map | Unexplained missing tests | Check discovery cache (`--discover-fresh`) before suspecting deletion |

## Exit criteria — ALL land in the SAME commit set

- [x] Gym demonstrates Player-as-Poi via direct-attach (`CkMinimapGym.as` `DoComposePlayerAsPoi` —
      composed onto `_PawnEntity`, the SAME entity hosting the minimap observer; idempotency-gated).
      `[EDITOR-VERIFY]` steps: PIE the TestGyms level → cycle to the Minimap gym → the HUD minimap
      (top-right `UCk_MinimapFrame_Widget`) shows a player blip pinned at FRAME CENTER (distance 0,
      priority 100) that stays centered while walking/turning; `Ck_GymMinimap_Readout` lists an
      entry `pos (0, 0) | dist 0 | prio 100`. NOTE: first gym run appends
      `Poi.Category.Player` to the host `Config/DefaultGameplayTags.ini` (expected one-line churn —
      commit it when it appears; tags resolve at gym RUNTIME, so the full-gate boot did not append it).
- [x] Full suite run complete (`Exit_Gate5_FullGate.log`, --no-nullrhi): 855 total / 844 passed /
      11 failed. Name-set diff vs `Regate2-PostRebase-Full.log` (776/773/3): ZERO tests removed,
      +79 added — 37 campaign-family (ALL GREEN) + 42 from sibling 07-14→07-21 work. Zero
      unexplained count deltas.
- [x] **The 11 reds are all Crowd/PathNetwork — NONE campaign-attributable (evidence-backed
      exclusion, not a wave-off):** 10/18 crowd tests fail DETERMINISTICALLY in isolation too
      (`Exit_Gate5_CrowdIsolation.log`) ⇒ not load flakes ⇒ a REAL pre-existing regression. But:
      the campaign diff over CkCrowd/CkAStar/CkNavigation/CkPathNetwork is EMPTY
      (`git diff 7e8347b75..HEAD` on those paths = nothing); every crowd-adjacent commit in the
      regression window (incl. the `a24c5a2d2` CkCrowd composition reshape + CkTests `a14ad47`
      test-interface update) is an ANCESTOR of campaign entry; 5 of the 10 were green in the 07-14
      reference (regressed in the pre-campaign window), 5 were authored after it. Surfaced to the
      maintainer as a foreign-workstream follow-up — root-causing it belongs to the crowd
      workstream, not this campaign.
- [x] PROMPT success criteria 1-6 walked with evidence in PROGRESS (campaign COMPLETE entry;
      criterion 6 met FOR CAMPAIGN SCOPE with the crowd exclusion recorded honestly).
- [x] PLAN.md row + this Status updated; ship-time cleanup instruction recorded in PROGRESS.
