# Gate 4 — CkCompass + CkMinimap semantic rewire

> **Status:** ✅ Done (2026-07-22 — actual: 1 session, as estimated)
> **Depends on:** Gate 3 ✅ (2026-07-22)
> **Estimate:** 1 session

## Goal

After this gate: the projectors consume `CkVisibleRange` STATE, not just config — an explicit
`Request_SetVisibility(Hide)` on a Poi removes it from every projector; a `VisibleRange` composed on
a consumer's `PoiDisplayDefinition` child culls that consumer's entry independently of the others;
the projectors feed `Update_Distance` so that state is real. The known Minimap delegate-signature
red is fixed (suite goes 15/15).

## Entry criteria (verified 2026-07-22)

- [x] HEADs: CkFoundation `ab8e82389`, CkTests `d3b096f`, CkGameplayDebugger `dbce397`, root `6290d45`.
- [x] Baseline = Gate 3 exit (`Exit_Gate3_*.log`): Poi 44/44, Compass 13/13, Minimap 14/15
      (`Minimap_Add_CreatesChild` red — root cause CONFIRMED this gate: `FCk_Lambda_InHandle` is a
      TwoParams dynamic delegate (`CkEcs/Delegates/CkDelegates.h:21-24` — FCk_Handle +
      FInstancedStruct payload) and the test handler `OnEachMinimap(FCk_Handle)` takes one param),
      VisibleRange 4/4.

## Locked design

### Decision: hybrid culling — inline config math STAYS; VR state ADDS capabilities

The naive "replace projector range math with `FTag_VisibleRange_Hidden` exclusion" has an inherent
**membership blip**: a Poi composed out-of-range is visible for ≥1 frame (distance not yet fed /
evaluated), firing spurious Appeared/Disappeared membership signals and breaking existing
never-appears test assertions. So:

- The projectors KEEP the Gate 3 inline cull/fade (read VR config, same-frame decision). This is
  the fast path, not a shim — it guarantees blip-free membership.
- NEW: base-entity `ck::FTag_VisibleRange_Hidden` skips the entry — as a **worker skip placed after
  the distance record**, NOT a view exclude (CORRECTED mid-gate, caught in the Fable audit: a
  view-excluded hidden Poi would stop receiving the distance feed and could never re-evaluate back
  to visible when the observer returns into range — a permanent hidden lock; the worker skip keeps
  feeding while not projecting). This is what makes **explicit hide** (`Request_SetVisibility(Hide)`)
  and the fed range state work projector-wide, with recovery.
- NEW: the single-threaded GATHER pass (not the ParallelFor worker — its contract is pure reads)
  feeds observer distance: `Update_Distance` into the base entity's VisibleRange (if composed) and
  into the resolved consumer child's VisibleRange (if composed). Plain setter; the VR processor
  evaluates on its own cadence.
- NEW: the worker, after resolving the consumer's DisplayDefinition, skips the entry if the DD
  handle carries `ck::FTag_VisibleRange_Hidden` or `ck::FTag_PoiDisplayDefinition_ParentHidden`
  (pure `Has` reads) — this is the **per-consumer restriction**: a VR on the compass's DD child
  culls the compass entry only. One-frame latency on this path is accepted (new capability, no
  existing assertion depends on its timing). Direct-attach DDs alias the base entity — both checks
  are harmless there (base Hidden is already view-excluded; ParentHidden is only ever set on record
  children).
- Multi-projector shared-VR caveat (document, don't solve): two projectors with DIFFERENT observers
  both feeding one base VR would fight over `_Distance`. One local viewer (this product) makes the
  writes agree; per-consumer differences belong on the DD children — that's what they're for.

### Minimap delegate fix (test-side only)

`OnEachMinimap` gains the second param with the AS const-ref struct shape
(`const FInstancedStruct&in` — the house trap; verify against a GREEN `FCk_Lambda_InHandle` usage
elsewhere in the corpus before assuming the exact spelling). If the `[Server] ! Has(InHandle)`
ensure from the original red persists after the signature fix, STOP and root-cause — do not paper
over it.

### MapDebugger Priority column (Gate 3 deferred decision)

DROPPED. Priority is per-consumer now; a single number per Poi is meaningless. Remove the column
from the row struct + widget; the PDD inspector is where per-consumer priority lives.

## Work items

1. **[C++ / Opus agent]** Both projectors: gather-exclude base Hidden tag; gather-pass distance
   feed (base + consumer child); worker per-consumer hidden checks. Files:
   `CkCompass_Processor.cpp` (~L278-354), `CkMinimap_Processor.cpp` (~L313-387). No Build.cs
   changes expected (VisibleRange/PDD deps landed in Gate 3).
2. **[C++ / same agent]** CkMapDebugger: drop the Priority column.
3. **[Fable]** Audit; toolbox build.
4. **[AS / Opus agent, after build green]** Fix `CkAutoTest_Minimap_Add_CreatesChild` handler
   signature. New integration tests in `Script/CkPoi/`:
   - `CkAutoTest_Poi_ExplicitHide_RemovesFromBothProjectors` — base VR (unlimited range),
     compass + minimap DDs; `Request_SetVisibility(Hide)` → both entries gone; `Show` → both back.
   - `CkAutoTest_Poi_PerConsumerRange_CullsOneProjector` — compass DD child composes VR with a
     range smaller than the test distance, minimap DD child has none → compass entry absent,
     minimap entry present (allow ≥2 projector updates for the child-VR latency path).
5. **[Fable]** Full sweep; diff vs baseline. Expected: Poi 46/44 (+2 new), Compass 13/13,
   **Minimap 15/15** (red fixed), VisibleRange 4/4.
6. **[Fable]** Docs same-commit: Compass/Minimap CLAUDE.md delivery contracts (state-driven
   exclusion, distance feed, per-consumer restriction recipe, shared-VR caveat);
   VR/PDD CLAUDE.md "Used by" updates; PROGRESS/PLAN/this Status header.

## Expected observations — branches

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Build after 1-2 | Green (deps already present) | LNK on VR/PDD symbols in MapDebugger | Add the direct dep, note it |
| Minimap suite | 15/15 | The `[Server] ! Has` ensure persists | STOP; root-cause the ForEach path (do not mask) |
| ExplicitHide test | Both entries gone within 2 projector updates | Entries persist | Check request drain group vs PostTransform ordering; adjust waits before suspecting code |
| PerConsumerRange test | Compass culled, minimap present | Both culled | Child-VR feed hitting the BASE VR by mistake (handle mixup in the gather feed) |
| Existing Compass/Minimap suites | Unchanged counts, green | AppearDisappear flaps | The gather exclude introduced a blip — the base Hidden tag flipped mid-test by the new distance feed; check MinRange/distance-0 interaction |

## Exit criteria — ALL land in the SAME commit set

- [x] Full sweep green (2026-07-22): Poi 46/46 (+2 new: ExplicitHide_RemovesFromBothProjectors,
      PerConsumerRange_CullsOneProjector), Compass 13/13 (=), **Minimap 15/15** (the campaign-long red
      is FIXED — see below), VisibleRange 4/4 (=). Logs `Exit_Gate4_{Poi,Compass,Minimap_3,VisibleRange}.log`.
- [x] Stock-ensure grep clean on all changed files; no old-machinery residue introduced.
- [x] `[EDITOR-VERIFY]` (human): confirmed NONE new — no reflected surface changed this gate (Gates
      2-3 BP checklists remain the outstanding human items).
- [x] Docs per item 6; PROGRESS dated entry with confirmed/inferred split.

**Post-gate corrections to this plan (recorded, not relitigated):**
- The Locked design's original view-exclude for base `FTag_VisibleRange_Hidden` was corrected
  mid-gate to a worker skip (see the amended Locked design above) — the exclude would have
  permanently locked range-hidden Pois out of the distance feed. Caught in the Fable audit before
  any test ran; the ExplicitHide test's Show-recovery phase now pins the fixed behavior.
- `Minimap_Add_CreatesChild` turned out to carry FOUR stacked defects, not one: (1) one-param
  handler vs the TwoParams `FCk_Lambda_InHandle` (the known delegate bug); (2) two direct-attach
  `Add` calls, stale since the pre-campaign Add/Create split — the `[Server] ! Has` ensure was
  `Add`'s one-per-entity guard, INDEPENDENT of the delegate (found by the AS agent, not masked);
  (3) same-tick enumeration of the DEFERRED RecordOfMinimaps connect; (4) asserting
  `ForEach_Minimap`'s returned array AND delegate count on one call — the API contract is
  EITHER/OR (`CkMinimap_Utils.cpp:99-105`: bound delegate ⇒ array deliberately empty). Fixed as:
  handler signature, Add→Create, one-frame settle, two calls (one per mode). Class name kept
  (renames wedge the generated files — Gate 3 lesson).
