# PROGRESS — saveload-v3-ergonomics

Living doc. The EXECUTOR updates this at every phase boundary and whenever reality diverges from a phase
doc. Planner: Fable session 2026-07-14 (design discussion with Adam + 3-agent terrain survey). Branch:
`feature/save-load-improvements` (CkFoundation submodule; CkTests changes ride its same-named branch).

Adam's constraints: **harness first; minimize builds (one per phase, 4 total); each phase independently
shippable; quick.** Nothing is pushed by the executor (Class-4 → Adam review).

## Baseline (recorded at Phase 1 entry, 2026-07-14 — executor filled)

- CkFoundation HEAD at entry: `993f6323c` (clean tree: Y). After the campaign-docs commit `2f45437fa`
  (docs-only, `git add -f`; binary unaffected) that is the current tip; the baseline gates below ran against
  the `993f6323c` binary (parity 5B build; no source changed since).
- CkTests HEAD at entry: `4d55ec34a` (clean tree: Y).
- Parity campaign Phase 5 committed: **Y** — parity STOPPED at end of Phase 5 per its PROGRESS.md (CkF `993f6323`,
  CkTests `4d55ec34`). Parity PHASE_6 is absorbed by this campaign's Phase 2; PHASE_7/VALIDATION stays parity's.
- **Ck.Snapshot: 35 pass / 0 fail** (pattern `Ck.Snapshot`, exit 0, 6m38s). No failing names. Full name list:
  scratchpad `baseline-snapshot-names.txt` + `CkAuto/logs/erg-baseline-snapshot.log`. (Planner expectation 30/30;
  parity Phase 5 added tests → 35, all green.)
- **Ck.Net: 90 pass / 0 fail** (pattern `Ck.Net`, exit 0, 15m30s). No failing names. Full name list:
  scratchpad `baseline-net-names.txt` + `CkAuto/logs/erg-baseline-net.log`. (Planner expectation 90/90 — matched.)
- Both suites all-Success → no pre-existing reds to attribute a regression to. **THE by-name delta baseline** for
  every later phase = these two name lists (delta-zero measured with the identical patterns).

## Phase status

| Phase | Status | Commit(s) | Gate result vs baseline | Notes |
|---|---|---|---|---|
| 1 — harness + cast sweep + Jump absolute | DONE (2026-07-14) | CkF `3439ac756` (cast) + `f780dcf2d` (Timer Jump); CkTests `6237fa5` (harness+conversion); docs `2f45437fa` (pkg) | build exit 0; **Ck.Snapshot 35/35 (delta-0 by name), Ck.Net 90/90 (delta-0 by name), Ck.Timer 21/21** (planner expected 0 — see D4) | Timer_MPReload converted, both cycles green. Deviations D1–D6. |
| 2 — rename bundle (absorbs parity PHASE_6) | DONE (2026-07-14) | CkF `a49969195` (rename bundle: 6A+slots+named+registrars, absorbs parity 6A) + `d0fd71e59` (6B T_Policy delete, absorbs parity 6B); CkTests `b29a8aa` (symbol-sync fix) | **build exit 0** (after a fix-rebuild — see [P2-D4]); **Ck.Snapshot 35/35 (delta-0 by name), Ck.Net 90/90 (delta-0 by name)** | parity 6A/6B rows annotated Y. All code gates 0 (6A / `.Apply=` / RegisterLazyTyped-outside-inl / policy-surface). Deviations P2-D1..D4. Site counts: 6A 229 hits/40 files; 24 registrars→29 named calls (NetOnly 4/SaveOnly 6/SharedApply 5/SplitApply 14); 6B ~84 aliased callers untouched. Docs → VALIDATION §5 [P2-D3]. |
| 3 — symmetry class (a), 7 features | DONE (2026-07-14) | CkF `3947af498` | build exit 0; **Ck.Net 90/90 (delta-0 by name — byte-identical-wire gate)**, **Ck.Snapshot 35/35 (delta-0)** | `TryProduce<T>` on `UCk_Utils_Net_UE`; 11 wire sites / 7 features consume it (Velocity ×2, Acceleration ×2, TagSet, MontagePlayer, GridOccupancy, Team ×2, Player ×2). All 7 Produce lambdas verified == old wire builders; write-then-replicate order verified for all seed sites. Exit grep 0. Deviation P3-D1. |
| 4 — symmetry class (b), fold | DONE (2026-07-14) | CkF `96a224ab2` | build exit 0; **Ck.Net 90/90 (delta-0 by name — byte-parity oracle)**, **Ck.Snapshot 35/35 (delta-0)** | 4 files: attribute Replicate template (5 kinds) folds child-keyed TryProduce; EntityCollection + Inventory Spatial/DataOnly full-replace from TryProduce. Prescribed bodies. Attribute array-order concern did NOT materialize (Net clean). Exit grep 0. Multi-inventory clobber gap preserved (fence). No deviations. |
| 5 — CkEcs/Persistence header split | DONE (2026-07-14) | CkF `2753e0053` | **build exit 0 FIRST TRY**; Ck.Snapshot 35/35 (delta-0), Ck.Net 90/90 (delta-0); grep gate 0 moved decls under Net/ | 6 new `CkEcs/Persistence/` files; wire plumbing + net dispatcher STAY in Net/; old container `.inl.h` deleted (git R085 rename→registry.inl.h). **RunAfter seam: FORWARD-DECL WORKED** (TDepList accepts the incomplete type — no Persistence→Net include). `PendingApplyTimeoutSeconds` in the shared hydration-processor header (unity-safe). 20 registrars swapped; 4 standalone save-only registrars now 0 Net/. CkTests untouched. No deviations. |
| VALIDATION | DONE (2026-07-14) | CkF `a616427d6` (P5 docs) + `59bc41826` (§5 doc-debt) | **grep checklist all-phases PASS** (6A/`.Apply=`/RegisterLazyTyped-outside-registry/policy-surface/`Data.Attributes.Emplace(ToReplicate)`/`ReplicatedFragmentContainer.inl` all 0; EnqueueRoundTrip 5≥2). No re-build: final CODE commit is `2753e0053` (P5) — `a616427d6`+`59bc41826`+doc-debt are *.md only, don't touch the binary, so the P5 gate (Snapshot 35/35, Net 90/90) IS the final acceptance (no post-build code edit → no stale-green). | §5 doc debt cleared: CkSnapshot/CkEcs/Source `CLAUDE.md` + CkInventory refreshed (6A prose rename, `Apply/Remove`→`NetApply/NetRemove`, named `Register_*` recipe, `CkEcs/Persistence/` relocation). §6 handoff below. |

## Blockers (STOP-and-record; do not improvise past these)

_(none yet)_

Format per entry: date / phase+step / what was expected / what was observed (verbatim error or grep
output) / what you did NOT do / question for the planner or Adam.

## Deviations from plan (executed differently than written, with reason)

- **D1 (Phase 1 / Timer conversion) — dropped old Stage 3b, folded old Stage 8.** The harness shape
  (Spawn/SubjectReady/Mutate/ReloadSettled/Assert) has no pre-save assert slot, so the old Stage 3b pre-save
  sanity ("server holds elapsed 7 pre-save") is dropped; its sole failure mode (the deferred Jump never took)
  surfaces identically in the post-reload Assert ("elapsed round-tripped (7, not Construct default 10)" reads 10).
  Old Stage 8 ("no re-fire after +60 ticks") is folded into `NumCycles=2` — re-running the whole round-trip and
  re-asserting is a structurally STRONGER stability check. Coverage-neutral; the 7 Stage-7 parity checks are
  preserved verbatim in Assert.
- **D2 (Phase 1 / commit split) — CkTimer_Fragment.cpp's cast change rides commit 3, not commit 2.** The plan's
  commit 2 ("cast") and commit 3 ("Jump") both touch CkTimer_Fragment.cpp, but its Jump-hydration change depends
  on Fragment_Data.h's new `_JumpMode` field, so a commit 2 containing Fragment.cpp would not compile alone.
  Split for atomicity: commit 2 = AnimPlan+attribute cast; commit 3 = Timer Jump + Timer's own cast. Message on
  commit 2 says "AnimPlan/attribute", not "Timer/AnimPlan/attribute".
- **D3 (Phase 1 / Step 4.2) — fixed a CountUp-absolute Done no-op the plan's prescribed code introduced.** PHASE_1
  Step 4.2 prescribes `TimerChrono.Tick(FCk_Time{DeltaToApply})` for the CountUp branch. `FCk_Chrono::Tick`
  (CkChrono.cpp:34) early-returns when `Get_IsDone()` (`_CurrentValue >= _GoalValue`) WITHOUT mutating, so a
  BACKWARD absolute jump on an already-Done CountUp chrono silently no-ops (stays at GoalValue instead of Target).
  Found by the pre-build adversarial review (subagent workflow), independently CONFIRMED against the chrono source.
  NOT reachable by the save/load caller (HydrationApply jumps a CountUp chrono from its post-Setup baseline of 0,
  never Done unless GoalValue==0 where the delta is 0) — so the Timer parity test is unaffected and stays green.
  But `_JumpMode` is a new public BlueprintReadWrite field, and a silent-wrong-result on a valid public call
  violates the no-silent-failure non-negotiable. Fix: CountUp absolute now `Reset()` then `Tick(target)`, landing
  from any prior position. CountDown is unaffected (Consume has no Done early-out).
- **D4 (Phase 1 / Step 5.4) — `Ck.Timer` matched 21 tests, not 0.** The planner expected zero tests for the
  `Ck.Timer` pattern; the substring tokens `Ck`+`Timer` actually match 21 Timer-related tests. All 21 pass (exit 0,
  2m11s) — extra green Timer coverage, no concern.
- **D5 (Phase 1 / harness) — added an explicit `CkEcs/Registry/CkRegistry.h` include to the harness .cpp.** The
  pre-build review flagged that `FCk_Registry` completeness (for `Get_RegistryHandle()`/`ck::MakeHandle`) was only
  transitive via `CkEcsWorld_Subsystem.h`. Added the explicit include for IWYU robustness (matches the proven
  `CkEntityScript_Utils.cpp` mirror). Non-behavioral.
- **[P2-D1] (Phase 2 / commit structure) — 2 commits, not 3.** Phase-doc commits 1 (vocab+slots) and 2 (named
  entry points + registrars) both touch the registry core (`.h`/`.inl.h`) AND all registrar files, so a clean
  1/2 split needs hunk-level staging (interactive `git add -p`, unavailable). Combined into ONE "rename bundle"
  commit; 6B is the second commit. Each rename step is internally self-consistent.
- **[P2-D2] (Phase 2 / 6B) — kept `_ROUNDTRIP`/`_TRANSIENT` macro family as policy-less ALIASES rather than
  collapsing to a single plain form + migrating ~84 callers.** PHASE_6 §6B says "the plain
  CK_DEFINE_ENTITY_HOLDER / CK_DEFINE_RECORD_OF_ENTITIES macros are the only form" (planner expected ~85 sites).
  Reality: the ~85 sites are ~84 CALLERS of `_ROUNDTRIP`/`_TRANSIENT`/`_AND_UTILS_ROUNDTRIP`/`_TRANSIENT` across
  ~44 feature files (my step-1 grep `FSnapshotPolicy_|TSnapshotMarker|_WITH_POLICY` undercounted at 22 — it
  missed the suffix-macro callers). Two behavior-identical ways to reach the goal (delete the inert T_Policy
  surface): (A) migrate all ~84 callers to plain `CK_DEFINE_*` and delete the suffix macros — wide churn on the
  unpushed stack; (B) drop T_Policy from the templates + macro DEFINITIONS and keep the suffix macros as
  policy-less aliases — ZERO caller churn. **Chose (B)** — parity PHASE_6 rule 3 explicitly kills wide unpushed
  churn ("56 include lines / 47 files"), and (B) is far lower-risk. Deleted: `CkSnapshot_Policy.h`, `TSnapshotMarker`,
  `FSnapshotPolicy_*`, the `_WITH_POLICY` macros, the `T_Policy` template param, and the policy arg on the 7 direct
  template uses. Gate: `FSnapshotPolicy_|TSnapshotMarker|_WITH_POLICY` = 0. **For Adam:** if you want the full
  collapse (plain-only, per the literal phase-doc), it's a mechanical suffix-drop perl over the ~84 callers — say
  the word. Behavior is identical either way.
- **[P2-D3] (Phase 2 / docs) — doc updates deferred to VALIDATION §5.** The Phase 2 rename GATES are code-scoped
  (their purpose: verify code renames; all 0). `CkSnapshot/Claude.md`, `Source/CLAUDE.md`, `CkEcs/Claude.md` still
  carry old vocab (`FCk_ReplicatedFragmentHandlerRegistry`, `.Apply`/`.Remove` slot names, `RegisterLazyTyped`
  recipe) — updated at VALIDATION §5 (its intended "final session" doc pass), where the Register_* recipe rewrite
  is done properly. If the campaign stops after Phase 2, this doc pass is the one outstanding item.
- **[P3-D1] (Phase 3 / Step 2) — also converted the Velocity + Acceleration Add-time SEED sites.** PHASE_3
  Step 2 named only the Velocity/Acceleration Replicate PROCESSOR sites, but the Step-4 exit grep demands 0
  `FCk_RepData_Velocity{`/`FCk_RepData_Acceleration{` outside `*_Fragment.cpp`, and the Add-time seeds
  (`CkVelocity_Utils.cpp:43`, `CkAcceleration_Utils.cpp:29`) also construct those payloads. Team/Player convert
  BOTH their seed + update sites, so for symmetry + to satisfy the exit grep I converted the Velocity/Acceleration
  seeds too. Order verified: `FFragment_Velocity_Current`/`_Acceleration_Current` are composed (Velocity_Utils:26,
  Acceleration_Utils:24) BEFORE the seed, so `TryProduce` reads the starting value — byte-identical to the old
  `FCk_RepData_X{InParams.Get_StartingX()}`. Exit grep now 0. Not a Blocker (routine completeness call).
- **[P2-D4] (Phase 2 / build) — first build failed; one missed cross-repo reference, fix-rebuilt.** The 6A/slot
  rename swept `Plugins/CkFoundation/Source` only; one CkTests file
  (`Test_Snapshot_DynamicFragment_HandleRemap_RoundTrip.cpp`) references the framework types
  `FCk_ReplicatedFragmentHandlerRegistry::Find` + `ECk_RepFragment_ApplyResult` and failed to compile. CkFoundation
  compiled clean (errors only in that one CkTests file); renamed the 2 symbols there, rebuilt (44s incremental) →
  exit 0. This means Phase 2 used TWO builds (fail + fix), not one — unavoidable after a compile error. BB Source
  had 0 stale refs (checked). Lesson for later phases: the 6A-style framework renames must sweep CkTests too, not
  just CkFoundation Source.
- **D6 (Phase 1 / skills) — `ck-tests-authoring-and-running` was NOT in this session's Skill registry.** PROMPT
  names it for session start; only `ck-change-control` was loadable. Relied on exemplar-mimicry per PHASE_1
  (M2b / TransformParity / TimerParity spec files + CkNetAutomation_Common) as the phase doc mandates anyway.

### Behavior notes (not deviations, flagged for review)
- **Spurious `OnTimerJump(amount 0)` on a same-position restore.** Timer HydrationApply now issues an
  UNCONDITIONAL absolute `Request_Jump` (the old `IsNearlyZero` guard is gone — that guard's job moved INTO the
  handler as the single source of truth). When saved elapsed == post-Setup baseline (CountUp saved at 0), the
  handler applies a 0-delta jump and broadcasts `OnTimerJump(0)` + `OnTimerUpdate` — signals the old guarded path
  suppressed. Never broadcasts `OnTimerDone` (the no-re-fire guarantee holds). Benign for the load path; only
  matters if game code binds those signals in BeginPlay and reacts to load-time hydration. The Timer test uses
  non-zero deltas so it never hits the 0 case.

## The one claim most likely to be wrong (Phase 1)

The CountUp-absolute Done fix (D3): I claim `Reset()` then `Tick(target)` on a CountUp chrono has no side effect
beyond setting elapsed to `target` (Reset only writes `_CurrentValue=0`, verified CkChrono.cpp:79). If a future
CountUp timer relied on Reset clearing some other state, or on monotonic elapsed, that assumption could bite — but
no such coupling exists in the chrono today. Verified against source; the gate (Snapshot 35/35 incl. Timer_MPReload
two-cycle) exercises the CountUp save/load path and stays green.

## §6 — Handoff summary (for Adam)

**Campaign COMPLETE. All 5 phases + VALIDATION done, committed, GREEN. NOTHING PUSHED — Class-4 (framework-invariant:
CkEcs `Net/`+`Persistence/`, replication + save/load paths) review is mandatory before push.**

### Final tree state
- **CkFoundation** — tip `59bc41826`, tree CLEAN, **107 ahead of `origin/dev`**. This session's ergonomics commits
  are the top 14 (from `2f45437fa` campaign-package down to `59bc41826`); everything below `993f6323c` is the prior
  parity/rebuild stack (also unpushed, not this campaign).
- **CkTests** — tip `b29a8aa`, tree CLEAN, **23 ahead of `origin/dev`**. This session's ergonomics commits: `6237fa5`
  (round-trip harness `ck::auto_test::snapshot` + Timer_MPReload conversion) and `b29a8aa` (Phase-2 symbol-rename
  follow in the DynamicFragment handle-remap test, [P2-D4]). The rest are the parity stack.
- **Push order when approved:** CkFoundation FIRST, then CkTests (CkTests `b29a8aa` compiles against the CkEcs
  persistence vocab that only exists at CkF `a49969195`+). Never push/merge CkTests ahead of CkFoundation.

### This session's commits (ergonomics campaign only)
CkFoundation (14): `2f45437fa` pkg · `3439ac756` P1 cast · `f780dcf2d` P1 Timer-Jump-absolute · `4c3213f60` P1 docs ·
`a49969195` P2 rename-bundle(6A+slots+named+registrars) · `d0fd71e59` P2 6B T_Policy-delete · `93095ff35` P2 docs ·
`3947af498` P3 class-(a) Produce-symmetry · `e94008e03` P3 docs · `96a224ab2` P4 class-(b) Produce-fold ·
`4ca8fec49` P4 docs · `2753e0053` P5 CkEcs/Persistence header-split · `a616427d6` P5 docs · `59bc41826` §5 doc-debt.
CkTests (2): `6237fa5` P1 harness · `b29a8aa` P2 rename-follow.

### Gate results (all delta-0 by name vs Phase-1 baseline: Ck.Snapshot 35/35, Ck.Net 90/90)
Every phase: build exit 0 + Ck.Snapshot 35/35 + Ck.Net 90/90, all delta-0 by name. **One build per phase** (per
instruction). VALIDATION added no build: the final CODE commit is P5 `2753e0053`; the three commits after it are
*.md only, so the P5 binary is the final binary — the P5 gate IS the acceptance gate (no post-build code edit, so
no stale-green risk). Full VALIDATION grep checklist (all phases) passed — see the VALIDATION row.

### Three-environment status
- **C++** — verified: full Snapshot + Net suites green on the final binary.
- **AngelScript** — boot-compile verified transitively (the toolbox gate boots the editor and compiles AS for the
  PIE autotests; a broken binding would fail boot and error the gate — it didn't). The only AS-facing surface change
  is `FCk_Request_Timer_Jump::_JumpMode` (reflected `ECk_RelativeAbsolute` + `CK_PROPERTY` setter). Its CODE PATH is
  exercised by the C++ round-trip harness, but a `.as` runtime call of `Set_JumpMode` is NOT exercised (no AS test
  authored). Everything else this campaign touched is internal C++ (persistence machinery, `TryProduce<>` template)
  — not AS-callable.
- **Blueprint** — `[EDITOR-VERIFY]` (human): the `Make FCk_Request_Timer_Jump` node should show a `_JumpMode`
  enum dropdown. No other BP-facing surface changed.

### Deviations / blockers
No blockers fired. Deviations recorded above: D1–D6 (P1), P2-D1..D4 (P2), P3-D1 (P3), none (P4/P5). Load-bearing
ones for review: **[P2-D2]** 6B left ~84 aliased T_Policy callers untouched (inert surface, Option-A retirement
deferred — matches the parity campaign's standing [F3-D2]); **[P2-D4]** the framework rename had to sweep CkTests
too (missed on first build → fix-rebuild); **[P1-D3]** the CountUp-absolute Jump-on-Done logic-bug fix (see below).

### The one claim most likely to be wrong (campaign-wide)
**Phase 4's attribute-array-order assumption.** The class-(b) fold rebuilds the attribute owner container by folding
each attribute's child-keyed `TryProduce<T>` payload; I claim this reproduces the old wire builder byte-for-byte and
the Ck.Net byte-parity oracle stayed green (delta-0). If some attribute family emits components in a
registration-order that the fold doesn't preserve under a configuration the Net suite doesn't cover (e.g. a
different attribute count/order than the test fixtures), a client could see a reordered container. The gate says
clean, but the fixtures are the limit of that evidence. Secondary: the P1 CountUp-absolute chrono fix (§ above) and
the benign `OnTimerJump(0)` on same-position restore (behavior note above).

### What's left for you
1. Review the Class-4 surface (CkEcs `Net/` + new `Persistence/`, replication + save/load handler contract).
2. `[EDITOR-VERIFY]` the Timer Jump `_JumpMode` BP node + (optional) an AS `Set_JumpMode` runtime call.
3. Decide the deferred **[P2-D2]/[F3-D2]** Option-A T_Policy retirement (still inert).
4. Push CkFoundation → then CkTests, when satisfied.
