# CkIntent — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->

**As of 2026-08-09 session end: ALL PHASES 0-8 CLOSED; CAMPAIGN-END FULL SUITE ✅ GREEN +
DELTA-ZERO BY NAME** (`1065/1062/3`, +38 rows all ours all green, zero lost, third red =
proven load flake [INV-B] — the only failures anywhere are the two eternal
`PathNetworkFollower` names). **Technical scope COMPLETE.** This session: Phase 7 closed
(7-1 verified committed CkFoundation `4c7ab4d0e`/`a3f4c8cd1`/`5a2a15595` + CkTests
`ab861b3f`; 7-2 = `CkIntentDebugger`, 25 files ~4380 lines in **CkGameplayDebugger** — a
NEW repo in the ship set — under [P7-D4]/[P7-D5], gated build + 120/120 + census 2/2);
Phase 8 closed (8-1 forty-move bake = criterion 6; 8-2 Fighting / 8-3 Souls / 8-4 Debugger
gyms; 8-5 coverage gap tests from the [P8-D6] audit; final scoped gate 123/123).
**COMMITTED 2026-08-09 (user-authorized "/commit while I test"; push still NEVER
authorized):** CkTests on `dev`: `f67fbf5c` (40-move bake) → `c350a184` (coverage gap
tests) → `03b977ca` (three gyms + registry) → `9fa88957` (wrapper regen + 4 populator
actors). CkFoundation on `dev`: `121614c89` (campaign docs) → `fc4dbf660` +1 follow-up
(module-doc line). **CkGameplayDebugger: `0f51a76` landed on
`feature/debugger-qol-campaign` — NOT dev** (the submodule was sitting on the insights/QoL
campaign's branch; discovered at commit time, deliberately not moved under the user's open
editor). **SHIP FLAG:** the ship conversation must decide cherry-pick-to-dev vs shipping
that branch, BEFORE any pointer bump (publish guard: only pushed SHAs). Ship flow
(fetch/divergence/backup/rebase CkTests/regate/push/pointer bumps) remains a separate
conversation. Foreign dirt untouched — never stage. **Human queue:** the
`[EDITOR-VERIFY]` items (Phase-7 debugger views incl. criterion-5 scrub; three gym
drive-throughs incl. criterion-1's counter; earlier queue items — KeyBinding gym, 1a-4/1a-5,
criterion-4 settings-page leg), 0A hardware spike, maintainer review flags ([P2-D2]/[P4-D1]/
[P4-D4]/[P3-D4] reconstructions, [P7-D4] fragment-read precedent, [P8-D6] residual C++
coverage + 2 dead-API flags + greenfield-prefix full-suite discovery gap, CkGameSettings
defects 1-5,7), the load-flake note to the crowd workstream. Today's rulings: [P7-D4]
[P7-D5] [P8-D1..D6]. Every gate log named in the dated entry below.

**Prior state (2026-08-08, superseded):**
Commits (all on `dev`): CkFoundation `9b49261bd` (keybinding subsystem fix, own commit per the
suggested split) → `c1cc2bce8` (Phase 1 raw layer) → `f888fbdf1` (Phase 1b bias) → `e646ad827`
(CkInput Claude.md) → campaign-docs commit (this file's snapshot). CkTests `9d5a3278` (gym + 21
autotests + 21 populator-placed external actors + wrapper regen). Superproject `56e4f36`
(DefaultInput.ini [P1A-D4]). Submodule pointer bumps deferred to push time (cross-repo publish
guard — bump only pushed SHAs). Left untouched as foreign: CkFoundation's 82 CkUsf GeneratedLooks
uassets + request-completion-delegates continuation + docs/reviews + docs/superpowers;
superproject's DefaultGameplayTags.ini + crowd-debugger continuation + _scratch/.

**Earlier session-end state:**
**THREE PHASES CODE-COMPLETE AND GATE-GREEN THIS SESSION — 1a, 1, 1b.** Final full suite:
`Total 1026 / Passed 1024 / Failed 2 / Skipped 0 / Contaminated 0` — delta-zero vs the
pre-campaign baseline (1005/1003/2): +21 new tests all passing (7 keybinding, 1 InputSource,
6 InputLayer, 7 InputBias), and the ONLY failures are the same two pre-existing
`PathNetworkFollower` names, deterministic across **5/5** full runs.
Landed: `Gym_Input_KeyBinding` (5 stations) + keybinding AutoTests; ONE production defect fix
(the DOA change-signal hook, own commit); the whole CkInput raw layer (`InputSource` inbox +
explicit device ownership, `InputLayer` priority stack / declarative captures / router with
press→release ownership / global actions, `InputSlate` observe-only writer incl. mouse-move +
the `ck.Input.DumpRawEvents` 0A instrument S1-S7); the `InputBias` conditioning stage
(`Collect → Bias → Route`); `CkInput/Claude.md` fully rewritten+extended; O11/O13/O9/O16 closed;
`DESIGN_PollSurface.md` PROPOSED (Phase 6 ruling); host `DefaultInput.ini` +1 line ([P1A-D4]).
**Stopping point is principled:** Phase 2 is 0F-gated (human sit-down); Phase 3 opens the new
`CkIntent` module — a structural boundary held for maintainer review of Phases 1/1b; 0A's
instrument is built and awaits the human's ~10-minute key-pressing session.
**Human queue:** `[EDITOR-VERIFY]` gym drive + 1a-4 persistence + 1a-5 hot-swap; 0A spike S1-S7;
0F sit-down (+ defect escalation to CkGameSettings owner); review of flagged rulings
([P1-D4] routing defaults, [P1B] TryGet sentinel, global-action completion-owner asymmetry);
commit authorization.
**Review separability (phases were chained under the AFK "as many phases as you can" directive —
each is independently reviewable/rejectable):** Phase 1a = `Script/CkInput/` gym+test files, the
`CkKeyBinding_Subsystem` fix (own commit), `DefaultInput.ini` line, `Claude.md` rewrite. Phase 1 =
NEW `CkInputSource_*`/`CkInputLayer_*`/`CkInputSlate_*`/`CkInput_ProcessorGroups.h` files +
`CkAutoTest_Input{Source,Layer}_*` tests; only existing-file touches: one friend line in
`CkInputSource_Fragment.h` (self-contained to Phase 1) and `Claude.md` sections. Phase 1b = NEW
`CkInputBias_*` files + 7 tests + the `FGroup_Input_Bias` block + one `Claude.md` subsection.
Rejecting a later phase = deleting its new files + its `Claude.md` sections; no earlier phase
depends on a later one. Earlier same-day state below.

**Prior same-day state (Phase 1a):**
Phase 1a **CODE-COMPLETE, GATE GREEN** — full suite `1012 / 1010 / 2 / 0 / 0` (10m14s), delta-zero
vs baseline: +7 new Input tests all passing, the only failures the SAME two pre-existing
`PathNetworkFollower` names (deterministic 3/3 runs). Remaining before the phase closes:
the human `[EDITOR-VERIFY]` steps (gym drive-through, 1a-4 PIE-restart persistence, 1a-5
controller hot-swap — exact steps in the 2026-08-08 wave-2 entry / stations-agent report) and
the commit decision. **One production-code fix shipped this phase** (dead change-signal hook,
own commit at ship time) and one host config line ([P1A-D4]). O11/O13/O9/O16 closed.
Next phase: 1 (CkInput raw layer) — blocked on nothing technical; 0A hardware spike remains the
human-gated input for Phases 1-3 design confirmation.
Phase 0 research done except human-blocked 0A/0F.
0B/0D/0G/0H/0I answered in `PHASE_0_RESEARCH.md`; two new blocking findings (N6, N7).
**Baseline being diffed against — ✅ CAPTURED 2026-08-08, ✅ RE-CONFIRMED same day (2nd run):**
`Total 1005 / Passed 1003 / Failed 2 / Skipped 0 / Contaminated 0` — 12m14s, build succeeded.
Command: `--build --target=Editor --test --no-nullrhi --parallel 1` (full suite, no `--test-pattern`).
Second run (test-only, unchanged tree, 10m16s) reproduced the identical summary AND the identical
two failing names — the failures are deterministic on this tree (2/2 runs); the single-run caveat
is lifted.
**The 2 pre-existing failures — any Phase must diff against these two names, not against zero:**
- `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`
- `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`

Tree state at capture: superproject `e9aed7d` (dirty: `Config/DefaultGameplayTags.ini`; CkTests
pointer moved), CkFoundation `b982baf24`, CkTests `fd26b553` on branch
`backup/pre-branch-audit-265-gfd26b553` — i.e. **another workstream's state is in this tree.** Both
failures are in that workstream's area (path-network / navmesh), and this campaign has written zero
code, so they are **not** ours. Re-capture the baseline if that tree state changes before Phase 1.
**Next action:** ~~rewrite `PHASE_1A.md`~~ **done before this session** — the file now carries the
corrected API names and the 1a-0 content prerequisite; verified by re-read 2026-08-08. Phase 1a is
running. Tree delta since baseline: CkTests is now on branch `dev` at the SAME SHA `fd26b553`
(was `backup/pre-branch-audit-265-gfd26b553`) — baseline ruled still valid, see [P1A-D1]. **Ignore the "open decisions" list at the foot of
`PHASE_0_RESEARCH.md` — four of its five are settled** (N6→D14/dead, O9→D18, N5→D19, 0D→D22 + the
subsystem shape). That list is stale; PROMPT.md's decision table is authoritative.
**Blocked on:** 0A (hardware spike) and 0F (CkGameSettings sit-down) both need a human — neither
blocks Phase 1a. The **poll-surface hole** (DESIGN_InputLayering.md) blocks Phase 6, not now.

---

## Decision log

| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-07 | Module named `CkIntent`, not `CkInput` — **CONFIRMED by maintainer** | `Source/CkInput/` already exists and ships an EI-backed rebinding stack | Closed |
| 2026-08-07 | EI + `UEnhancedInputUserSettings` stays the binding/rebinding store | Ships, AS-bound, BusterBlock consumes it, CkGameSettings decision #3 locked it 2026-08-05 | Never, unless CkGameSettings reverses #3 |
| 2026-08-07 | Intents are client-local; no replication | No client→server input transport exists; 4p co-op PvE does not need one | A PvP or competitive mode is chartered |
| 2026-08-07 | Intent phase = fragment + signals, not `CkByteAttribute` | Attribute coalescing makes transient phases unobservable | Never — mechanism is structural |
| 2026-08-07 | Poll/consume is the primary gameplay surface; signals are presentation | Late binders receive only the LAST payload; replay cannot reconstruct a sequence | Never |
| 2026-08-07 | Deferral from forward ambiguity only (holds, chords) | Sequence suffixes are already in the buffer; the backward scan resolves them on the press frame | Never — this is the latency guarantee |
| 2026-08-07 | Fighting-game surface is in scope | CkFoundation is a framework; breadth is the requirement | Never — maintainer decided |
| 2026-08-07 | Compact notation is the authoring surface, not just the test fixture | AS cannot brace-init `TArray` defaults; nested structs would need per-move imperative code | If the notation grows conditionals — forbid that instead |
| 2026-08-07 | Extend `SCkDebug_EventTimeline` rather than build a timeline | Shared parameterized widget in `CkDebuggerCommon`, already consumed by `CkGoapDebugger` | If its lane model cannot express per-intent phase lanes — record the gap, then decide |
| 2026-08-08 | **D1 REVISED — two modules.** `CkInput` (raw + bias + device ownership + existing EI binding/glyphs) → `CkIntent` (sampling, matching, intents) | Maintainer directive: CkInput is "our own raw input layer" that biases before CkIntent | **Reverses this log's first row** — CkInput is no longer merely "the name that was taken"; Phases 1a–2 write substantial CkInput code |
| 2026-08-08 | D14/D16/D24 — layered input delivery, layer stack in `CkInput` | ftxc-proven; blocking becomes structural, not a flag | Never — replaces suspension |
| 2026-08-08 | **D15 REVISED** — physical hold is a fact; per-intent transition policy decides eligibility | Original "matching unconditional" detonated stale charges on layer pop; both cancel-on-enter and carry-into-vehicle are valid game designs | Never — the framework must not legislate either |
| 2026-08-08 | **D17 DEAD** — superseded by D25b | No return value exists, so ftxc's return-false footgun cannot occur | Only if a callback is reintroduced, which D25b forbids |
| 2026-08-08 | D18 — axes in scope for acquisition/record, out of the intent grammar | Excluding them entirely would force a second consumer of the same Slate stream | Closed |
| 2026-08-08 | D19 — `MaxReplayedTicks` clamp on shared `TProcessorBase`, default unlimited | Unbounded replay loop; 1 s hitch = 60 `DoTick`s | Needs its own review note (shared base) |
| 2026-08-08 | **D21 REVISED** — capability is per-device-class provenance, not one field | Keyboard and gamepad differ on the same local player under D22; one flag would lie | Closed |
| 2026-08-08 | D20/D22/D23 — biasing stage, explicit device ownership, keybinding gym first | Maintainer | D20's params must be a fragment, not settings-only (runtime-mutable) |
| 2026-08-08 | **D25 / D25a / D25b** — ECS-by-default; source interface deleted; captures declarative | Maintainer: UObject/UInterface only where ECS would be a contortion. 7 UINTERFACEs across 101 modules | Never |
| 2026-08-08 | **[P1A-D1]** Baseline `1005/1003/2` stays valid across CkTests' branch-label change (`backup/pre-branch-audit-265-gfd26b553` → `dev`, same SHA `fd26b553`) | Identical tree content at the same commit; only the ref name moved | If the CkTests SHA itself moves |
| 2026-08-08 | **[P1A-D2]** Flakiness re-run folded into orchestrator-serialized toolbox plan: full test-only run NOW on the unchanged tree, BEFORE any Script/ edit lands; all toolbox runs are orchestrator-only; 1a-0 drafts go to scratchpad and install only after this run completes | Script/ edits poison in-flight AS test runs; subagent gate lanes die with their session | — |
| 2026-08-08 | **[P1A-D4]** PHASE_1A's "no host-project config change" clause is AMENDED: `bEnableUserSettings=True` added to host `Config/DefaultInput.ini` (`[/Script/EnhancedInput.EnhancedInputDeveloperSettings]`). Scan paths still NOT configured; registration stays runtime-per-test; zero production code unchanged | First gate run failed with "user settings unavailable": engine defaults the flag OFF (`EnhancedInputDeveloperSettings.cpp:17`) and gates settings creation on it (`EnhancedInputSubsystems.cpp:47-49,109-111`); without it the ENTIRE shipped rebinding surface is un-instantiable in this host. The scope clause was written about scan paths and did not foresee this. One reversible ini line; uncommitted; flag to maintainer at commit time | If the maintainer prefers a different host wiring |
| 2026-08-08 | **[P1-D1]** Phase 1 Slate preprocessor: index 0, **observe-only** (never eats) — all consumption at ECS routing per D14/D25b; coexists with the two existing index-0 processors, whose mutual collision stays flagged to the debugger owner | Recording fidelity wants earliest visibility; an observe-only processor cannot join the collision | If a processor ahead of us provably eats events before we see them |
| 2026-08-08 | **[P1-D2]** O2 seam = minimal `ULocalPlayerSubsystem` owning the per-local-player input-source entity, created from `PlayerControllerChanged` (+ lazy backstop) — the 1a timing lesson applied | Engine-lifecycle boundary is exactly where D25 permits subsystems; no general LocalPlayer↔entity mechanism exists (0D) and Phase 1 must not invent one | If the maintainer wants a general mechanism instead |
| 2026-08-08 | **[P1-D3]** Phase 1 proceeds WITHOUT the 0A spike; the Phase-1 thin writer + event dump becomes the 0A instrument (supersedes the throwaway probe, pending maintainer agreement) | PHASE_0.md's expected-observations table pre-writes a graceful response to every 0A outcome — the answers refine docs/tuning (Phases 3-4), not Phase 1 structures; maintainer's live directive authorized phase chaining this session | If 0A's results contradict a D21/D22 assumption after all |
| 2026-08-08 | **[P1-D4]** Default device routing for the thin writer: keyboard+mouse → local player 0's source; gamepad → local player at its raw UserIndex; **explicit D22 assignment always overrides**. | With zero assignments nothing would flow (unusable); this is the pragmatic UE convention while keeping D22's explicit ownership as the authority; PHASE_0 :128 pre-wrote the composite-routing proposal | Maintainer review — split-screen keyboard assignment is exactly the case D22 exists for |
| 2026-08-08 | **[P1A-D7]** Gym redesign (maintainer directive, REVISES [P1A-D6]): stations become SM-driven (`CkGym_StationSm`) self-running, **self-asserting** demos — each step renders WHAT/EXPECT/GOT + PASS/FAIL; exec commands demoted to manual layer; last-action report renders on the advertising panel. Design bar for ALL future gyms: a viewer with zero feature knowledge must read working/broken off the panels. Visual layer: per-line colored text via a new additive colored-lines API on the shared station display (`UTextRenderComponent` = one color per component — no rich text, no glyph brushes without .uasset widgets, so key icons = colored key-name text). Palette: white base / green pass / red fail / amber changed / cyan active / grey idle | Maintainer drove the gym at [EDITOR-VERIFY] and could not tell what was working without orchestrator narration — exec-driven stations make the viewer the assertion engine | If the colored-lines layout regresses other gyms' displays (shared-file change) |
| 2026-08-08 | **[P1A-D3]** Layer ordering = **option (b), explicit `_Priority` int with registration-time collision detection** — option (a) ordered-child-record is DEAD. This executes DESIGN_InputLayering.md's own prewritten fallback ("(b) only if CkRecord does not preserve order"), triggered by O11's verdict | O11: `ORDERED-TODAY-BUT-INCIDENTAL` — `DoForEach_Entry` swap-prunes dead entries (`CkRecord_Utils.h:825` `RemoveAtSwap`, orchestrator re-verified), re-insertion is tail-only, zero contractual language; CkCamera's shipped layer stack already uses explicit priority on a population destroyed mid-life (`CkCamera_Processor.cpp:210-216`) | Only if CkRecord ever gains an ordering contract |
| 2026-08-08 | **[P1A-D9]** Defect-#13 production fix AUTHORIZED by maintainer: `UnbindConflictAndRemap`'s unbind loop truly unbinds the conflict holder via `MapPlayerKey` + `EKeys::Invalid` (was `UnMapPlayerKey` = revert-to-default, a no-op for a default-bound holder → silent duplicate binding). Pinned by a new AutoTest; gym step-6 amber CONFIRMED-DEFECT annotation retired | Maintainer directive 2026-08-08 ("Fix the unbindconflictandremap"). Mechanism proven in-module: `SwapKeys` already assigns `EKeys::Invalid` through `MapPlayerKey` (anti-pattern #6 in `CkInput/Claude.md`) | If a downstream consumer (CkGameSettings page, BusterBlock) turns out to want revert-to-default semantics — that becomes a separate, honestly-named API, not a revert of this fix |
| 2026-08-08 | **[P2-D1]** 0F settled by the maintainer's resume directive: the ButtonId map is a read-only consumer of the store the CkGameSettings page writes through; `OnSettingsChanged` is the shared broadcast both already use; no conflict. Phase 2 UNBLOCKED | Maintainer owns both campaigns and directed "move on to the next phase(s)" with the 0F gate on record. Defect escalation (1-5, 7; #13 fixed) stays on the human queue | If the CkGameSettings page grows a WRITE path keyed on anything the map derives from besides mapping names |
| 2026-08-08 | **[P2-D2]** Button model reconstructed (original design PROMPT superseded + lost): ButtonId = stable `(Tier, FName)` identity, NOT a dense int (dense packing = Phase 4 bake). Tier 1 Mapped = one id per EI mapping name, associations re-derived on `OnSettingsChanged`; Tier 2 Physical = one id per raw FKey, fixed (the D16/test tier). Identities never change for the map's lifetime; FKey→ButtonId is one-to-many by design | Mapping names are what the settings store keys on — rebind-stable by construction; duplicate/shared keys are legal EI states (defect #13's residue proved they occur) | Maintainer review (reconstruction flag); or if Phase 3's frame record needs dense global indices before Phase 4 exists |
| 2026-08-08 | **[P2-D3]** `InputButtonMap` is a feature composed on the input-SOURCE entity (mimic `InputBias`: `Add` only, no `Create`), opt-in in v1 | A map on a child entity has no player identity; Phase 3 composes it alongside the sampler | If every source ends up needing one unconditionally — then auto-compose from the source subsystem |
| 2026-08-08 | **[P2-D4] GATE POLICY, campaign-wide (maintainer):** phase-close FULL-suite runs are DEFERRED — phases close on their SCOPED gate; ONE full suite runs when the complete campaign is green (and the ship flow's regate law still applies at push time). The Phase-2 full-suite run was killed mid-flight on this directive (no orphan editor, log lock verified free) | Maintainer 2026-08-08: "let's speed this up. We can do a full suite when the complete campaign is Green." Delta-zero accountability moves to campaign end: the final full suite diffs vs `1027/1025/2` PLUS every test name added after it | If a phase touches shared/base code with suite-wide blast (e.g. the D19 clamp on `TProcessorBase`) the orchestrator may still flag a targeted broader pattern, but not a full suite |
| 2026-08-09 | **[P7-D4] Debugger modules MAY read CkIntent/CkInput fragments directly (read-only)** — the 7-2 dispatch package's "public Utils APIs only" clause is RELAXED to match in-plugin precedent; [P7-D1]'s substance (render RECORDED facts, never recompute matching/octants/verdicts) stays iron. NO new CkFoundation read APIs for debugger-only needs (option 2 rejected: `Get_PendingEpisodes`/`Get_ActiveSet` as public Utils would expose matcher internals gameplay code must never touch — worse encapsulation than a privileged editor-only observer). Gaps G1-G4 resolve via `View<>`/`Has<>`/`CK_PROPERTY_GET` accessors: G1 layer-stack enumeration via `FFragment_InputLayer_Params` view, G2 `_ActiveSet`, G3 `_PhaseFrame` (IS the current-phase span start), G4 pending episodes + hold accumulators. Write-protection intact (phase-row friend narrowing still forbids writes at compile time) | Shipped precedent: `CkAggroDebugger_DataCollector.cpp:52-78` (fragment view + `Get_LastSwitchTime`), `CkGoapDebugger_Module.cpp:50,69,80` (`Has<FFragment_*>`). Debuggers are UncookedOnly privileged observers; the Utils surface is the GAMEPLAY API, not the diagnostic one | If a debugger read is ever observed to mutate (e.g. a non-const view) or a recompute sneaks in under the read |
| 2026-08-09 | **[P7-D5] CkIntentDebugger registers an `FCkDebug_EntityTargetRoute`** (resolves exact/ancestor/descendant to the owning source + layer row via the common closest-lineage helper; never a tab-open-only route). The 7-2 unit's deferral was procedurally correct but the mimicked exemplar (CkGoapDebugger) registers one and the layer-stack panel selects entities — in scope per [P7-D3]'s mimic ruling, not an invention | Plugin CLAUDE.md "Entity-aware debugger entry" contract; grep: Goap/Sm/AStar/Crowd all register routes | If the closest-lineage helper cannot express source-or-layer resolution |
| 2026-08-09 | **[P8-D5] Gym motion legs are stick-only; keyboard fallbacks exist for BUTTON legs only** — a keyboard-driven octant is unrepresentable (octant derives solely from the conditioned axis pair, `CkIntentSampler_Processor.cpp:277-323`; SOCD cleaned slots are never compared against direction steps) and the mouse-axis alternative is REJECTED (unnormalized pixel deltas vs the 0.25 neutral radius; last conditioned value persists at rest → sticky octant; one sampler per source would break the gamepad path for all stations). Panels state the constraint in plain text; no SOCD quad is composed (a quad that moves a readout while the octant refuses would read as broken to the zero-knowledge viewer) | The 8-2 unit's verbatim STOP analysis, verified against the sampler processor + module doc ("a player on a stick moves _Octant and leaves both cleaned slots at Neutral") | If a future device class supplies digital octants natively |
| 2026-08-09 | **[P8-D6] Coverage-audit ruling over the criterion-7 matrix (89 UFUNCTIONs / 58 automated / 37 zero-automated):** (a) macro-generated `DoCast`/`DoCastChecked`/`Get_InvalidHandle`/`Has` boilerplate = NOT gaps (uniform `CK_DEFINE_*` codegen, one working instance proves the generator; several gym/debugger-exercised); (b) tag-keyed matcher quartet zero-covered BECAUSE every set bakes an empty tag (open `Intent.*` namespace, already a maintainer flag) — but the CURRENT contract (empty-tag reads answer Idle/INDEX_NONE/false/invalid) gets a pinning test; (c) REAL gaps dispatched (unit 8-5): the three `UnbindFrom_*` signal functions (unbind regressions are a recorded failure class — this campaign's first commit was an unbind fix) + the subsystem `Get_InputSource` lazy-creation/idempotence seam; (d) maintainer flags, no action: `InputSource::Create` (zero callers repo-wide) + `Get_RoutedEventsThisFrame` (AS-bound, never called) — dead-or-future API; (e) criterion-7 environment reading: AS reflection exercise validates the BP/AS binding layer for all 58; C++ automation full only on grammar, C++ consumption of read surfaces via the debugger; residual per-function C++ exercise flagged to maintainer review, not force-closed | The enumeration agent's matrix (full tables in its return, 2026-08-09); non-negotiable #4 read against repo-wide practice | Maintainer review; or if the Intent.* namespace lands (then the tag-keyed quartet needs real coverage) |
| 2026-08-09 | **[P7-D3] revisit clause CLOSED — `SCkDebug_EventTimeline` lane model suffices:** `FCkDebug_TimelineSpan{LaneIndex,Start,End,Color,Tooltip}` carries per-intent phase spans; `FCkDebug_TimelineEvent{Shape,Tooltip,SelectionId,SideLabel}` carries blocked-by markers with scrub ids (`SCkDebug_EventTimeline.h:28-50`). No new timeline widget | 7-2 unit verified against the header during enumeration | Closed |

---

## Dated entries (append-only, newest first)

### 2026-08-09 — session resume (fresh orchestrator): 7-1 disk state verified; 7-2 re-dispatch

- **Unit 7-1 on-disk verification ✅** (the continuation prompt's first task): CkFoundation
  `4c7ab4d0e` (matcher + scan diagnostics) → `a3f4c8cd1` (`Source/CkIntent/Claude.md`) →
  `5a2a15595` (handoff notes) all present on `dev`; CkTests `ab861b3f` (427-line
  `CkAutoTest_Intent_ScanDiagnosticsRecordOnlyWhenEnabled.as` + wrapper regen + 1 pipeline
  uasset), status clean. `Get_ScanDiagnostics` present in `CkIntentMatcher_Utils.h/.cpp` +
  processor. 7-1 was reviewed AND gated before commit per the 2026-08-08 entries — nothing
  landed unreviewed. CkGameplayDebugger: NO `CkIntentDebugger` dir, status clean — the killed
  7-2 unit wrote nothing, confirmed. Only foreign dirt remains anywhere (CkUsf GeneratedLooks
  etc. per the handoff list).
- **7-2 dispatch prep:** launcher census located — `CkDebuggerLauncherCatalog.spec.cpp` keeps
  a strict 15-entry tab-id set + per-tool descriptor asserts + UNIQUE category/order slot;
  authoring steps = `Source/CkDebuggerLauncher/CLAUDE.md` steps 1-5 (descriptor after tab
  spawner, token-matched unregister before spawner removal, census row, `Ck.DebuggerLauncher`
  filter). New-module checklist = `Source/CkDebuggerCommon/CLAUDE.md` "Creating a new debugger
  module". Gate plan for 7-2: build `--generate` (uplugin gains a module) + scoped
  `Ck_AutoTest_In` + `Ck.DebuggerLauncher` pattern (census spec must pass with the 16th row).
- **Unit 7-2 DISPATCHED** (fresh Opus agent per the re-dispatch package: skills-first, mimic
  CkGoapDebugger, [P7-D1] recorded facts only, census row + Interface category next-free
  order slot, EndPIE/OnEnginePreExit lifecycle contracts, no git/no builds, STOPs incl. the
  `SCkDebug_EventTimeline` lane-model revisit clause and any missing read API).
- **Phase 8 OPENED in parallel** (`PHASE_8.md` authored; rulings **[P8-D1]** unit split/order
  — bake test first, gyms sequential (shared registry file), Script/ drafts to scratchpad
  while runs are in flight; **[P8-D2]** 40-move set = asset-shaped AS container + one
  parse+bake AutoTest, criterion-6 "no per-move structs" enforced by review-grep;
  **[P8-D3]** gym scope caps for Fighting/Souls/Debugger, all [P1A-D7] self-asserting,
  criterion-1 counter in Fighting, criterion-5 scrub fodder in Debugger; **[P8-D4]**
  gap-closing = orchestrator coverage audit at phase close, only proven gaps dispatch).
  Phase-7/8 overlap justified: units touch disjoint repos/files; 8-4's drive-through wants
  7-2 landed but has no code dependency. **Unit 8-1 DISPATCHED in parallel with 7-2**
  (Opus; drafts to scratchpad `phase8/`, orchestrator installs + gates).
- **Unit 8-1 LANDED (scratchpad), orchestrator-reviewed, INSTALLED, gate in flight.** Drafts:
  `CkIntent_Moves_Assets.as` (223 — `UCkTests_Intent_MoveTable` + `asset MoveTable_FortyMove`,
  40 `Declare_Move(name, notation, priority)` lines + 8 `Declare_Button`, `Reset_Declarations`
  re-compile guard) + `CkAutoTest_Intent_FortyMoveBake.as` (228 — entity-free parse→bake→
  assert; count pins, three terminal-zone verdicts LP 0/0, HP hold=45, HK chord=window=4,
  strictly-descending priority walk per asserted row). **Orchestrator verification:** Parse/
  Bake/TryGet_ResolutionRow/Get_DeferralVerdict signatures read against
  `CkIntentGrammar_Utils.h` (match, incl. default-verdict semantics that make LP's 0/0
  structural); terminal zones hand-checked against all 40 rows (LP: 6 rivals none hold/chord;
  HP: Qcf/Charge/Strong/LenientHcf, one hold=45; HK: bare + LK+HK); `5`-neutral standalone
  step verified legal at parser source (`:58`; ChordNeutralDirection is chord-only `:204`);
  criterion-6 self-grep in the unit's return (every ctor site singular+shared, none per-move).
  **Accepted judgment calls:** AS-declared member `Declare_*` methods called unqualified from
  the asset init block (precedent = C++ members `UnmapAll`/`MapKey` in `CkInput_Assets.as:89`;
  residual risk = loud AS compile error, fallbacks enumerated); button vocabulary in-asset;
  globally-unique priorities; hold constant duplicated as notation text + pinned by verdict
  assert; no `_TimeoutSeconds` (entity-free, no waits). Installed to
  `Plugins/CkTests/Script/CkInput/` (2 untracked files; no run was in flight). Gate: test-only
  `Ck_AutoTest_In --discover-fresh --parallel 1` (AS-only change, no build; editor lock
  probed free; expect 120 = 119 + FortyMoveBake by name; wrapper regen + populator churn
  expected in `Script/Generated/` + the `.umap`).
- **Unit 8-1 gate ✅ GREEN: 120/120 (1m15s), first try** — `Ck_AutoTest_Intent_FortyMoveBake`
  passed BY NAME (`Saved/Logs/Test-Phase8Unit1.log`), all 119 prior rows green, zero
  contamination. **Success criterion 6 PROVEN HEADLESS** (40 notation strings from an
  asset-shaped container parse+bake with zero per-move struct construction — the review-grep
  is in the unit's return, recorded above). Pipeline churn as predicted: wrapper row appended
  to `Script/Generated/CkTests_AutoTestActors.as` + 1 populator-placed external actor (staged
  by the populator's own save). **Unit 8-1 CLOSED.** Uncommitted (awaiting user commit
  authorization with the rest of the phase): the 2 new .as files + the 2 generated artifacts.
- **Unit 7-2 LANDED post-ruling** (25 new files ~4380 lines in `Source/CkIntentDebugger/` + 3
  modified: uplugin entry, census row 15→16, launcher CLAUDE.md tool-group row). All five
  views; all reads funnel through ONE file (`CkIntentDebugger_DataCollector.cpp`); Utils-first
  with fragment reads only at the ruled gaps; timeline axis = logic-frame indices (unit-agnostic
  widget, wall-clock would lie under hitches); witnessed-phase ring in the ViewModel (Goap
  history-ring precedent — matcher stores one phase, no history API per [P7-D4]); near-miss
  rows keyed by scan signature not ring position (CkDebuggerCommon selection contract);
  EndPIE + session-invalidated + world-change all route to one snapshot reset; PreExit drops
  the tree (not ShutdownModule). Launcher slot `Interface:30`, icon `Crosshair`; new module
  CLAUDE.md NOT gitignored (check-ignore exit 1 — no add -f needed). Self-greps clean (no anon
  namespaces, no breadcrumbs, teardown symmetric). Full draft `[EDITOR-VERIFY]` queue (8 steps
  incl. criterion-5 scrub with the WindowExhausted-vs-ContiguityBroken contrast) in the unit's
  return — installed into PROGRESS at phase close. **Orchestrator spot-review** (Module.cpp
  + DataCollector.cpp read in full): lifecycle + census + read discipline verified; THREE
  findings sent back as a follow-up batch (same agent): [P7-D5] entity route (see decision
  log), the banned `static_cast<const FCk_Handle&>` reference-alias at collector :196
  (unnecessary — typesafe handles inherit `Get<>`), value casts → `ConvertToHandle()` sweep.
  **Accepted deviation recorded:** Params-fragment reads (sampler/matcher config blobs,
  collector :345/:423) are outside the literal G1-G4 list but inside [P7-D4]'s substance
  (stored config, no Utils surface, read-only). Gate deferred until the follow-up lands —
  ONE gate covers module + fixes (build `--generate` + `Ck_AutoTest_In` + `Ck.DebuggerLauncher`).
- **7-2 follow-up batch LANDED** (same agent): [P7-D5] route registered after spawner+
  descriptor, token-matched unregister FIRST in Shutdown (GOAP's order); route REALLY targets
  — layer target selects owning source in toolbar + its row in the stack panel, source target
  selects the source; pending target reduced to two int32s before storage (no handle survives
  the open-to-first-snapshot gap — strictly safer than GOAP's retained `FCk_Entity`); the two
  re-entrancy hazards handled (pending flag cleared BEFORE setters that broadcast back into
  the apply path; wanted-layer resolved against the snapshot before any setter). Collector
  casts: module-wide sweep grep = zero `static_cast<FCk_Handle...>` hits; :196 → direct
  `Get<>`, two genuine conversions → `ConvertToHandle()`, three sites → bare typesafe
  compares (operator!= availability verified at `CkHandle_TypeSafe.h:94/118`). Module
  CLAUDE.md gained the Entity-targeting section. Two `[EDITOR-VERIFY]` additions (Open In
  from ECS debugger warm + cold tab). **Gate 1 IN FLIGHT:** single-shot build `--generate` +
  `Ck_AutoTest_In --discover-fresh --parallel 1` (editor lock probed free; 8-2 agent writes
  scratchpad only — no source-edit hazard). Gate 2 after: test-only `Ck.DebuggerLauncher`
  (census 16-row assert).
- **7-2 gate attempt 1: BUILD BREAK, fixed inline by orchestrator** (5 sites, 2 error classes,
  all in the new module): (1) `SCkIntentDebuggerWindow.cpp:3` module-header include path —
  the header lives at module root, not under `Public/`; respelled to GOAP's proven form
  `"CkIntentDebugger/CkIntentDebugger_Module.h"`; (2) `auto X = INDEX_NONE` deduces the
  unnamed enum → C2440/C3487 on later int32 assignment — fixed at `TimelineDock.cpp:235-236`
  + `Window.cpp:294,:354` with `static_cast<int32>(INDEX_NONE)` (house typed-cast idiom) and
  an explicit `-> int32` on the timeline's SelectedId lambda; module-wide grep confirmed no
  further instances. Re-gate in flight (`BuildTest-Phase7Unit2b.log`, no `--generate` needed
  — regen succeeded in attempt 1).
- **7-2 re-gate ✅ GREEN: build succeeded + 120/120 (1m9s)** (`BuildTest-Phase7Unit2b.log`) —
  the module compiles in the full build, zero suite regressions. **Census gate ✅ GREEN: 2/2
  (23s)** (`Test-Phase7Unit2-Census.log`) — the 16-row catalog assert passes with
  `CkIntentDebugger` registered at `Interface:30`. **Unit 7-2 CLOSED. PHASE 7 CLOSED under
  [P2-D4]** — exit criteria: scoped gate green (120 incl. the 7-1 diagnostics row) ✔;
  module compiles in full build ✔; docs ✔ (`Source/CkIntentDebugger/CLAUDE.md` authored;
  `CkIntent/CLAUDE.md` stale "Not yet built: debugger lanes" line refreshed at close);
  comment audit ✔ (unit greps + orchestrator review; inline fixes added no comments);
  full suite NOT run ✔. **`[EDITOR-VERIFY]` queue for Phase 7** (drive steps verbatim in the
  7-2 unit returns; summary): (1) launcher entry Interface group below Enhanced Input,
  open/focus/close indicator cycle; (2) toolbar world+source strips populate in PIE;
  (3) layer-stack selection persists across ticks, copy menu works; (4) rosette shows
  RECORDED hysteresis (dot crosses a wedge boundary while the previous spoke stays lit —
  the load-bearing check that it renders, not recomputes) + SOCD policy readout;
  (5) resolution table: immediate vs `hold sibling Nf` rows, `<unbound>` red after settings
  unbind; (6) timeline lanes + deferral marker on BLOCKED lane + scrub LIVE↔SCRUB;
  (7) **criterion 5**: CVar on → fumbled 236 → newest near-miss row names step +
  `WindowExhausted` + frames examined, detail panel walk-order list, timeline scrubs to
  terminal frame; contrast run shows `ContiguityBroken`; (8) teardown: EndPIE empty views
  no crash, re-PIE no AV, editor close with docked tab clean, restart restores tab;
  (+) `Open In ▸ CK Intent Debugger` from ECS debugger selects layer row + owning player,
  warm AND cold tab.
- **Unit 8-2 LANDED (scratchpad), one STOP-lite ruled [P8-D5]** (see decision log). 8 new
  Script/CkInput files + 1-line registry diff (~2330 lines): shared gym namespace (idempotent
  SOURCE composition — subsystem source + bias/map/sampler, forced by anti-patterns 10/6;
  composition from display tick not DoConstruct, source needs a controller first), 3 stations
  on layers 520/510/500 each with own matcher (LatchDecay 600) + own move table via 8-1's
  `Declare_Move` idiom (criterion-6 grep clean: zero move-struct ctors, 10 Declare_Move).
  Station 1 = criterion-1 counter (pad QCF+P vs bare P + keyboard `;` leg, EXPECT 0 delay);
  station 2 = D7 visible (chord `'`+`,` vs suffix `.` — fully keyboard-drivable); station 3 =
  near-miss (w=12 too-slow QCF, pad RB + keyboard `/` legs, CVar armed on arming/cleared in
  EndPlay+exec, WHAT-THAT-MEANS line per outcome, names the debugger view). Keys `;'`,`./`
  grep-proven unclaimed; no autotest rows (per PHASE_8.md, verified no reason to deviate).
  **Accepted evidence-backed deviation:** measured chord gap renders `>= window` not `==`
  (module's own contract, `ChordWindowResolvesPartnerOrTimeout.as:171-172` asserts `>=`;
  the BAKED verdict still renders exact). **Flagged unverified binding:**
  `UCk_InputSource_Subsystem::Get(PC)` from AS (precedent: EnhancedInput subsystem same-shape
  at `CkCameraGym_GameMode.as:37`; all sites validity-guarded) — first compile confirms.
  Install + gate DEFERRED until the 7-2 gate completes (Script/ freeze during runs).
- **8-2 INSTALLED after Phase-7 gates completed** (8 new Script/CkInput files + registry —
  mirror diffed against repo first: exactly the one claimed line). Gate in flight: test-only
  `Ck_AutoTest_In --discover-fresh --parallel 1` (`Test-Phase8Unit2.log`) — gyms add no
  autotest rows, so PASS = AS compile green (editor boot compiles all Script/, exit 76 =
  AS_COMPILE_FAILED would name the line — this also settles the flagged
  `UCk_InputSource_Subsystem::Get` binding) + suite stays 120/120.
- **8-2 gate attempt 1: AS_COMPILE_FAILED (exit 76, results invalid by design), fixed inline
  by orchestrator:** `CkIntentGym_Shared.as:251,:265` used `Cast` as a LOCAL VARIABLE NAME —
  `Cast` is a reserved word in AS (the builtin `Cast<T>()` operator), so `auto Cast = ...`
  parse-errors and `Cast.IsSet()` reads as `Cast<`-missing. Renamed both locals to
  `CastResult`; grep confirmed no other `Cast`-as-identifier sites in the gym files. NEW TRAP
  for the memory file (SelfHeal had no strategy; the error text never names the actual
  problem). Re-gate in flight (`Test-Phase8Unit2b.log`).
- **8-2 re-gate ✅ GREEN: 120/120 (1m17s), zero AS errors** — gym compiles (settles the
  flagged `UCk_InputSource_Subsystem::Get` AS binding), suite unchanged. **Unit 8-2 CLOSED**;
  drive-throughs on the `[EDITOR-VERIFY]` queue (verbatim steps in the unit return: station 1
  = criterion-1's 0-frame counter with pad QCF+A, station 2 keyboard-only D7 contrast,
  station 3 near-miss WindowExhausted/fast-Matched + CVar-off-after-exit check). New AS trap
  (`Cast` reserved identifier) saved to the memory index. **[P8-D3] AMENDED in PHASE_8.md:**
  the Souls delivery-loss station demos the D15 DEFAULT pair only — the original "vs opt-in
  Continue/Inherit" wording contradicted [P5-D6] (opt-ins not constructible in v1); caught
  before dispatch, no executor cost. **Unit 8-3 (Souls gym) DISPATCHED** (scratchpad,
  sequential — may now reference the INSTALLED `CkIntentGym_Shared.as` + registry).
- **Unit 8-3 LANDED (scratchpad), orchestrator-verified, INSTALLED, gate in flight** (7 new
  files ~2100 lines + shared-file extension + 1 registry line — both cross-cutting diffs
  verified against repo before install: registry exactly +1, shared file's only removed line
  the old one-gym title comment). 3 stations at priorities 420/410/400 (+450 menu masker,
  clear of 8-2's), keys `[`/`]`/`\`/`=` + 4 pad buttons grep-proven unclaimed. **Key review
  points:** threshold expectation taken VERBATIM from the battery (`TapVsHold...as:186`
  asserts exact `hold==N`, correcting the dispatch's N−1..N paraphrase — evidence-backed);
  every hold ships a bare-tap rival (forced: `_HoldSiblingFrames` needs ≥2 candidates —
  deferral law); menu key read off the record (no capture, no delegate signature trap);
  display-only hold arithmetic labeled per anti-pattern 23; 10-frame amber grace before any
  red ([P1A-D8]). Accepted judgment calls all precedent-cited. **Flagged:** first in-tree AS
  use of `utils_input_layer::Request_RemoveCapture` (0 prior AS call sites; wrapper shape
  same as AddCapture's 13) — the gate's AS compile settles it. No autotest rows → PASS =
  AS compile green + 120/120 (`Test-Phase8Unit3.log`).
- **8-3 gate ✅ GREEN: 120/120 (1m9s), AS compile first try** — `Request_RemoveCapture`
  AS binding settled. **Unit 8-3 CLOSED**; drive-throughs on the `[EDITOR-VERIFY]` queue
  (verbatim in the unit return: tap-vs-hold exact-45 gap, charge countdown + no-resume,
  menu-eats-charge + RequireRePress persistence). **Unit 8-4 (Debugger gym) DISPATCHED** —
  last Phase-8 unit before the [P8-D4] coverage audit.
- **[P8-D4] coverage audit run** (read-only Opus enumeration + orchestrator ruling
  **[P8-D6]**, see decision log): 89 UFUNCTIONs across the 7 new surfaces, 58 automated (AS
  58/58 of those; C++ automation full on grammar only; BP by specifier + reflection, zero
  assets), 37 zero-automated → ruled: boilerplate/tag-quartet accepted with reasons, 2
  dead-API maintainer flags (`InputSource::Create`, `Get_RoutedEventsThisFrame`), 3 REAL
  gaps → **unit 8-5 dispatched, LANDED, orchestrator-reviewed, INSTALLED** (3 test files
  ~733 lines): `UnbindStopsDelivery` (all 3 uncovered UnbindFrom_* off ONE routed press —
  matcher registers captures on its own layer; dual-handler positive controls on the SAME
  broadcast, snapshot-relative counters, RemoveSingle semantics verified at
  `CkSignal_Fragment.inl.h:185-211`, settle window justified in-file per wait-rule 1;
  orchestrator spot-read the silence/controls steps), `TagKeyedReadsAnswerEmptyOnUnmintedTag`
  (empty + registered-unrelated tag legs, _ByName positive controls on a completed+claimed
  row, contract pre-verified at `CkIntentMatcher_Utils.cpp:477-502`),
  `SubsystemSourceLazyCreateIdempotent` (read-only, nothing composed on the shared source).
  Keys F6/F7 (the only free function keys). Gate deferred to the combined 8-4+8-5 run
  (expect 123 = 120 + 3 by name).
- **Unit 8-4 LANDED (scratchpad), orchestrator-verified, INSTALLED** (8 new files + registry
  +1 + shared-file extension; both cross-cutting diffs verified against repo pre-install:
  registry exactly +1 line, shared deletions = title/paragraph comments + the ONE sampler-Add
  hunk). 4 view-paired stations at priorities 340/335/330/320/310 (clear of both gyms), 16
  `Declare_Move` rows, criterion-6 grep clean. **Accepted judgment calls:** (1) SOCD quad in
  the SHARED sampler composition — the one behavioral shared change; forced (sampler has no
  requests, `_SocdQuad` is Add-time-only, one sampler per source → a station-local compose is
  structurally impossible), blast radius nil (cleaned pair feeds no matching, other gyms
  don't read it, unminted-button quad = documented normal state), fallback recorded (drop the
  hunk → station 3 renders Neutral/Neutral); quad keys NumPad operators, grep-proven free;
  (2) stations 1-2 keyboard-only (pad legs spent where sticks are REQUIRED; 2-move-table
  legibility; one-leg precedent cited); (3) near-miss corpus 6 moves = 3 windows × 2 devices
  (one press still yields exactly 3 distinguishable rows); (4) panel renders Pending-rows AND
  blocked-terminals (the spec's single count over-counts vs the BLOCKED lane — recorded-facts
  grouping, no recompute); (5) frames-examined = last recorded step's own value; (6)
  octants-visited as int32 bitmask (no unreflected TArray<UENUM> member). Combined 8-4+8-5
  gate in flight (`Test-Phase8Units4and5.log`, expect 123 = 120 + 8-5's 3 by name; 8-4 adds
  no rows).
- **Combined gate attempt 1: AS_COMPILE_FAILED — orchestrator inline fix:** 8-4 used
  `EKeys::Gamepad_RightTriggerButton`, which does not exist (`Gamepad_RightTrigger` is the
  real key); 2 sites in `CkIntentGym_Shared.as`. **Re-gate ✅ GREEN: 123/123 (1m8s)**
  (`Test-Phase8Units4and5b.log`) — all 3 gap tests by name (`UnbindStopsDelivery`,
  `TagKeyedReadsAnswerEmptyOnUnmintedTag`, `SubsystemSourceLazyCreateIdempotent`), Debugger
  gym compiles. **Units 8-4 + 8-5 CLOSED.**
- **PHASE 8 CLOSED under [P2-D4].** Exit criteria: 8-1 bake test green by name + review-grep
  recorded ✔; FOUR gyms (Fighting/Souls/Debugger + the pre-existing KeyBinding) compiled,
  registered, [P1A-D7] self-asserting ✔; coverage audit run, 3 real gaps closed+gated,
  accepted/flagged remainder recorded in [P8-D6] ✔; `[EDITOR-VERIFY]` queue updated (all
  gym drive-throughs + the 8-4 panel-vs-debugger-view comparisons, verbatim in the unit
  returns) ✔; PROGRESS current ✔; comment audit over ALL new Script files: zero breadcrumb
  hits (grep over gyms + 5 autotests + assets) ✔; full suite not yet run ✔ — it runs NOW as
  the campaign-end gate. **Expected full-suite arithmetic:** baseline `1027/1025/2` + AS
  In-set additions since (92→123 = +31) + `Ck.Intent.Grammar` C++ battery (+10) +
  `TickRateTrait` clamp (+1) = 1069 total expected, failures EXACTLY the two
  `PathNetworkFollower` names; verdict is by NAME-diff, not arithmetic alone.
- **CAMPAIGN-END FULL SUITE RAN: `1065/1062/3` (11m14s, editor closed,
  `Test-FullSuite-CampaignEnd.log`).** Name-diff vs the baseline log
  (`Test-FullSuite-PostFix.log`): **ZERO lost rows; +38 added, ALL 38 ours, ALL GREEN**
  (37 AS: 30 Intent + 6 ButtonMap + 1 SubsystemSource — the 1027 baseline predated the
  ButtonMap-test install, resolving the arithmetic — + the TickRateTrait clamp row).
  **Prediction miss #1 RESOLVED, not a defect:** the toolbox full-suite discovery includes
  `CkTests.UnitTests.*` + `Project.Functional Tests.*` but NOT greenfield `Ck.*` C++
  prefix families — identically in BOTH runs (0 Grammar/DebuggerLauncher rows in either),
  so the grammar battery (10/10, Phase 4) + census (2/2, today) stand on their own scoped
  gates. Flag to maintainer: greenfield-prefix C++ families are invisible to the toolbox
  full suite. **Prediction miss #2 UNDER INVESTIGATION [INV-B]:** THREE fails, not two —
  the eternal `PathNetworkFollower` pair PLUS `FallsBackToNavigation` (same foreign
  family; passed in baseline; this campaign changed zero nav code; family has recorded
  load-flakiness). Discriminating experiment in flight: isolated `PathNetworkFollower`
  re-run, editor closed — pass ⇒ load/contamination flake under the grown suite;
  fail ⇒ deterministic, needs cross-test-contamination bisection before any delta-zero
  claim.
- **[INV-B] CLOSED — load flake, not a regression.** Isolated family re-run (13 rows, 52s,
  editor closed, `Test-PathNetFollower-Isolated.log`): `FallsBackToNavigation` **PASSED**;
  failures exactly the two eternal names (`DesiredNavmeshClearanceMovesInward`,
  `ProjectsRibbonWaypointWithinNavQueryExtent`). Foreign family, zero nav code changed by
  this campaign, recorded load-flake precedent in the same workstream's area
  (`KinematicPlatformCarriesDynamicBox`, PathRefresh). Flagged to the crowd workstream via
  the human queue. **CAMPAIGN-END GATE: ✅ GREEN + DELTA-ZERO BY NAME** (composition:
  full suite `1065/1062/3` + isolated pass of the third red + name-diff proving zero lost
  rows and all +38 additions green; the only reds anywhere = the two pre-existing
  `PathNetworkFollower` names). **THE CKINTENT CAMPAIGN'S TECHNICAL SCOPE IS COMPLETE** —
  success criteria: 2, 3, 6 proven headless (criterion 2/3 in Phases 5-6, 6 in 8-1);
  1, 4, 5 headless legs proven + `[EDITOR-VERIFY]` legs queued; 7 audited under [P8-D6]
  (AS 58/58, C++ full on grammar, residual flagged); 8 = this gate. Remaining: human
  EDITOR-VERIFY queue, maintainer review flags, commit authorization for today's work,
  and the ship conversation (push never authorized).
- **7-2 STOPped correctly pre-write** (legitimate fork: views 1/2/4 need data with no public
  Utils read API — gaps G1 layer-stack enumeration, G2 active compiled set, G3 non-Completed
  phase frames, G4 pending episodes/hold accumulators; its report enumerated the full
  available-vs-missing read surface per view, views 3+5 fully covered by Utils). **Ruled
  [P7-D4]** (fragment reads permitted, read-only, in-plugin precedent; NO new CkFoundation
  APIs) + **[P7-D3] revisit clause closed** (timeline lane model suffices — spans + events
  with scrub ids). Launcher slot approved from its census read: `Interface:30`, icon
  `Crosshair`, census 15→16. Agent RESUMED with the ruling + guardrails (read-only const
  access; Utils-first, fragment reads only for G1-G4; lifecycle contracts now more
  load-bearing). Awaiting its full implementation return.

- Maintainer answered both open questions in one directive: fix `UnbindConflictAndRemap`, then
  continue the campaign. Ruled [P1A-D9] (see decision log). Dispatched a single Opus unit:
  the 3-line C++ fix (`CkKeyBinding_Utils.cpp:577-584` — `NewKey = EKeys::Invalid` via
  `MapPlayerKey`), the deferred pinning AutoTest (from the 1a-7 "would-pin" list), and retirement
  of the gym step-6 amber CONFIRMED-DEFECT caption (the step's `<unbound>` expectation should now
  pass green on its own). Orchestrator gates with build + scoped `Ck_AutoTest_Input`
  + `--discover-fresh` after return (C++ change → build required → waits for the editor lock).
- **Phase 2 OPENED** (`PHASE_2.md` authored; rulings [P2-D1] 0F settlement, [P2-D2] button
  model reconstruction — the original design PROMPT that defined "two-tier button space" is
  lost, so the model is a fresh orchestrator ruling FLAGGED for maintainer review, [P2-D3]
  source-entity composition). Sequencing: fix unit returns → orchestrator review → build +
  scoped gate → full-suite run with editor closed (triple duty: strict gate-of-record for the
  gym polish, fix verification, Phase-2 entry baseline — expect `1027/1025/2`) → dispatch the
  Phase-2 `InputButtonMap` unit. Serialization law holds: no Script/ or source edits while any
  build/test runs, orchestrator runs every gate.
- **[P1A-D9] fix unit LANDED and orchestrator-accepted** (single Opus unit, no STOPs). The
  defect was corroborated AT ENGINE SOURCE before the edit: `UnMapPlayerKey` →
  `FoundMapping->ResetToDefault()` (`EnhancedInputUserSettings.cpp:1268-1271`) and it skips the
  broadcast when the holder is not dirty — so a default-bound holder produced no change event
  either, matching the Change Signal evidence exactly. `MapPlayerKey` accepts `EKeys::Invalid`
  (`:1133-1250` — NewKey never validated, goes straight to `SetCurrentKey`), and
  `GetMappingNamesForKey` drops Invalid-keyed rows from the key index (`:591-610`). Diff:
  `CkKeyBinding_Utils.cpp:577-585` (the 3-line fix), `.h:346-350` contract reword,
  `CkInput/Claude.md` 2 falsified sentences fixed, NEW
  `CkAutoTest_Input_UnbindConflictAndRemapUnbindsHolder.as` (103 lines — holder-on-default is
  the load-bearing precondition; asserts both `Get_KeyForMapping` AND the key→name index;
  unconditional teardown + restore-proof asserts), gym step-6 amber caption + falsified header
  sentence removed. **Accepted deviations:** teardown mimics the battery (no SaveKeyBindings —
  the test never writes disk; no unregister — no sibling does), no `_TimeoutSeconds` override
  (CkTests/CLAUDE.md marks the actor-wrapper wording stale; synchronous test), 2 extra restore
  assertions (only test that leaves a row unbound). Orchestrator spot-checks: `Assert_False`
  exists (`CkAutoTest_Base.as:632`), `<unbound>` rendering confirmed
  (`CkInputGym_Shared.as:226`), asset idiom matches `RemapRoundTrip:46`, C++/h diffs read
  exactly to spec. Call-site census: only the gym (2 sites) + the new test call it; both gym
  sites already assert `<unbound>` (were red under the defect, go green now); step-7
  `RemapKeys` re-binds all four rows so the demo cycle is self-healing. BusterBlock NOT swept
  (downstream, pointer-bump concern). **Gate ✅ GREEN: 22/22 (31s)** — build succeeded, the new
  pinning test PASSED BY NAME (`Saved/Logs/BuildTest-UnbindFix.log`). Full suite (test-only,
  `--no-nullrhi --parallel 1`, editor closed) launched as the triple-duty run (gym-polish
  gate-of-record / fix verification / Phase-2 entry baseline; expect `1027/1025/2`).
- **Phase-2 unit DISPATCHED in parallel with the full suite** (single Opus unit,
  `InputButtonMap` per PHASE_2.md): C++ writes direct (test-only run uses built binaries), ALL
  AS test files drafted to scratchpad `phase2/` — installed by the orchestrator only after the
  suite completes ([P1A-D2] precedent). STOPs: new subsystem, scheduler cycle, absent-settings
  Result-semantics fork with no precedent, any unenumerated design fork.
- **Full suite ✅ GREEN, DELTA-ZERO: `1027/1025/2/0/0`, 10m31s**
  (`Saved/Logs/Test-FullSuite-PostFix.log`, editor closed) — exactly the prediction. Failing
  names = the SAME two pre-existing `PathNetworkFollower` rows, nothing else. Three claims this
  single run settles: (1) the [P1A-D7]+[P1A-D8] gym rework is regression-free under the strict
  gate-of-record — the earlier Jolt red (`KinematicPlatformCarriesDynamicBox`) PASSED with the
  editor closed, confirming the load-flake diagnosis and CLEARING that caveat; (2) the
  [P1A-D9] fix is verified suite-wide (+1 test vs the 1026 baseline, all green); (3) **Phase-2
  entry baseline = `1027/1025/2`** with those two names — Phase 2 diffs against this.
- **Phase-2 unit LANDED and orchestrator-accepted** (single Opus unit, NO STOPs — both
  potential forks had in-module precedent). 6 new C++ files (`CkInputButtonMap_*` quartet,
  ~990 lines), `CkInputSource_Subsystem` gains the `OnSettingsChanged` → `Request_Rederive`
  seam (late-binding discipline mirrored from source creation, unbound in `Deinitialize`,
  `Cast`-guarded for the opt-in map), `CkInput/CLAUDE.md` +1 section + anti-pattern #13
  narrowed (its "two halves share nothing" claim was falsified by this feature). NO new
  processor group (HandleRequests drains in `FGroup_Input_Collect`, cancel in
  `FGroup_EndPlay`); no Build.cs change. **Key precedents verified by the orchestrator:**
  `_CompletionDelegate` is `mutable` in `FCk_Request_Base` so the const-ref
  `Set_CompletionDelegate` shape matches `InputBias_Utils.cpp:184` exactly; handle meta
  matches all 3 sibling handles; absent-PC re-derive early-outs BEFORE the clearing pass
  (transient absence never wipes state) and reports `Succeeded` (retryable ≠ Failed, house
  Result rule). **Accepted judgment calls:** slot-First association (a name with only
  non-First rows keeps an identity holding Invalid), clear-then-rebuild for Mapped tier
  (vanished mapping → Invalid key, identity retained), params read-only at runtime (table is
  single source of truth), flat array no key-index, no struct formatter for ButtonId (only
  `CkFormat_Defaults.cpp` precedent — logs print tier/name separately), test keys F1/F2/F9-F11
  grep-verified unused. **Orchestrator fixed inline:** one `In`-prefix param-name drop in the
  subsystem .cpp (the exact style deviation the old module was flagged for). 6 AS tests
  installed from scratchpad AFTER the full suite completed ([P1A-D2] discipline held).
  **Scoped gate ✅ GREEN: 28/28 (33s)** — quartet + subsystem edit compiled first-try, ALL SIX
  ButtonMap tests passed by name (`Saved/Logs/BuildTest-Phase2.log`). Comment audit: breadcrumb
  sweep over the quartet = zero hits.
- **Phase 2 CLOSED under [P2-D4]:** scoped gate 28/28, doc subsection landed + reviewed,
  decisions recorded, comment audit clean. The phase-close full suite was killed mid-flight on
  the maintainer's directive — **full-suite delta-zero for Phases 2+ is DEFERRED to campaign
  end** (final diff vs `1027/1025/2` + all names added since). Phase 3 opens next: the
  `CkIntent` module (sampler / frame record / ring / octant+SOCD).
- **Phase 3 OPENED** (`PHASE_3.md` authored; rulings [P3-D1] module scaffold + one-way dep
  law, [P3-D2] D19 clamp per the 0H-researched shape — the phase's high-blast edit, [P3-D3]
  sampler/record/ring on the source entity at Hz(60), [P3-D4] router delivery-retention
  fragment — FLAGGED for maintainer review, first structure DESIGN_PollSurface anticipated,
  [P3-D5] octant/hysteresis/SOCD deferred to unit 3-2). **Unit 3-1 DISPATCHED** (single Opus
  unit): scaffold + clamp + retention + sampler quartet + 5 AutoTests. Gate on return: build
  `--generate` (uplugin gains a module) + scoped pattern `Ck_AutoTest_In` `--discover-fresh`
  (expect 28 + new Intent rows). STOPs: 0H-vs-code contradiction, scheduler cycle, undrivable
  clamp test (STOP-lite: implement + report gap), any unenumerated fork.
- **Unit 3-1 LANDED with one legitimate STOP** (13 new CkIntent files ~991 lines + clamp on
  `TProcessorBase` + router retention + 4 AS tests + 1 C++ clamp test). Orchestrator verified
  the high-blast `CkProcessor.h` diff line-by-line: pure insertions + ONE modified line
  (`else` → `else if constexpr (unlimited)`), unlimited branch byte-identical, misuse guarded
  by 3 static_asserts, drop-don't-defer + single `ck::ecs::Log` report (deliberately not
  Warning — AutoTest harness escalates warnings; why-comment at the call site). Clamp
  implemented VERBATIM to 0H's researched shape; clamp test added to the EXISTING hermetic
  `Test_Processor_TickRateTrait.cpp` fixture (clamped=2 vs unclamped=4 over one
  `TickWith(1.0)`, drop-not-defer + cadence-resume proven). **Gate note recorded:** that row
  is `CkTests.UnitTests.CkEcs.Processor.*` — needs a SECOND test pattern (`TickRateTrait`)
  beside `Ck_AutoTest_In`. **The STOP — ruled [P3-D6]:** Hz(60) sampler + per-render-frame
  retention loses edges >60 fps / duplicates <60 fps. Ruling = accumulator option (A) refined
  with `FGroup_Intent_Collect` (same-group order is not a guarantee — house law), pending
  buffer consumed-and-cleared by the rated sampler, hitch attribution documented. Follow-up
  assigned back to the 3-1 agent (context intact); gate deferred until it lands.
- **[P3-D6] fix LANDED** (same agent, context intact; CkIntent-only + 1 new test, CkInput/
  CkEcs untouched this round). `FGroup_Intent_Collect` spliced (edges declared once each,
  chain `... Route → Intent_Collect → Intent_Sample → Gameplay`); unrated
  `FProcessor_IntentSampler_Collect` appends retention → pending buffer; sampler
  claims-by-value-and-clears BEFORE deriving (orchestrator spot-checked
  `CkIntentSampler_Processor.cpp:74-75`); held-keys + raw-axis fallback read claimed events;
  Claude.md defect text REPLACED by the buffered contract. New test
  `NoEdgeLostAcrossSkippedRenderFrames` pins exactly-one-edge with three non-vacuity guards
  (frame-index-based settle, not render hops). **Accepted judgment calls:** buffer unbounded
  by design (bounded in practice by one logic frame; a cap would drop the edges it exists to
  keep), `Claimed` copied not moved (row must own storage), sampler's first rows may precede
  router-state stamping (benign, tests gate on content). **Gate 1 ✅ GREEN: 97/97 (58s)** —
  the `Ck_AutoTest_In` substring caught Interaction/Inventory suites too (free regression
  coverage over the D19 base-class edit's blast radius); all 5 Intent rows passed BY NAME;
  CkIntent module compiled + generated + discovered first-try
  (`Saved/Logs/BuildTest-Phase3Unit1.log`). **Gate 2 ✅ GREEN: 3/3 (28s)** —
  `TickRateTrait_ClampBoundsCatchUpReplay` passed + both pre-existing trait tests unchanged
  (`Saved/Logs/Test-TickRateTrait.log`). **Unit 3-1 fully gated.** Unit 3-2 (octant/
  hysteresis/SOCD, [P3-D5]) dispatched next — fresh Opus agent, extends the landed sampler
  quartet additively (the 5 existing Intent tests must stay green).
- **Unit 3-2 LANDED, orchestrator-accepted** (no STOPs; +466 C++ lines across the sampler
  quartet + 4 new AS tests, 859 lines; additivity preserved — no existing field/getter/
  signature changed). Orchestrator verified the octant algorithm by reading `DoRecordOctant`:
  neutral-pass RESETS the hysteresis memory (why-comment: gesture ended, memory must not bias
  the next one), hold-band asymmetry = half-width + margin, `FindDeltaAngleDegrees` handles
  the W-octant wraparound; hand-checked test 1's angle walk (26°→E held, 30°→NE, 21°→NE by
  travel direction, 5°→E). **Defaults proposed:** neutral radius 0.25 (≈ XInput stock
  deadzone), hysteresis 5° (~11% of an octant); flagged in Claude.md as proposal pending 0A.
  **Accepted judgment calls:** `FCk_Intent_SocdQuad` struct (all-or-none structural),
  quad-distinctness rejection added (mirrors AxisKeysAreDistinct), press-order as
  oldest-first ButtonId array rebuilt from claimed arrival order, one `ECk_Intent_CleanedAxis`
  enum on two row fields, SOCD quads from tier-2 Physical buttons (no binding-profile
  dependency), policy-disagreement test uses two samplers in one test (single policy is
  unfalsifiable alone), NO new utils. Follow-up noted: `Source/CLAUDE.md` decision-tree row
  now incomplete (octant/SOCD missing) — orchestrator takes it at phase close. Gate in
  flight: build + `Ck_AutoTest_In` `--discover-fresh` (expect 101 = 97 + 4; CkEcs untouched
  → no trait re-run).
- **Unit 3-2 gate ✅ GREEN: 101/101 (1m0s)** — all 4 octant/SOCD tests by name, the 5 earlier
  Intent tests green = additivity proven (`Saved/Logs/BuildTest-Phase3Unit2.log`).
  Orchestrator took the flagged follow-up (Source/CLAUDE.md decision-tree row now names
  octant/SOCD). Comment audit over ALL CkIntent source: clean (2 hits, both domain "phase").
  **PHASE 3 CLOSED** under [P2-D4].
- **Phase 4 OPENED** (`PHASE_4.md` authored): rulings [P4-D1] grammar v1 (numpad directions
  onto the octant vocabulary, `+` chords, whitespace sequences, `w=<frames>` in LOGIC frames
  — D9's `w=200` re-read as frames under frame-determinism, `lenient`, `hold=<frames>`; flat,
  no conditionals) — RECONSTRUCTION, maintainer review flag (original design doc lost);
  [P4-D2] definition model (parser is the only producer, explicit int priority, tie =
  rejection); [P4-D3] bake = pure function → `FCk_Intent_CompiledSet` with per-terminal
  resolution tables + D7 deferral VERDICTS AS DATA (sequence-suffix never defers — success
  criterion 2 baked in); [P4-D4] "cycle validation" reconstructed as strict-total-order
  validation over shared-terminal priorities; [P4-D5] pure data + free functions, zero ECS
  this phase, AS/BP `Parse`/`Bake` surface. Units: 4-1 parser (dispatched), 4-2 bake (after
  4-1's gate).
- **Unit 4-1 LANDED** (no STOPs): `CkIntentGrammar_Data.h` (375) + `_Utils.h/.cpp` (87+397)
  + 505-line hermetic C++ test (6 tests, `Ck.Intent.Grammar.*` greenfield family, 13
  rejection reasons each with a distinct asserted reason) + entity-free AS notation test +
  Claude.md authoring section. Two justified Build.cs edits: `GameplayTags` → CkIntent
  (recorded LNK2019 lesson), `CkIntent` → CkTests (C++ test include path). **Evidence-backed
  shape calls:** result struct mimics `FCk_2dGridPlacement_Result` (enum-mode + reason +
  empty-definition-on-reject); NO ExpandEnumAsExecs (repo sweep: exec pins ONLY on DoCast
  family, never BlueprintPure); "parser is the only producer" enforced via friend +
  CK_PROPERTY_GET-only + private all-fields ctor + BlueprintReadOnly (public default ctor =
  the empty rejection value, precedent `FCk_Nav_PathResult`); `ECk_Intent_Lenience` enum not
  bool. **Binding rule:** digit-run in a chord token emits prefix digits as standalone steps,
  last digit joins the chord — `236+LP` → `[S][SE][E+LP]`, uniform over token position.
  Edges settled: case-insensitive modifier keywords (cost: no button named `lenient`),
  `=`-containing token is always a modifier, duplicate modifiers reject (never last-wins),
  digit-leading token IS a direction run (enforces button-name charset), `0` window rejects
  (stored 0 = undeclared, unambiguous), malformed input logs Verbose only (no ensure — AS
  rejection leg needs no ExpectedLogErrors). Gate 1 in flight (build `--generate` +
  `Ck_AutoTest_In` `--discover-fresh`, expect 102); gate 2 = `Ck.Intent.Grammar` pattern
  after (memory: brand-new C++ test .cpp discovers fine when its TU links this build;
  recovery if "No tests matched" = touch + rebuild).
- **Unit 4-1 gates ✅ GREEN: 102/102 (1m3s) + 6/6 (25s)** — AS notation test by name (the
  generated `utils_intent_grammar` surface exists and works), all 6 C++ grammar tests incl.
  the 25-row/13-reason rejection matrix (`BuildTest-Phase4Unit1.log`, `Test-IntentGrammar.log`).
  **[P4-D6] ruled at 4-2 dispatch:** chord simultaneity window = BAKE parameter
  (`ChordWindowFrames`, uniform per set, default 3 logic frames), not notation — the grammar
  has no chord-window syntax and D9's revisit clause resists growing one; feeds the D7
  chord-membership verdicts. **Unit 4-2 (bake) dispatched to the SAME agent** (context holds
  the model): compiled set + resolution tables + D7 verdicts-as-data (sequence-suffix
  no-deferral must be the structural DEFAULT, not a computed special case) + [P4-D4]
  tie-rejection + parser-as-fixture C++ tests + AS bake leg.
- **Unit 4-2 LANDED** (compiled-set data 569 lines + bake in the same utils class + 447-line
  C++ test + AS bake row; no Build.cs change). **Key shapes:** verdict = two named int32
  causes (`_HoldSiblingFrames`/`_ChordMemberFrames`, 0 = absent — combining left to the
  matcher); resolution rows = index arrays into the set's own `_Intents` (no name/priority
  duplication); deferral rows store ONLY deferring buttons — no-deferral is what happens when
  nothing wrote a row, structurally. **Accepted judgment calls:** a DIRECTION in a chord is
  not chord ambiguity (directions are frame-record state on the press frame; only a second
  BUTTON can still be in flight — this is what makes `236+LP` legally zero-deferral);
  both causes require ≥2 candidates (`HasRivals` — D7's "forward ambiguity only" as one
  gate); chord-vs-chord rivals defer too; compiled intents do NOT carry source definitions
  (one authoritative step list); pure-direction-terminal intents contribute no resolution row
  (documented Phase-5 gap: direction-driven triggers); tests pass ChordWindowFrames=4 to
  prove the argument is carried, not the default. **[P4-D7] ruled at review** (the unit's
  flagged fork): name→ButtonId duplicate rows — same-value idempotent, conflicting-value
  REJECTS (`ConflictingButtonRow`, names button + both ids); first-match-wins-silently was a
  non-negotiable-#3 violation. Fix assigned to same agent; gate after it lands.
- **[P4-D7] fix LANDED** (+15/+24/+47/+7 across the four files): vocabulary check placed
  BEFORE any row is read (first-match becomes a proven consequence, not a policy);
  `_ConflictingButton` field mirrors the `_OffendingIntent`/`_ConflictingIntent` naming;
  both identities carried (the fix is deleting one of two rows — showing one is useless);
  idempotent-duplicate BAKES leg + conflicting-duplicate REJECTS leg added. AS deliberately
  NOT extended (would prove nothing new; enum + ButtonId getters already exercised AS-side).
  Phase-4 closing gates in flight: build + `Ck_AutoTest_In` (expect 103 = 102 + bake AS
  row), then `Ck.Intent.Grammar` (expect 10 = 6 parser + 4 bake).
- **First gate attempt: BUILD BREAK, root-caused + fixed inline by orchestrator.** LNK2019 in
  `Test_IntentGrammar_Bake.cpp`: unresolved dllimport `FCk_Input_ButtonId::{operator==, 2-arg
  ctor}`. Cause: `CkTests.Build.cs` never declared `CkInput` (AS tests reach it via
  reflection; this is the first CkTests C++ TU constructing a ButtonId), and the 4-1 agent's
  "comes transitively through CkIntent's public deps" claim did not hold for LINKAGE on this
  toolchain — includes propagated (it compiled), the import lib did not (it failed to link).
  Fix: `"CkInput"` added to CkTests deps (one line, alphabetical slot; same class of fix as
  the recorded GameplayTags-LNK2019 lesson). Rebuild `--generate` + gate re-running.
- **PHASE 4 CLOSED under [P2-D4]:** post-fix gates ✅ `103/103` (1m5s, both grammar AS rows
  by name) + ✅ `10/10` (21s, full C++ battery incl. [P4-D7] legs). Comment audit: zero hits
  across all four grammar files. Reconstruction flags standing for maintainer review:
  [P4-D1] grammar, [P4-D4] cycle-validation reading, [P2-D2] button model.
- **Phase 5 OPENED — the poll-surface ruling batch.** Orchestrator adversarially reviewed
  `DESIGN_PollSurface.md` and ACCEPTED its analysis (its load-bearing reframe verified:
  D15-revised's text already mandates the delivery rows, and [P3-D4]/[P3-D6] BUILT the
  per-event substrate). All five questions it demanded be ruled in one sitting are ruled in
  `PHASE_5.md`: **[P5-D1]** a layer IS the compiled-set anchor (`IntentMatcher` feature on
  the layer entity; D8's vehicle case is the motivating case); **[P5-D2]** α/β/γ = γ
  (per-layer matching over layer-visible events; visibility derived from recorded outcomes;
  α unrepresentable, β's recording need already met); **[P5-D3]** Shape C is the primary
  read API WITHOUT v1 intent entities (tag-keyed reads on the matcher handle; persistent
  rows in one stable fragment — kills C-F3; C-F5 deferred with Phase 6's entity decision);
  **[P5-D4]** claim = immediate mutator (declared escape hatch + reason), per-layer free
  under γ, through-mask unrepresentable, NO down-stack claim, same-layer order accepted +
  documented; **[P5-D5]** naming N1 — capture keeps `Consume`, gameplay verb is
  `Request_Claim`; **[P5-D6]** D15 policies = default pair only in v1; **[P5-D7]** captures
  follow the SET, rebinds follow the MAP (activation resolves ButtonIds→FKeys through
  InputButtonMap, re-derive re-registers — success criterion 4's wiring). Dossier appendix
  items: O13/O9 stale-table rows acknowledged (already-closed items; table cleanup deferred
  to campaign close); naming hazard resolved by [P5-D5]; immediate-vs-deferred now declared
  by [P5-D4]. Units: 5-1 matcher core (dispatching), 5-2 deferral/holds/claim.
- **Unit 5-1 LANDED, orchestrator-accepted** (no STOPs; 6 new `CkIntentMatcher_*` files
  ~1499 lines + 5 AS tests ~1267 lines + group `FGroup_Intent_Match` spliced after Sample;
  zero additions to gated Phase-4 surfaces — one falsified comment corrected in
  `CkIntentGrammar_Data.h`, flagged). **Load-bearing shapes verified by orchestrator:**
  visibility predicate handles the dead-consumer edge conservatively (unrankable → not
  visible); scan bookkeeping clamps unscanned rows to the ring, resets on swap, replays
  oldest-first, missing row fails safe (spot-read `:413,:476-480`). **Accepted judgment
  calls:** swap drains in `FGroup_Intent_Collect` (deterministic set-before-scan without a
  4th group; disjoint fragments from the sampler's collector), matcher UNRATED using record
  frame indices (two 60 Hz accumulators have no defined phase alignment), terminal accepts
  pressed-or-held / prefix requires a visible press EDGE (chord = "down together", sequence
  = "an input"), capture edits DIFFED (two terminals may share a key), swap-reject logs
  Verbose (unbound mapping is legitimate player state + avoids warning-escalation),
  unknown==uncompleted (`Idle`/INDEX_NONE, anti-pattern #19), rebind test uses Flashlight
  F→F3 (no other test remaps it). 5-1 deferral boundary: nonzero-verdict buttons SKIPPED
  with one Verbose per activation (5-2 implements). Gate in flight (build + `Ck_AutoTest_In`
  `--discover-fresh`, expect 108 = 103 + 5).
- **First 5-1 gate attempt: AS COMPILE FAILED (toolbox correctly invalidated the run —
  AS_COMPILE_FAILED verdict, stale bytecode never trusted).** Two agent errors, both known
  traps, fixed inline by orchestrator: `Request_Rederive(_Map)` missing its request-struct
  arg (×2 in `RebindMovesTheMatch.as:157,182` — the AS wrapper defaults only the trailing
  delegate) and adjacent string literals in `SuffixTerminalNeverDefers.as:188` (the recorded
  no-splice trap; SelfHeal named it but has no strategy). Gate re-running.
- **Unit 5-1 gate ✅ GREEN: 108/108 (1m4s)** — all 5 matcher tests by name
  (`BuildTest-Phase5Unit1b.log`). **Success criteria 1, 2, 4 now PROVEN HEADLESS:**
  completion frame == press-row frame (criterion 1), suffix-terminal-never-defers with live
  arbitration (criterion 2), rebind-moves-the-match with zero definition edits (criterion 4
  headless leg; the `[EDITOR-VERIFY]` CkGameSettings-page leg remains human). Unit 5-2
  dispatched to the same agent (context intact): deferral execution (Pending phase, chord
  window, hold thresholds from the baked verdicts), D11 hold accumulator, `Request_Claim`,
  D15 default-pair on delivery loss, criterion-3 tap-vs-hold threshold±1 tests.
- **Unit 5-2 LANDED** (no STOPs; matcher files grew to ~2488 lines total + 5 AS tests
  ~1219 lines; ZERO new processors/groups — the Match processor owns episodes, "two readers
  of one ring would race the hold count"). **Shapes:** episodes keyed by FRAME not offset
  (offsets slide); two armed-cause flags not a mode enum (a button can be both); accumulator
  separate struct per D11, policy-applied (advances only down+deliverable). **Resolution
  order per row:** delivery-check → accumulator → chord branch (terminal=this row) → hold
  branch (terminal=press row) → disarm → final resolution (both disarmed; scan anchored on
  press row, completion frame = resolution row — latency paid, history not rewritten).
  **Accepted judgment calls:** losers → `Idle` not `Failed` (Failed = a wait that matched
  NOTHING); episode-open clobbers candidate latches; swap drops episodes (withdraws the
  question); any completion purges episodes listing that intent (no chord double-complete);
  fresh episodes advance once against their own press row (partner-already-down = zero
  latency); delivery LOSS polled prospectively via read-only cross-module fragment view
  (would-this-key-reach-me — correct question, distinct from the historical per-event
  predicate; precedent = sampler's Collect view, no CkInput edits); claim rejections split
  `Failed_NotEnqueued` (handle validity) vs `Failed` (state refusals = answers). Both AS
  lessons applied (request-structs everywhere, zero adjacent literals, grep-verified). Gate
  in flight (expect 113 = 108 + 5).
- **Unit 5-2 gate ✅ GREEN: 113/113 (1m6s), first try** — all 5 by name
  (`BuildTest-Phase5Unit2.log`). **Success criterion 3 PROVEN** (tap at threshold−1, hold
  completes ON the threshold frame, held-past still reports the threshold frame). Comment
  audit over all matcher files: zero hits. **PHASE 5 CLOSED under [P2-D4].** Criteria 1-4
  all proven headless; remaining: 5 (debugger, Phase 7 + EDITOR-VERIFY), 6 (~40-move AS
  bake, Phase 8), 7 (three-environment, ongoing), 8 (full suite, campaign end).
- **Phase 6 OPENED** (`PHASE_6.md`): **[P6-D1]** signals on the MATCHER entity, identity in
  the payload — NO per-intent entities, closing [P5-D3]'s deferred question and retiring
  C-F5 permanently; **[P6-D2]** latch decay to Idle after `_LatchDecayFrames` (default 20 ≈
  333 ms buffer feel), claim cleared with it, Pending never decays, decay owned by the Match
  processor (single-reader rule) — without decay a stale completion is claimable seconds
  later = the detonation bug reborn at the claim level; **[P6-D3]** exactly two signals
  (`OnIntentPhaseChanged` full transitions incl. decay, `OnIntentCompleted` presentational)
  fired from the one phase-write helper (structural can't-miss, not disciplinary), D6's
  last-payload law restated at the API. Unit 6-1 dispatched to the matcher agent (context
  intact).
- **Unit 6-1 LANDED, orchestrator-accepted** (no STOPs; additive-only on gated 5-x surfaces;
  5 new AS tests ~1259 lines; no new processors/groups — decay is the Match processor's last
  per-row pass). **Structural enforcement EXCEEDS spec:** `FIntentMatcher_PhaseRow`'s friend
  list narrowed to `{PhaseWriter, Utils}` — a direct phase write NO LONGER COMPILES;
  orchestrator ran the agent's own verification grep: `_Phase/_PhaseFrame` written ONLY at
  `Set_Phase` (:433-434), claims only at the stamp + the writer's clear. 6 call sites
  inventoried (swap→Idle DOES signal — observable phase move; dropped episodes silent —
  different thing, documented). **Binding policy:** default `FireIfPayloadInFlightThisFrame`
  — the only policy that cannot contradict the poll (a latch cannot decay in its stamping
  frame); `FireIfPayloadInFlight` documented opt-in with its decayed-payload hazard.
  **Accepted calls:** any transition clears the claim inside the writer (subsumes 5-2's
  per-site clears); writer fires on phase-or-frame change (re-completion is a real event,
  no-op tick silent); 6-param delegate verified against variadic signal templates;
  `_LatchDecayFrames` optional-with-default (5-x tests untouched). One stale doc breadcrumb
  (anti-pattern 15 named a Phase) removed. Gate in flight (expect 118 = 113 + 5).
- **Phase-6 gate attempt 1: 116/118 — both reds in the NEW decay path** (everything prior
  stayed green). `PhaseChangedObservesFullOrder`: decay-gap arithmetic got **-32** (fits
  payload frame = INDEX_NONE: -1 − completion ~31); `LatchDecayClearsClaim`: `Check_Decayed`
  never true in 1.04s on a CLAIMED row (fits a claim guard wrongly skipping decay).
  Failures + hypotheses bounced to the 6-1 agent for confirmed root-cause + fix; design
  forks return, mechanical fixes proceed.
- **Decay reds ROOT-CAUSED + FIXED** (hypothesis (a) CONFIRMED at
  `CkIntentMatcher_Processor.cpp:1155-1156` — decay passed `INDEX_NONE` into the signal
  payload; -32 = -1 − completion 31; the claimed-row "no decay" was OBSERVATIONAL — the
  test's readiness gate required `_DecayFrame >= 0`; hypothesis (b) disproven, no claim
  guard exists). Fix: real frames threaded through `DoDecayLatches` AND
  `DoSettleLosingRows`/`DoPurgeEpisodesResolvedElsewhere` (same defect — losers' 
  `Pending→Idle` also reported -1). Zero test-assertion changes needed (`TryGet_Completion*`
  gates on phase, not frame). **Bonus hardening:** agent found a latent render-rate coupling
  it had introduced — `WaitFrames` counts RENDER frames, decay counts LOGIC frames; three
  latch-holding tests pass only above 60 fps — hardened with explicit
  `Set_LatchDecayFrames(600)` (the precondition made explicit; they are not decay tests).
  **[P6-D4] ruled:** the swap-reset's `→ Idle` payload KEEPS `INDEX_NONE` — the swap drains
  before this frame's row exists; an administrative transition tied to no row should say so
  rather than borrow the last row's index and invent precision. Documented as the one
  explicit exception. Re-gate in flight.
- **Phase-6 re-gate ✅ GREEN: 118/118 (1m8s)** — both former reds pass with ZERO assertion
  changes (`BuildTest-Phase6b.log`). **PHASE 6 CLOSED under [P2-D4].**
- **Phase 7 OPENED** (`PHASE_7.md`): [P7-D1] the debugger renders recorded facts, never
  recomputes (dossier's B-debugger argument → doctrine); [P7-D2] near-miss needs new data —
  opt-in scan-diagnostic ring on the matcher, `ck.Intent.RecordScanDiagnostics` CVar
  default OFF, answers criterion 5's "which step timed out and by how many frames";
  [P7-D3] `CkIntentDebugger` module mimicking CkGoapDebugger (category + MVVM +
  `SCkDebug_EventTimeline` reuse); rosette = part of the key/state view. Units: 7-1
  diagnostics (dispatched to the matcher agent), 7-2 debugger UI (fresh agent, must load
  ck-gameplaydebugger-extension + ck-slate-tools skills first; visual layer =
  EDITOR-VERIFY).
- **COMMIT CHECKPOINT (user-authorized, /commit):** 6 CkFoundation commits
  (`241048f2d` fix → `46f92c369` ButtonMap+seam+doc → `6d7431b15` retention → `e90384363`
  clamp → `0bb6febde` CkIntent module minus matcher → `8f27fd68f` campaign docs) + 4
  CkTests commits (`9fc7ffb7` gym → `e552626b` pins → `c6dc4605` intent battery+Build.cs →
  `6260c453` pipeline artifacts). Matcher files + `CkIntent/Claude.md` + the 7-1 test HELD
  BACK (7-1 in flight at commit time) — they get one commit post-review with fresh
  authorization. Push NOT authorized; pointer bumps deferred (publish guard). Continuation
  prompt written: `CONTINUATION_PROMPT_Phases7and8AndShip.md` (this dir).
- **Unit 7-1 LANDED, orchestrator-accepted** (STOP-lite exercised as permitted: deferral
  EPISODE outcomes get no ring entries — window expiry already IS the per-frame failed
  chord scans; hold cancel runs no scan and belongs to the phase signal; verbatim reasoning
  recorded in its report + Claude.md). Shapes: 32-entry ring, entry = intent identity +
  terminal frame + outcome + per-step walk-order detail; **`WindowExhausted` vs
  `ContiguityBroken` is the criterion-5 distinguisher**; `_StepIndex` = definition index so
  authored order recovers. CVar `ck.Intent.RecordScanDiagnostics` owned by Utils (one
  source of truth), read once per pass. **Funnel exclusivity orchestrator-verified by
  grep:** `Get_ScanSucceeds` has ONE caller (`DoRunScan` :1235→:1246) which has exactly 2
  sites (:886 immediate arbiter, :1017 all episode branches) — the ring cannot describe a
  subset. Accepted: swap does NOT clear the ring (entries describe scans that happened,
  [P7-D1] coherent); ring-fill cost note documented (wide chord windows; ring small + off
  by default). Gate in flight (expect 119 = 118 + 1).
- **7-1 gate: 118/119 on the suite run, then GREEN after two ORCHESTRATOR test fixes**
  (both in the new test, code untouched): (1) the exact frames-examined pin lacked its
  fixture guarantee — the scan clamps to retained rows, so pressing before the ring held a
  full window made retention the binding constraint (expected 9, got 4); fixed with a
  `Check_RingWarm` wait (`Get_FrameCount >= 12`); (2) **leg 2 never released the punch** —
  leg 1's assert step injects `Released`, leg 2's did not; the suite run's timing masked it,
  the isolated run wedged on `Check_Released`; fixed by injecting the release at the TOP of
  `Step_AssertFailure` (early-return guards can't skip it). Isolated re-run ✅ 1/1 (27s,
  `Test-ScanDiag2.log`); composition with the suite run's 118 greens = **unit 7-1 CLOSED**.
  The `[EDITOR-VERIFY]` queue and Phase-7 close await unit 7-2 (debugger UI).

### 2026-08-08 — [P1A-D7] gym's first drive: CONFIRMED defect #13 + presentation rulings [P1A-D8]

- **CONFIRMED shipped defect #13 (NEW — not among the 12 suspected):**
  `UCk_Utils_KeyBinding_UE::UnbindConflictAndRemap` does not unbind a conflict holder — the
  unbind loop calls `Settings->UnMapPlayerKey` (`CkKeyBinding_Utils.cpp:583`), which REMOVES THE
  PLAYER'S CUSTOMIZATION (revert-to-default), not the binding. A holder sitting ON its authored
  default is a no-op: it keeps the contested key → silent DUPLICATE binding, no change event.
  Evidence (maintainer's live gym drive, 3 independent assertions): step-6 verdict
  `Interact EXPECT <unbound> -> GOT E` red; profile shows Jump AND Interact both on E;
  Change Signal shows Interact's listener NEVER fired across multiple laps while the other
  rows fired 4-15 times. Mechanism is the only reading consistent with all three. The old
  exec-driven gym demoed this function for weeks and printed "previous holder is cleared"
  unconditionally — the self-asserting redesign caught it on first drive.
  **True fix (maintainer decision, NOT applied — semantic choice + CkGameSettings/BusterBlock
  consume this path):** `MapPlayerKey` with `EKeys::Invalid` (the mechanism SwapKeys' trap
  proves works), or rename/redocument to revert-to-default semantics. **Escalate with 1-5+7.**
- **[P1A-D8] gym color contract (rules the panels the maintainer flagged):** Red = a true
  failed assertion ONLY. A CONFIRMED defect renders its red verdict + an amber "CONFIRMED
  DEFECT (recorded, escalated)" annotation — the gym's job is to show it, labeled, not to
  expect-around it. Demo-driven divergence (rows off-default mid-lap) = amber, never
  red-with-apology. Environment absence (this host ships no CommonUI controller-data key art →
  every glyph misses) = one amber note, not N red rows; per-row red only when SOME rows
  resolve (differential = meaningful). Change Signal's 4/4 verdict DECOUPLED from defect #13
  by adding Interact to the batch step — every row then moves per lap on healthy paths alone.
- **[P1A-D8] fixes LANDED** (single Opus unit, orchestrator-reviewed: no stale callers of the
  re-signatured glyph helpers, edits confined to Script/CkInput/, deviations = panel-width
  line wrapping only). Regate: scoped `Ck_AutoTest_Input` **21/21 green (31s)**. Uncommitted
  with the [P1A-D7] work. Awaiting maintainer: PIE re-drive of the panels; decision on the
  production `UnbindConflictAndRemap` fix (true unbind via MapPlayerKey+EKeys::Invalid) + the
  deferred pinning AutoTest.

### 2026-08-08 — [P1A-D7] gym redesign executed (post-commit polish, uncommitted on top)

- Single Opus unit LANDED: colored-lines API on the shared station display
  (`CkGym_Utils.as` +94 purely additive, `CkGymStation_EntityScript.as` +250: one
  `UTextRenderComponent` per same-color run, plain path falls through untouched, auto-size
  measures runs) + all 5 Input stations reworked to self-asserting colored panels + 8-step
  SM demo loop (`CkInputGym_RemapDemoSteps.as`, dwell-gated, `Ck_GymInput_Auto` toggle,
  manual exec = demo-hold) + persistence checklist w/ construct-time probe
  (`k_PersistMarkerKey = EKeys::Y`, grep-verified unused) + wart fix (4 report families
  render on their advertising panel). **Accepted agent judgment calls** (all within latitude,
  recorded in its report): tag-based station resolution (SM context resolves to the GymStation
  display entity via lifetime-owner context inheritance — matches known ck::Ctx behavior);
  batch step also moves Flashlight + step 6 clears Interact so Change Signal's 4/4 verdict is
  reachable; FString formatter family deleted (grep: zero remaining refs); glyph station has
  no "last device seen" line (`Get_ActiveControllerData` ensures on legitimate miss).
- **Orchestrator review found 1 defect, fixed inline:** colored path never cleared the plain
  description component, which still holds the spawn-payload text at the SAME anchor the
  colored runs stack from → overlapping text on every colored station. Fix in
  `Apply_ColouredBody`: retry-safe blank of the description component (async-instantiation
  guarded, `_DescriptionClearedForColour`), mirrors the file's existing null-retry pattern.
- Gate: scoped `Ck_AutoTest_Input` **21/21 green (35s)**. Full suite `1026/1023/3` — the 2
  known PathNetworkFollower names **plus 1 NEW red**:
  `Ck_AutoTest_CkJolt_KinematicPlatformCarriesDynamicBox`, 1/597 assertions ("box 165.9 vs
  platform 200.0" tracking tolerance). Root-caused as **load flake, not regression**: (a) no
  causal path — the diff is gym display script, never executed in the headless AutoTests
  world; (b) first full run today with the user's interactive editor open alongside (5/5
  earlier full runs green on this test); (c) discriminating experiment: 2/2 isolated re-runs
  GREEN (27s each, editor still open). Confidence: most-likely, not proven — strict proof
  would need a red repro on the pre-change tree under identical load. **Gate-of-record
  caveat:** if strict delta-zero is wanted before committing the polish, re-run the full
  suite with the user's editor closed. Uncommitted; commit only on user authorization.

### 2026-08-08 — Phase 1b executed (same session, after Phase 1 gate green)

- **`PHASE_1B.md` authored; rulings [P1B-D1]** (parametric bias v1: per-axis deadzone/exponent/
  sensitivity/invert, no curve assets) **+ [P1B-D2]** (raw rows never mutated — separate
  conditioned-state fragment; buttons untouched). Single Opus unit.
- **Unit LANDED** (`InputBias` quartet + 7 AutoTests + `FGroup_Input_Bias` group + one Claude.md
  subsection; no STOPs). **Spec correction the agent caught:** PHASE_1B said "processor after
  Route" — unimplementable, Route drains the inbox; correct chain `Collect → Bias → Route`
  (doc fixed; bonus: capture handlers read same-frame conditioned values). Stage order:
  inversion → deadzone (magnitude rescale, sign preserved) → exponent (magnitude) → sensitivity.
  **Accepted judgment calls:** `TryGet_AxisBias` invalid-key sentinel over `ExpandEnumAsExecs`
  (zero in-repo precedent for consuming AS `TOptional` — gate risk; reversible one-signature
  change, maintainer may prefer exec shape); `Get_LastRawAxisValue` added (only way to assert
  verbatim-raw from gameplay post-drain); `Add` keeps universal `FCk_Handle&` composition shape;
  duplicate ParamsData axis keys rejected atomically, runtime request is add-or-update.
  Orchestrator fixed the flagged Claude.md heading/count drift (two→three groups, chain updated).
  Wave gate (build + `Input` pattern + `--discover-fresh`, expect 71) in flight; then full suite
  vs `1019/1017/2` (expect `1026/1024/2`).

### 2026-08-08 — Phase 1 opened (same orchestrator session, after 1a gate green)

- **Why 1a's "stop for review" does not stall the session:** the maintainer's live directive
  authorized phase chaining while AFK; the 1a boundary record above is the review artifact, and
  commits remain withheld so the human checkpoint still happens — on files, not on a paused session.
- **Why 0A does not block Phase 1:** ruled [P1-D3] — PHASE_0.md's expected-observations table
  pre-writes a graceful response to every possible 0A outcome, so the spike refines docs/tuning
  (Phases 3-4), not Phase 1 structures. The Phase-1 thin writer + event dump becomes the 0A
  instrument.
- **`PHASE_1.md` authored** (orchestrator): 4 units, wave-gated; rulings [P1-D1] (observe-only
  index-0 preprocessor), [P1-D2] (minimal LocalPlayerSubsystem seam for O2, with the 1a
  `PlayerControllerChanged` timing lesson applied). Phase baseline: `1012/1010/2`.
- **Dispatched:** unit 1-1 (Opus) — `InputSource` quartet (raw-event inbox fragment per D25a/D21/
  D18, explicit device ownership per D22, `Request_InjectRawEvent` as both producer and
  synthetic-test surface, subsystem seam, named processor groups) + unit AutoTest. Orchestrator
  builds + gates on its return; 1-2 (layer stack + router) and 1-3 (Slate thin writer) follow.
- **Unit 1-1 LANDED** (9 new C++ files in CkInput + 1 AutoTest; zero existing files modified;
  Build.cs untouched — CkEcs dep already present). Accepted judgment calls: exclusive device
  ownership (different-source re-claim `Failed`, same-source idempotent `Succeeded`);
  `FGroup_Input_Collect` (after TimeDelta) + empty-but-registered `FGroup_Input_Route` (after
  Collect, **RunBefore Gameplay** — agent-added edge so raw input is visible same-frame; no
  cycle); ownership rows co-located in `Current`. Group mechanism verified expressible
  cross-module (`CkProcessorRegistration.h:85`, ghost nodes for empty groups,
  `CkProcessorGraph.cpp:336-388`); seam mimics `CkDialog_Subsystem` transient-owner entity.
  **Known first-pass risks (agent-flagged):** compile is INFERRED; `utils_input_source.as` exists
  only after the build's editor boot emits it; `EKeys::Gamepad_LeftX` AS availability assumed;
  `Get_ExpectedLogErrors` substring match. Wave gate (single-shot build + `Input` pattern,
  expect Total 8) in flight. Note: the subsystem adds one entity per local player in every PIE
  world — new suite-wide side effect, harmless by design, watch the full-suite gate.
- **Unit 1-1 gate ✅ GREEN, verified by name.** Build succeeded (adaptive-unity excluded the new
  .cpps cleanly); first pattern run was STALE-GREEN (57/57 but the new test absent — the
  discovery-cache trap caught by the Total check); `--discover-fresh` re-run:
  `Ck_AutoTest_InputSource_InjectAndOwnership` 1/1 PASSED (22s) — synthetic inject round-trip +
  atomic invalid-assignment rejection + completion-delegate contract all proven, and the
  generated `utils_input_source.as` exists.
- **Wave 2 dispatched (parallel, file-disjoint):** unit 1-2 `InputLayer` (layer quartet,
  declarative captures in one stable fragment, priority-collision rejection, deferred capture
  edits w/ one-frame contract, router in `FGroup_Input_Route` with press→release ownership +
  capture-triggered signal, D16 global actions, 6 AutoTests; may add ONLY friend lines to
  `CkInputSource_Fragment.h`) and unit 1-3 `InputSlate` (observe-only index-0 preprocessor per
  [P1-D1], [P1-D4] default routing, PIE focus gating, repeat-key drop, D21 provenance
  keyboard=SubFrameOrdered / gamepad=Simultaneous, console-toggleable `[CkInputDump]` 0A
  instrument, new files only). Orchestrator gates after both return.
- **Unit 1-3 LANDED** (4 new files, zero existing-file edits, comment audit clean). Handler→row
  mapping with device class derived from the KEY (gamepad face buttons via key handlers classify
  correctly); [P1-D4] routing with explicit-assignment override; focus gate =
  `HasAnyUserFocusOrFocusedDescendants` on the game viewport per game instance;
  `ck.Input.DumpRawEvents` CVar → `[CkInputDump]` lines incl. `Frame=` (load-bearing for 0A Q3,
  accepted). **Full 0A step list S1-S6 delivered** (stick sweep, UserIndex composite, near-
  simultaneous ordering at 3 framerates with gamepad control case, focus gates, repeat-drop,
  hot-plug) — wired for the human's spike session. **Gaps ruled, none blocking the wave gate:**
  (i) mouse-move look deltas NOT yet captured (`HandleMouseMoveEvent` — a real D18 acquisition
  gap) → named follow-up REQUIRED before Phase 1 exit; (ii) wheel/touch/motion out of scope,
  device-class enum extensible; (iii) single focus policy accepted, policy enum is a later design
  surface; (iv) inbox unbounded until 1-2's router drains — resolves this wave; (v) LP0 routing
  anchor flagged for split-screen review; (vi) no 1-3 AutoTest (granted — no in-repo precedent
  for faking Slate events); (vii) preprocessor deliberately non-exported (C4275). Compile is
  INFERRED until the wave gate.
- **Unit 1-2 LANDED** (`InputLayer` quartet + 6 AutoTests + the single permitted friend line in
  `CkInputSource_Fragment.h:33`; no STOPs). Captures = data rows in ONE stable fragment;
  enum-mode+value optionality; capture-edit requests drain in Collect (one-frame contract has its
  own test with an in-pass probe); router in Route group walks descending priority,
  first-Consume-wins, effect = `OnInputCaptureTriggered` signal; press→release ownership on the
  SOURCE's router-state fragment. **Accepted design points:** owned release bypasses the stack to
  its recorded owner (anything else strands a mid-hold consumer); `SetupRouterState` covers ALL
  sources so layer-less inboxes drain (closes 1-3 gap iv); global actions = reserved bottom layer
  at `TNumericLimits<int32>::Lowest()` (CkCamera sentinel precedent), that priority rejected for
  ordinary layers. Perf note recorded: per-event layer ranking is a filtered world view scan —
  fine at local-player counts, index it if layer counts grow. Wave gate (build + `Input` pattern
  + `--discover-fresh`, expect ~64 rows incl. 6 new `InputLayer` names) in flight.
- **Wave gate ✅ GREEN: 64/64**, all six `InputLayer` tests verified by name (masking,
  passthrough, global-action arbitration, release-pairing across layer pop, one-frame
  capture-edit contract, collision rejection). 1-2 + 1-3 compiled first-try.
- **Final Phase-1 dispatches:** [P1-D5] ruled — mouse-move look deltas captured as
  `MouseX`/`MouseY` `AnalogAxis` rows (delta verbatim, non-zero components only, same focus
  gate/routing/observe-only) — assigned back to the 1-3 agent (context intact) touching ONLY its
  preprocessor files + an S7 dump step; unit 1-4 (fresh agent) extends `CkInput/Claude.md` with
  the landed raw layer (extend-not-rewrite, no breadcrumbs, reads the possibly-concurrent
  mouse-move code last). After both: final build + full-suite phase gate vs `1012/1010/2`
  (expect `1019/1017/2` — 7 new tests this phase: 1 InputSource + 6 InputLayer).
- **[P1-D5] mouse-move capture LANDED** (preprocessor files only): `MouseX`/`MouseY` `AnalogAxis`
  rows from `GetCursorDelta()`, exact-zero skip (an epsilon would be the filtering D18 forbids),
  same gate/routing/observe-only; `MouseX/MouseY` → Mouse classification PROVEN
  (`InputCoreTypes.cpp:532-533`); S7 added to the 0A list. Perf note: first per-event path that
  scales with polling rate — trivial today, profile-flag if the raw layer ever shows up.
- **Unit 1-4 LANDED**: `CkInput/Claude.md` extended 186→462 lines (extend-not-rewrite; falsified
  claims fixed — "no ECS state" rescoped, stale CkEcs-unused line deleted; symbol-verified
  reference for all four surfaces + processor groups + arbitration + writer). **Follow-ups from
  its cross-check (recorded, not fixed):** (a) `Request_AddGlobalAction` fires completion with
  the SOURCE as owner on sync-reject but the LAYER on success (`CkInputLayer_Utils.cpp:298-344`)
  — API asymmetry, maintainer call; (b) LP0-source gate in `DoTryGet_RoutedSource`
  (`CkInputSlate_Preprocessor.cpp:252-256`) drops ALL events until LP0's source exists — under
  the [P1-D4] split-screen review; (c) the one-frame-contract comment at
  `CkInputLayer_Utils.h:186-187` overstates scope (pre-Collect enqueues land same frame) —
  comment polish. Phase 1 closing gate (build + FULL suite `--no-nullrhi --parallel 1`) in
  flight.

### 2026-08-08 — Phase 1a execution begins (orchestrator session, tiered dispatch)

- **Confirmed at session start:** superproject `e9aed7d`, CkFoundation `b982baf24`, CkTests
  `fd26b553` — SHAs unchanged from baseline capture; CkTests branch label moved to `dev` (ruled
  benign, [P1A-D1]). `PHASE_1A.md` rework (previous "Next action") was already complete on disk.
- **Ran:** second full-suite run (test-only, `--no-nullrhi --parallel 1`, no `--build` — zero code
  changed since the green baseline build). **Result: `1005/1003/2/0/0` in 10m16s, failing names
  identical** (`Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`,
  `..._ProjectsRibbonWaypointWithinNavQueryExtent`). The two failures are deterministic (2/2 runs);
  baseline double-confirmed. Toolbox exit was 1 as expected for a run with test failures; the
  `=== Test summary ===` block is the verdict, per standing rule.
- **Dispatched (all Opus, parallel, per continuation-prompt §6):**
  1. **1a-0** — AS input content + registration + verify AutoTest. Drafts to scratchpad ONLY
     ([P1A-D2]); orchestrator installs after review + after the flakiness run finishes.
  2. **O11** — CkRecord iteration-order research → `RESEARCH_O11_CkRecordOrdering.md`.
  3. **`CkInput/Claude.md` rewrite** — document the shipped surface only, no campaign breadcrumbs.
  4. **Poll-surface design dossier** → `DESIGN_PollSurface.md`, status PROPOSED — explicitly
     decides nothing; ruling reserved for orchestrator/maintainer.
- **Toolbox serialization plan:** orchestrator runs every gate itself; agents are forbidden to
  build/test. Next gates: scoped run for 1a-0's verify (`Get_AllRemappableKeys` non-empty) with
  `--discover-fresh`, then stations/AutoTests, then phase-end full suite diffed against
  `1005/1003/2` + new test names.
- **Standing user directives this session:** no commit, no push, no `.uasset` without asking.
- **`CkInput/Claude.md` rewrite LANDED and accepted** (orchestrator read the full rewritten doc):
  the old doc was fiction — it described entity-tied mapping-context lifetime and ECS cleanup that
  never existed; the module ships zero fragments/processors/handles. New doc documents the real
  three-surface module (context lifetime / rebinding / glyphs), the scan-is-a-feeder registration flow
  (real line: `CkKeyBinding_Subsystem.cpp:62`, campaign docs previously said `:58`), broadcast
  discipline, and 8 anti-patterns. **Load-bearing for 1a-0:** a mapping is remappable only if its
  InputAction carries `UPlayerMappableKeySettings` AND its IMC is registered with user settings;
  mutators return `bool` + `FGameplayTagContainer& OutFailureReason`.
- **12 suspected shipped-code defects in CkInput recorded** (from the doc-rewrite read; SUSPECTED —
  none repro'd yet; 1a-7 AutoTests should confirm the testable ones). Highest-relevance for the
  keybinding stations and the CkGameSettings page: (1) `UnbindFrom_MappingKeyChanged` removes ALL
  listeners matching (MappingName, Slot), not just the caller's (`CkKeyBinding_Subsystem.cpp:132-135`);
  (2) `BindTo_MappingKeyChanged` before a PC exists → first broadcast fires spurious
  `Invalid → Key` change (`:112-120`); (3) `SwapKeys` with an unbound source assigns `Invalid` to the
  other mapping instead of rejecting (`CkKeyBinding_Utils.cpp:474-517`); (4) `RemapKeys([])` reports
  success (`:296-312`); (5) `Get_ActiveControllerData` ensures on a legitimate device-mismatch miss
  (`CkKeyIcon_Utils.cpp:98`) — **AutoTest harness escalates ensures, so 1a-5/1a-7 must not call it
  speculatively**; (6) `CkKeyIcon_Utils.h:31` tooltip names nonexistent `UCk_InputActionWidget_UE`;
  (7) `FCk_Handle_KeybindListener` carries `HasNativeBreak/HasNativeMake` with no native make/break —
  un-makeable in BP; (8) zero calls to the module's own `ck::input` log functions — all failure
  paths silent; (9) unused `CkEcs` dep in Build.cs; (10) inconsistent null-profile validity policy;
  (11) style deviations (no `Private/` dir, `In` prefixes dropped in .cpps, missing enum formatter);
  (12) LOW-CONFIDENCE: `RemoveMappingContexts` uses `.Get()` so an unloaded soft ref silently skips.
  **Communicate 1-5 + 7 to the CkGameSettings owner** — their page consumes exactly these paths.
- **1a-0 draft LANDED, reviewed, INSTALLED** (`Plugins/CkTests/Script/CkInput/CkInput_Assets.as` +
  `CkAutoTest_Input_RemappableKeysRegistered.as`; scoped gate run in flight at time of writing).
  Content: 4 IA script-literals (`IA_CkTests_{Jump,Crouch,Interact,Flashlight}` →
  SpaceBar/C/E/F) each with instanced `UPlayerMappableKeySettings` (Name/DisplayName/Category),
  1 IMC; categories arranged so Jump/Crouch collide under both conflict scopes and Jump/Interact
  under `All` only (free negative case for 1a-3). Test asserts count == `k_MappableRowCount` (4)
  + per-key mapping names, then unregisters.
  **PHASE_1A's unverified hop RESOLVED with engine-header evidence:**
  `RegisterInputMappingContext` and `UnregisterInputMappingContext` are both `BlueprintCallable`
  (`EnhancedInputUserSettings.h:804-805`, `:812-813`) → AS-visible; **both fallbacks moot, Phase 1a
  stays zero-production-code.** Orchestrator independently verified harness API names against
  `Script/Common/CkAutoTest_Base.as` (`FinishFailure:549`, `Assert_True:612`,
  `Assert_Equals_Int:637`) and the AS `ck::IsValid`/`Is_NOT_Valid` UObject bind
  (`CkIsValid_Defaults.cpp:36` + `CkIsValid_AngelScript.h:61`).
  **Known residual risks (agent-enumerated, compile-settled):** (1) `MapKey` returns
  `FEnhancedActionKeyMapping&` — if the AS reflected binder rejects ref returns there is NO AS
  workaround (STOP → orchestrator); (2) unqualified `MapKey`/property writes against asset-block
  implicit `this` — mechanical fallback: qualify with the asset's own name.
  **Teardown is structurally partial (engine limit, recorded not improvised):**
  `UnregisterInputMappingContext` drops the context but its profile rows persist for the PIE
  session (`EnhancedInputUserSettings.cpp:1789-1792`); nothing reaches disk (no `SaveKeyBindings`),
  re-registration idempotent → count stable. `ResetAllToDefaults`+save becomes mandatory from
  1a-3/1a-4 onward.
  **Expected side effect at the gate:** the map populator auto-places the new wrapper actor and
  auto-saves `Content/AutoTests/AutoTests_CkTests_Level.umap` — a pipeline-owned modification of an
  existing tracked .umap, not a hand-created asset; flag to the user at commit time.
- **1a-0 gate, first attempt: AS pipeline fully GREEN, runtime failure diagnosed to root cause.**
  Scoped run (`--test-pattern "RemappableKeys" --discover-fresh`): the two risky AS hops both
  cleared (assets + test compiled; `MapKey` ref-return bound; unqualified asset-body calls
  resolved), wrapper generated, populator placed the actor, discovery found the test (Total 1).
  Test failed at `Get_InputUserSettings` → null. **Root cause CONFIRMED in engine source, not
  guessed:** `bEnableUserSettings` defaults false (`EnhancedInputDeveloperSettings.cpp:17`) and
  gates user-settings creation (`EnhancedInputSubsystems.cpp:47-49`); host `DefaultInput.ini`
  never set it. Fixed per [P1A-D4]; gate re-run in flight. (A first toolbox attempt before that
  exited instantly on a relative `--output` path — retried with absolute path, no side effects.)
- **1a-0 gate, second attempt: 5/6 assertions green; count failed `expected 4, got 8`.** All four
  authored names present. Ruled out with evidence: cross-session `.sav` persistence (no EI save
  file exists), device-identity mismatch (`DetermineHardwareDeviceForActionMapping` hardcodes
  `Invalid`, `EnhancedInputUserSettings.cpp:1786-1789`), foreign mappable content (gym IMCs carry
  no `PlayerMappableKeySettings`). **Leading hypothesis (fix applied, dump instrument added):** AS
  asset init bodies re-run on the AS recompile the autotest wrapper generator itself triggers, and
  `MapKey` APPENDS → the IMC doubles to 8 mappings → registration creates 4 names × 2 slots.
  Fix: `UnmapAll()` (BlueprintCallable, `InputMappingContext.h:242-243`) opens the asset body,
  making init idempotent; the count assertion now embeds a full row dump (name+slot) so a third
  failure is self-diagnosing. Gate re-run in flight.
- **1a-0 gate, third attempt: ✅ GREEN.** `Total 1 / Passed 1` — count exactly 4, all 6 assertions
  green. The `UnmapAll()` outcome confirms the duplicate-append hypothesis (asset init re-runs
  under wrapper-generator recompiles). **1a-0 verify criterion met: `Get_AllRemappableKeys`
  non-empty, count == authored rows.** Wave 2 dispatched: one Opus unit for 1a-1 scaffold +
  stations 1a-2/3/4/5/6 (single coherent gym file set), one Opus unit for the 1a-7 headless
  AutoTest battery (disjoint files). Orchestrator gates after BOTH return — no toolbox runs while
  either writes Script/.
- **Wave 2 LANDED (both units).** 1a-7: six AutoTests (`RemapRoundTrip`, `ConflictDetection_ScopeMatters`,
  `SwapSymmetry`, `ResetAllRestoresEveryDefault`, `ChangeSignalFiresOnRemap`, `SaveWritesUserSettings`),
  each registering at start, restoring defaults at end, asserting on OWN mapping names never counts
  (shared-world law); orchestrator spot-checked the conflict test — house-quality. Known-defect
  edges deliberately not pinned (recorded as "would-pin" list). Stations unit: 8 gym files +
  1 registry row (`Gym "Input Key Binding"`), five stations (inspection / remap+conflict /
  reset+persistence / key-icon / change-signal), 12 exec commands, armed/suspended teardown.
  **Rulings:** [P1A-D5] 1a-5 glyphs use per-tick re-resolution, no CommonUI event bind — strictly
  stronger than event-driven, zero unverified AS bindings (no in-tree precedent for
  `UCommonInputSubsystem` auto-accessor), matches module-doc don't-cache rule. [P1A-D6] stations
  are exec-driven, no `CkGym_StationSm` step graphs — ops are viewer-triggered discrete mutations,
  not timed sequences; maintainer may request SM-step polish later.
  **Honest unknowns carried to the gate:** whether entity `DoEndPlay` fires on gym exit (teardown
  has 3 paths incl. `Ck_GymInput_ResetAllAndSave` exec; if DoEndPlay is dead, teardown is
  manual-exec-only — check at [EDITOR-VERIFY]); `FCk_OnMappingKeyChanged` handler shape;
  `FSlateBrush.ResourceObject` read. All compile-settled by the scoped gate now running
  (`--test-pattern "Ck_AutoTest_Input" --discover-fresh`, also compiles the gym files).
  No construct-time reset backstop by design — it would erase the rebind that 1a-4's persistence
  check observes surviving; the arm/suspend exec pair is the "offer and default to reset" shape.
- **Campaign-authored dirty paths so far (for the commit conversation — nothing committed):**
  superproject: `Config/DefaultInput.ini` ([P1A-D4]) + campaign docs under CkFoundation
  `docs/campaigns/2026-08-07-CkIntent/` (+ rewritten `Source/CkInput/Claude.md`). CkTests:
  `?? Script/CkInput/` (1a-0 files + wave-2 output), `M Script/Generated/CkTests_AutoTestActors.as`
  (wrapper regen — pipeline artifact), and
  `A Content/__ExternalActors__/AutoTests/AutoTests_CkTests_Level/3/9D/7JL09P85VDGIALIQ1DT6SV.uasset`
  — the populator-placed wrapper actor, **auto-staged by the editor's source-control integration,
  not by the orchestrator**; a pipeline-owned external-actor file, the closest thing to a `.uasset`
  this phase produces — flag to user. Foreign (untouched): `Config/DefaultGameplayTags.ini`,
  root `CONTINUATION_PROMPT_CrowdDebugger...md`, `_scratch/`, CkTests pointer drift, CkUsf
  GeneratedLooks dirt in CkFoundation.
- **Scoped gate (7 Input tests): 6/7 green; the 1 red CONFIRMED a shipped defect — the
  change-signal surface was dead on arrival.** `ChangeSignalFiresOnRemap` failed with 0 delegate
  fires while its two `Get_DidMappingKeyChange` poll assertions PASSED — remap works, polling
  works, only the subsystem watcher dispatch is dead. **Root cause, engine-confirmed and
  deterministic (not a race):** the engine creates `UEnhancedInputUserSettings` only from
  `PlayerControllerChanged` (`EnhancedInputSubsystems.cpp:43-50`), which fires AFTER subsystem
  collection init — so `UCk_KeyBinding_Subsystem::Initialize`'s `GetUserSettings()` is ALWAYS null,
  its silent early-out skips the `OnSettingsChanged.AddDynamic`, and **`BindTo_OnMappingKeyChanged`
  has never worked anywhere; the `MappingContextScanPaths` registration in the same block is
  equally unreachable.** Upgraded from "suspected" to **CONFIRMED defect**; explains the module's
  zero-logging finding (nothing ever reported the early-out). CkGameSettings' page depends on this
  path — escalate to maintainer.
- **FIRST PRODUCTION-CODE CHANGE of the campaign (the PHASE_1A "fix only if trivial" branch,
  called out separately, NOT folded into gym work):** `CkKeyBinding_Subsystem.h/.cpp` — the bind +
  scan-path block moved to `DoBindToSettingsAndRegisterScanPaths()` (guarded by existing `_Bound`),
  invoked from `Initialize` (harmless, future-proof), a new `PlayerControllerChanged` override
  (mirrors the engine's own settings-creation point), and lazily from `BindTo_MappingKeyChanged`
  (backstop against same-collection ordering). This must be its own commit at ship time.
- **Fix VERIFIED: build succeeded, scoped gate 7/7 GREEN** (`ChangeSignalFiresOnRemap` now passes —
  the change-signal surface works for the first time).
- **PHASE GATE: ✅ GREEN, DELTA-ZERO.** Full suite `Total 1012 / Passed 1010 / Failed 2 /
  Skipped 0 / Contaminated 0`, 10m14s — exactly the prediction (baseline 1005 + 7 new, same two
  `PathNetworkFollower` failures, now deterministic 3/3 runs). `Saved/SaveGames/
  EnhancedInputUserSettings.sav` exists as the SANCTIONED defaults-content residue (PHASE_1A
  teardown law); behavioral proof of no leaked rebind: the full run loaded that save and every
  default-asserting Input test passed. Comment audit run over orchestrator-authored diffs
  (subsystem fix + AS instrument comments are why/contract comments; agent files were authored
  under the no-breadcrumb rule and spot-checks complied). `ck-change-control` formal checklist
  deferred to the ship step (commits withheld); its substance — compile + full-suite gate on the
  final artifact, defect call-out, cross-environment exercise (AS tests over the C++ fix) — is
  covered above.
- **Poll-surface dossier LANDED** (`DESIGN_PollSurface.md`, status PROPOSED — decides nothing;
  orchestrator will adversarially review before any ruling, which Phase 6 owns). Its headline
  claims, unreviewed: (i) shapes A and B are not alternatives — D15-revised already commits to
  per-frame delivery rows, so the live fork is only the read API; (ii) proposes a Shape C
  (layer-resolved intent handles) making bypass unrepresentable, conditional on two open questions;
  (iii) **the hole is wider than charges** — a tap/chord completing on the press frame under an
  empty delivery set is undefended by D15's defaults, and D15 leaves the matcher's behavior for
  non-accumulating constructs under masked delivery unstated (three readings α/β/γ laid out);
  (iv) consumption must be an IMMEDIATE mutator (house escape hatch), else the same-frame
  two-consumer case races — must be declared or someone will "fix" it into a deferred request;
  (v) "Consume" is overloaded across D14 (routing mask) vs D6 (gameplay claim) — naming hazard.
  Also flagged two stale rows in this file's Open-items table (O13, O9) — both fixed today.
- **O11 CLOSED** (same day, later): verdict `ORDERED-TODAY-BUT-INCIDENTAL`, full evidence in
  `RESEARCH_O11_CkRecordOrdering.md`; orchestrator re-verified the two load-bearing citations
  (`CkRecord_Utils.h:825` swap-prune; `CkSmState_Processor.cpp:123` order-reliant consumer comment)
  before accepting. Ruling recorded as [P1A-D3] — layer ordering uses explicit priority ints.
  **Adjacent CkRecord findings logged in that doc, NOT chased (not this campaign's):** suspected
  duplicate-append in `Get_ValidEntries`/`_If` for EntityExtension entries (`:378`+`:385` both
  recurse); apparently never-instantiated `Get_ValidEntries_ByTag` (`:420-451`) that would not
  compile if instantiated; stale `FProcessor_RecordOfEntities_Destructor` friend decl;
  `_DisconnectionFuncs` keyed by record entity rather than (entity, record type). Flag to the
  CkRecord owner; also note CkStateMachine's transition walk relies on record order holding only
  because SM transitions are never disconnected mid-life.

### 2026-08-08 — cold review; consolidation pass; Phase 1a sent back for rework

- **Ran:** a cold adversarial review of the whole doc set by an agent that authored none of it
  (deliberate — the prior reviewer would have been reviewing its own D13/N1–N5/phase plan).
- **Confirmed** (I re-verified each before acting):
  - **`PHASE_1A.md` is unexecutable as written.** No `IMC_*`/`IA_*` assets exist anywhere in the
    project and no `MappingContextScanPaths` is configured, and `UCk_KeyBinding_Subsystem` registers
    mappable keys **only** by asset-registry scan of those paths. `Get_AllRemappableKeys` returns
    empty; every station tests nothing. The missing scan-path config lives in the **host project's**
    `Config/DefaultGame.ini`, which also breaks the doc's "CkTests only" scope.
  - **7 of 14 function names in `PHASE_1A.md` were wrong.** Actual: `RemapKey`, `SwapKeys`,
    `ResetMappingToDefault`, `ResetAllToDefaults`, `SaveKeyBindings`, `BindTo_OnMappingKeyChanged`,
    `UnbindFrom_OnMappingKeyChanged`.
  - **D15 was broken.** Confirmed by construction, not opinion: hold/charge accumulates while
    consumed and detonates on layer pop, because D6 makes poll primary and a completed undelivered
    intent is state waiting for a poller. My replay-determinism justification was wrong — per-frame
    layer state is in the record anyway, so delivery-aware matching stays a pure function of it.
  - Eight doc contradictions, all introduced in the last day by folding D25 in too fast. The
    design-of-record still taught the callback model D25b deleted.
- **Decided:** D15 revised to fact-vs-policy (see decision log). D17 dead. D21 revised to per-device
  provenance. O13 closed by deferred requests rather than an ftxc-style snapshot guard.
- **Inferred (unconfirmed):** that deferred capture edits cost exactly one frame of transition
  latency (a modal pushed by frame N's Esc does not consume the rest of frame N). **Would confirm:**
  a Phase 1 test asserting the boundary.
- **Follow-ups recorded, not chased:** `CkInput/Claude.md` describes none of the module's current
  surface and must be rewritten in Phase 1; the biasing stage's params must be a runtime-mutable
  fragment, not settings-only; cross-module processor ordering (CkInput before CkIntent) needs
  explicit groups.

### 2026-08-08 — Phase 0 baseline captured; 0B/0D/0G/0H/0I answered

- **Ran:** `./CkAuto/UnrealToolbox.exe --build --target=Editor --test --no-nullrhi --parallel 1`
  (full suite, no pattern) → **build succeeded**; `Total 1005 / Passed 1003 / Failed 2 / Skipped 0 /
  Contaminated 0`, 12m14s. Toolbox exit 1 = test failures, not a build break (skill: "non-zero exit
  from `--test` is the normal way the toolbox reports test failures").
- **Confirmed:** the 2 failures are `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`
  and `..._ProjectsRibbonWaypointWithinNavQueryExtent`. This campaign has written **zero code** —
  markdown only — so these are pre-existing and belong to the path-network/crowd workstream whose
  state is in this tree.
- **Inferred (unconfirmed):** that both are deterministic rather than flaky. **Only one run was
  made**, so flakiness is untested. **Would confirm:** a second full run, or the owning workstream's
  own record. Do not treat "2 failing" as a stable number without that.
- **Confirmed** (research, evidence in `PHASE_0_RESEARCH.md`): N6 (CkUI already owns input
  suspension, and `IsInputSuspended()` is public `BlueprintPure` — so observing it needs no CkUI
  change); N7 (four `*Intent*` tag namespaces, not two); 0H (the `ReplayMissedTicks` branch is a bare
  unbounded `while` — a 1s hitch at `Hz(60)` runs 60 `DoTick`s in one frame); 0D (no local-player ↔
  entity mechanism exists; `UCk_LocalPlayer_UE` is an empty shell); 0B (two preprocessors both
  register at index 0, so registration order decides — a pre-existing latent defect).
- **Follow-ups recorded, not chased:** the index-0 preprocessor collision belongs to the debugger
  owner; `Bb.NpcIntent.*` arguably wants renaming to `Bb.NpcGoal.*` downstream — BusterBlock's call.

### 2026-08-07 — CkInteraction intent study → D13

- **Ran:** a study of `CkInteraction`'s pre-existing intent concept and its call sites across
  CkFoundation, CkTests, and the BusterBlock downstream repo.
- **Confirmed** (read directly):
  - The concept lives entirely in `InteractionResolver` — `Start/StopIntent` requests,
    `TSet<FGameplayTag> _ActiveIntents`, `_IntentChannelMappings`, `FTag_InteractionResolver_IntentUpdated`,
    and an `OnBestTargetsChanged` signal carrying the intent tag.
  - It is **producer-agnostic by design**: `BB_NpcAI_Combat_Feature.as:537-543` asserts
    `StartIntent`/`StopIntent` from AI with no input involved. This is the fact that settles the
    consolidation question — the resolver must not couple to an input system.
  - A downstream project already hand-built the bridge:
    `BB_Hfsm_Task_IntentToResolver.as` (7.5 KB) binds intent-ByteAttribute change signals and routes
    them to `Request_Start/StopIntent`. Its header states the contract verbatim: *"the input layer
    only writes the attribute and never pokes the resolver directly. Other intent sources (AI,
    scripted tutorials) reuse the same path."* That producer convention is exactly what D5 deprecates.
  - `UCk_Utils_InteractionResolver_UE::Add(..., ECk_Replication InReplicates = ECk_Replication::Replicates)`
    — `CkInteractionResolver_Utils.h:42`. The resolver replicates by default; CkIntent is client-local
    (D4). Recorded as O10 against the future bridge, not solved here.
  - The same task's unbind path documents a trap worth carrying forward: an intent left asserted makes
    the resolver swallow every subsequent `StartIntent` on that channel (`:120-140`).
- **Inferred (unconfirmed):** that a shared `Intent.*` namespace plus a later bridge module is
  cheaper than extracting shared identity infrastructure. **Would confirm:** the bridge module coming
  in at roughly the size of the downstream task it replaces, with no new shared types.
- **Decided:** D13 — independence preserved, no rename, shared tag namespace, bridge deferred.
  Rejected: renaming the resolver's surface (~15 files, three repos, cosmetic), and extracting a
  shared identity module (the only shared substance is an `FGameplayTag`; GameplayTags already *is*
  that mechanism — it fails the CkPoi v2 test, which extracted real data + behavior).
- **Follow-ups recorded, not chased:** O8 (tag namespace named for the deprecated carrier),
  O9 (continuous look/move axes), O10 (bridge authority conflict).

### 2026-08-07 — design reviewed twice, rewritten, campaign docs authored

- **Ran:** two adversarial review rounds (Fable) against the original `CkInput` PROMPT, plus
  independent verification of every load-bearing claim in the main session.
- **Confirmed** (each read directly, not taken from the subagent):
  - `Source/CkInput/` exists — 16 files incl. `CkKeyBinding_Utils.h` (361 lines), `CkKeyIcon_Utils.h`,
    `CkKeyBinding_Subsystem.h`, with AS bindings at `Script/Generated/utils_key_binding.as` and
    `utils_key_icon.as`. The original design never mentioned it and reused its module name.
  - `docs/specs/2026-08-05-CkGameSettings-design.md:205` — "Rebinding stays on CkInput /
    `UEnhancedInputUserSettings`"; `:139` settings page consumes `UCk_Utils_KeyBinding_UE`
    end-to-end; `:17` BusterBlock already migrated.
  - `Source/CkAttribute/CLAUDE.md:54` — same-tick request coalescing → one `OnValueChanged`
    carrying only the last value. `CkByteAttribute_Fragment_Data.h:55` — `ECk_MinMax::None` default,
    so clamping is *not* the problem.
  - `Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h` — `TickRate` trait,
    `ECk_ProcessorTickCatchUp::ReplayMissedTicks`, `_RemainingDeltaTFromLastFrame` carry. No
    max-catch-up clamp.
  - `Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment_Data.h:12-17` — verbatim: late binders under
    `FireIfPayloadInFlight*` receive only the LAST payload.
  - `CkLoadingScreen_Subsystem.cpp:81-89` — all preprocessor handlers return `Get_CanEatInput()`,
    registered at priority 0 (`:658`). Two further preprocessors ship in the debugger tree.
  - `CkDebuggerCommon/.../SCkDebug_EventTimeline.h` — shared `SLeafWidget` with `LaneLabels`,
    `FCkDebug_TimelineEvent`, `FCkDebug_TimelineSpan`, `OnEventSelected`; consumed by
    `CkGoapDebugger`. `CkSmDebugger` has a separate one.
  - Engine on disk is 5.7.4 — the original doc said 5.6; superproject CLAUDE.md says 5.5. Both stale.
- **Inferred (unconfirmed):**
  - That deferral from forward ambiguity only (D7) preserves the zero-latency guarantee at ~40 moves.
    Algorithmic reasoning, not measured. **Would confirm:** the Phase 4 resolution-table dump plus the
    Phase 2 zero-deferral AutoTest holding green after `Gym_Input_Fighting`'s move set is registered.
  - That a 120-frame ring is adequate (longest scanned construct ≈ 60–90f for 360/720 motions;
    charges are accumulator-based so cost nothing). **Would confirm:** Phase 5 scan-bound assertions.
- **Follow-ups recorded, not chased:**
  - Superproject `CLAUDE.md` still claims UE 5.5 — stale, not this campaign's to fix.
  - The original PROMPT lives only in session history + the review digest; it is superseded, not archived.

---

## Open items

| Item | Status | Next step |
|---|---|---|
| O1 — module name `CkIntent` (D1) | ✅ Closed 2026-08-07 | Confirmed by maintainer. |
| O7 — `CkInteraction` intent consolidation | ✅ Closed 2026-08-07 | Resolved as D13: independence kept, shared `Intent.*` tag namespace, bridge deferred to its own campaign. Added Phase 0 item 0I. |
| O14 — where does the **layer stack** live? | ✅ Closed 2026-08-08 | `CkInput`. Now D24. |
| O15 — does the rebinding/glyph surface stay in `CkInput`? | ✅ Closed 2026-08-08 | Yes. Plus a new requirement: `Gym_Input_KeyBinding` (D23), landing first. |
| O16 — keybinding + key-icon surface has **zero coverage** | ✅ Closed 2026-08-08 | Phase 1a landed 7 AutoTests + the 5-station `Gym_Input_KeyBinding` — and the coverage immediately caught a DOA change-signal surface (see 2026-08-08 entries). Gym runtime behavior still awaits `[EDITOR-VERIFY]`. |
| O11 — does `CkRecord` guarantee iteration order? | ✅ Closed 2026-08-08 | Verdict: `ORDERED-TODAY-BUT-INCIDENTAL` (`RESEARCH_O11_CkRecordOrdering.md`). Ruling [P1A-D3]: layer ordering uses option (b), explicit priority + registration-time collision detection. |
| O12 — do axes participate in layer captures? | ⏳ Open | A vehicle layer consuming "steer" is axis consumption. Recommend yes, else continuous input has no layering story. |
| O13 — layer entity destroyed mid-dispatch | ✅ Closed 2026-08-08 | Deferred capture-edit requests make the capture set immutable for a whole routing pass (`DESIGN_InputLayering.md` — struck CLOSED there; this row was stale). Destruction still gets the usual `CK_IGNORE_PENDING_KILL` treatment. |
| O8 — intent tag namespace still named for the deprecated carrier | ⏳ Open | Downstream tags are `ByteAttribute_Intent_*`; D5 removes that carrier. Decide in Phase 0 (0I) whether to propose a rename or tolerate the misnomer. |
| O9 — continuous look/move axes: intents or not? | ✅ Closed 2026-08-08 | Settled by D18: axes in scope for acquisition/sampling/record/debugger, OUT of the intent grammar. This row was stale — the decision log already carried it. |
| O10 — bridge authority conflict | ⏳ Deferred | Resolver `Add` defaults to `ECk_Replication::Replicates` (`CkInteractionResolver_Utils.h:42`); CkIntent is client-local (D4). Not this campaign's to solve — belongs to the bridge module's design. |
| O2 — local-player → entity association | ⏳ Open | Neither review round found existing machinery. Genuine Phase 0 research item. |
| O3 — possession change / pawn respawn semantics | ⏳ Open | Unaddressed by the original design. Decide during Phase 0 alongside O2. |
| O4 — Slate analog cadence, `UserIndex` fidelity, sub-frame arrival order | ⏳ Open | **Hardware spike in Phase 0.** Not verifiable from this repo; Phases 1–3 depend on the answers. |
| O5 — max-catch-up clamp trait on `TProcessorBase` | ⏳ Open | Phase 3 deliverable. Touches shared CkEcs base — needs its own review note. |
| O6 — CkGameSettings boundary sit-down | ⏳ Open | Confirm with that campaign's owner that CkIntent reading the resolved key map introduces no conflict. |

**Rule: no completion claim may be written anywhere in this file while any row here is unresolved.**
