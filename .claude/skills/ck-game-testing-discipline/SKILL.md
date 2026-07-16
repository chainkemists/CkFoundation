---
name: ck-game-testing-discipline
description: 'Use when choosing coverage and test layers for CkFoundation gameplay, structuring AutoTests, preventing shared-world contamination, or defining proof of no regressions.'
---

# ck-game-testing-discipline

## Overview

This skill is the **consumer testing policy** for a game built on CkFoundation: what every
feature must cover, which test layer to pick, how a new game wires the test surfaces in,
the contamination traps unique to a shared suite world, and what counts as evidence.

The **mechanics** of authoring and running every layer — base-class API, wrapper generation,
populator pipeline, net stubs, gym registration, Gauntlet bridge, exact command lines — are
owned by CkTests' `ck-tests-authoring-and-running` skill and the three spec files in
`Plugins/CkTests/Script/Common/`. This skill cites them constantly and restates nothing.

Vocabulary (defined once): an **AutoTest** is an AngelScript entity script subclassing
`UCk_AutoTest_Base` that runs headless assertions inside one shared PIE world; a **gym** is
an interactive station level for eyeballing/tuning a feature (no assertions); a **Gauntlet
test** boots a real game process and asserts on log breadcrumbs; a **driver** is the
world-owning controller entity that discovers and routes to feature entities (see
ck-game-driver-architecture).

Evidence base: the BusterBlock corpus at superproject `52a75e13d` — 216 AutoTest `.as` files
across 66 feature dirs, ~80 gym files across 23 features, 22 registered Gauntlet tests,
zero C++ unit tests (counted 2026-07-03). Corpus examples are labeled; everything else is
generic to any `<Game>` on the framework.

## When NOT to use this skill

| If you need… | Load instead |
|---|---|
| Authoring/running mechanics for any layer (file template, populator, Session Frontend, net stubs, gym exec commands, Gauntlet bridge/exit codes, exact command lines) | `ck-tests-authoring-and-running` + the spec chain in `Plugins/CkTests/Script/Common/` |
| Diagnosing a red or flaky test | ck-game-debugging-playbook |
| Writing the feature the test covers | ck-game-feature-recipe |
| The replicated-spawn ownership RULE and rationale | ck-game-replication-patterns (this skill owns only the test-side mechanics) |
| Cook/stage/package flow (staging test AS, NeverCook, DisablePlugins mechanics) | ck-game-build-and-cook |
| Creating the `<Game>Tests` plugin skeleton from an empty repo | ck-game-project-bootstrap |
| Testing changes to the Ck plugins themselves | `ck-change-control` (framework skill) |

---

## 1. The coverage norm: feature and tests, same commit

**Every behavioral feature lands with its AutoTests in the same commit as the behavior they
cover.** Feature-with-test — not test-first, not test-after. `[PROMOTED FROM CORPUS
2026-07-03]` — this is uniform practice across the corpus, verified in the commit log:

- `01300d524` `feat(employee): spawn EmployeeManager on StoreDriver + daily-wage debit (routed) + AutoTest` — integration code + a new 174-line end-to-end AutoTest, one commit.
- `9a6bfb897` `feat(employee): DayCycle shift reconcile -> OnShift/OffShift presence + AutoTest` — behavior file + 118-line AutoTest, one commit.
- `0ddfada98`, `c27dfc88a` — same pattern at the unit and compose tiers (below).

(All four verified via `git show --stat` 2026-07-03.)

### 1.1 The three-slice coverage shape

A feature built along the standard arc (ck-game-feature-recipe) accumulates **three kinds of
AutoTest**, each landing with the slice it covers. Corpus example (BusterBlock Employee):

| Slice | Covers | Lands with | Corpus exhibit |
|---|---|---|---|
| **Unit-style AutoTest** | the pure-logic namespace (no world state beyond the harness) | the logic file | `0ddfada98`: `BB_Employee_Schedule.as` (26 ln, pure fns) + `BB_AutoTest_Employee_ScheduleWindows.as`, same commit |
| **Compose AutoTest** | `utils_<feature>::Add` produces the right entity shape — children present, fragments present/absent, tags applied | the Utils file | `c27dfc88a`: `BB_Employee_Utils.as` + `BB_AutoTest_Employee_ComposeFromRoster.as`, same commit |
| **End-to-end routed AutoTest** | the feature reached THROUGH the driver/owner path a player would exercise | the driver-integration commit | `01300d524`: wage debit routed via the StoreDriver + `BB_AutoTest_Employee_WageDeductsViaStoreDriver.as`, same commit |

Not every feature earns all three — a small self-contained feature may collapse to one
compose-plus-behavior test (corpus: Entryway shipped with a single 133-line
`DirectionalSignals` AutoTest). The norm is: **each behavioral claim the commit message makes
has an assertion somewhere in the same commit.**

### 1.2 What is exempt — scope policy, not a laxity license

Pure cosmetic/display glue is the corpus's deliberately untested tail: widgets that only
mirror state, splash text, music selection, signage, interact-prompt display. The policy:
if the code's only failure mode is "looks wrong" — no state mutation, no signal another
system consumes, no economy/inventory effect — a gym or manual PIE check is the verification
tier, and an AutoTest is not required. The moment display glue grows a behavioral edge
(a cooldown, a queue, a purchase side effect), that edge gets an AutoTest like anything else.

Two hard rules regardless of tier:

- **Never author tests in Blueprint.** BPs are binary — undiffable, unreviewable. Anything a
  BP subclass could add is an AS subclass with `default` overrides (`ck-tests-authoring-and-running`
  §1, AutoTest spec §11).
- **Scenario names state what is VERIFIED**, not what the code does: `MoneyFloor`,
  `NoPickupBeforeTime`, `MultiOccupant` — never `Test1`, `Demo` (AutoTest spec §4).

---

## 2. Layer choice — consumer decision table

Decide by what the assertion needs, not by habit. Mechanics for each row: the cited section
of `ck-tests-authoring-and-running`.

| Layer | Use when the assertion needs… | Cost | Mechanics |
|---|---|---|---|
| **AS AutoTest** (PIE) | a ticking ECS world: processors, deferred requests, signals, entity lifecycles. **~95% of feature logic lands here.** | one PIE boot amortized across the whole map | §2a |
| **Net AS AutoTest** (multi-PIE) | client–server: does the value/state actually replicate | multi-world PIE; **adding a test requires a C++ rebuild** (stub generation — the least discoverable fact in the pipeline, §2b.4); hard 30 s convergence ceiling regardless of `_TimeoutSeconds` (§2b.7) | §2b |
| **Gym** | a human's eyes — interactive demo, feel/tuning, manual QA. Built for interactive/visual features; pure-logic features get AutoTests only (corpus: Trashcan has 3 AutoTests, no gym; Shelf has a 10-file gym) | manual PIE session | §2d |
| **Gauntlet** | a real process: actual boot, real GameMode/input pipeline/navmesh, exit-code contract — player journeys an in-PIE test can't exercise | **a fresh editor boot per run** — corpus measured ~50–65 s/run, suite ~9–10 min; budget accordingly and don't reach for it when a ticking world suffices | §2e |
| **C++ automation test** | no world at all — pure math, parsers, formatting, data shapes | milliseconds | §2c |

On the last row: the corpus has **zero** C++ unit tests in practice — its "pure utility"
code lives in AS namespaces and gets unit-style AutoTests instead (§1.1). If you do write
one, pretty-naming is `[UNDER ADJUDICATION — see CkFoundation .claude/reports/ADJUDICATIONS.md
A2]`: join the feature family's existing prefix; greenfield consumer analogue is
`<Game>.<Feature>.*`.

---

## 3. Project wiring — what a NEW game must set up

Generalized from BusterBlock's wiring (verified 2026-07-03); the bootstrap skill owns the
empty-repo procedure, ck-game-build-and-cook owns the cook/stage half. This section owns
the test-specific wiring decisions.

### 3.1 A `<Game>Tests` editor-only plugin — non-negotiable

Host ALL test AngelScript (`Script/{Tests,Gyms,Gauntlet,Generated}`) plus one C++ module
(for generated net-autotest stubs) in a dedicated plugin:

- `<Game>Tests.uplugin`: `"EnabledByDefault": false`, depends on CkTests.
- Game `.uproject`: enable it with `"TargetAllowList": ["Editor"]`.

**Why this is load-bearing** — the Shipping-staging incident (BusterBlock `e0de34899`,
verified): packaged Shipping compiles the project's staged `Script/` at boot, but test/gym
scripts inherit CkTests base classes, and CkTests is disabled in Shipping. Result: **431
"unknown super type" errors aborted AS preprocessing and the client never booted.** The
first fix was a `Script/Dev/` convention; the final home is the editor-only plugin
(`68616a8a5` moved 683 files there), which removes test AS from every packaged build by
construction. Pair it with `DisablePlugins.Add("CkTests")` in Shipping/Test target rules
and `DirectoriesToNeverCook` for gym/autotest maps — mechanics in ck-game-build-and-cook.

### 3.2 The AutoTest map config

One AS-defined `UCkAutoTestMapConfig` asset per test root, pointing the populator at your
AutoTests level. Full setup: AutoTest spec §9 (9a level, 9c config, 9d populator behavior).
The consumer-side discipline: **set `ClassScanRoot` to your plugin's scope explicitly** —
the populator auto-saves the map on sync, and an over-broad scan root can wipe or repopulate
the wrong map. Corpus example (BusterBlock):
`Plugins/BusterBlockTests/Script/Tests/BB_AutoTestMapConfig.as` sets
`ClassScanRoot = "/BusterBlockTests/"` with a comment saying exactly this.

### 3.3 Gym registry and Gauntlet dispatcher

- Gyms: a `<Game>` gym registry namespace + a game base GameMode that registers project gyms;
  templates and console commands in the gym spec §2/§5/§9 (real command set:
  `Ck_Gym_Restart/Next/Prev/GoTo/List` — the spec's `ShowInfo`/`ValidateStations` rows are
  stale, per `Plugins/CkTests/CLAUDE.md`'s trust table).
- Gauntlet: every project owns a **dispatcher** mapping scenario names to AS classes —
  the dispatcher contract is Gauntlet spec §7. Corpus example (BusterBlock): a repo-root
  `RunGauntlet.bat` registry + `RunGauntletAll.ps1` suite/flake harness that parses the .bat
  as its single source of truth, keeps expected-failure tests registered but excluded from
  the gate, and flags "unexpectedly passing" for status flips. `[SINGLE-EXEMPLAR]` as a
  concrete script pair, but the dispatcher-per-project contract itself is spec-mandated.

---

## 4. Patterns every consumer AutoTest needs

House patterns, each verified in multiple shipped tests (prevalence counts over the 216
corpus test files, 2026-07-03). Framework pattern skeletons live in AutoTest spec §7
(Pattern A signal-driven step machine, B settle-timer poll, C single-shot); below is the
consumer-hardened form of each.

### 4.1 Signal-driven step machine + `ScheduleSettle` (114/216 files)

Drive the test off the feature's signals, with a `_Step` counter; use a one-shot timer to
let the deferred-request/processor chain drain before asserting — **especially for negative
assertions** ("X did NOT fire"), which are only meaningful after a settle window. The
universal helper (corpus form, `BB_AutoTest_Door_MultiOccupant.as`):

```angelscript
private void ScheduleSettle(FCk_Handle& InOwner, float InSeconds, FName InHandlerName)
{
    auto _CkPerfScope = ck::ScopedStat();
    auto Params = FCk_Fragment_Timer_ParamsData(FCk_Time(InSeconds));
    Params.Set_StartingState(ECk_Timer_State::Running)
          .Set_Behavior(ECk_Timer_Behavior::StopOnDone);
    auto Timer = utils_timer::Add(InOwner, Params);
    Timer.BindTo_OnDone(FCk_Delegate_Timer(this, InHandlerName));
}
```

Guard every signal handler against late re-fires: `if (IsFinished()) { return; }` first line.
Pick settle durations against what you observe, not round numbers — the corpus fixed a test
whose 0.4 s settle let passive regen fully refill and auto-pause before a "still Running"
assert (BusterBlock `585fd6035`; fix was 0.15 s to catch the mid-flight state). And remember
mutations are deferred: reading a tag the same tick you added it asserts against the old
state — defer past `WaitOneFrame` (BusterBlock `de3099e1c`, second bullet; deferred-request
contract: `ckecs-architecture-contract` §3).

### 4.2 Freeze time-of-day and clocks (20 files reference `MinutesPerTick`)

Any test composing a day/clock system freezes it for determinism, then advances it
explicitly. Corpus form (`BB_AutoTest_Trashcan_NoPickupBeforeTime.as`): compose the test's
OWN clock with `MinutesPerTick = 0`, drive with `Request_AdvanceMinutes(...)`, settle, then
assert both the positive (fired at the boundary) and the negative (nothing before it).
A test that lets wall-clock time drive a scheduled behavior is a flake by construction.

### 4.3 `_TimeoutSeconds` override with a justifying comment (155/216 files — the norm)

The harness default is deliberately tight; overriding is normal, but the override carries
a one-line justification so the budget is reviewable:

```angelscript
class U<Prefix>_AutoTest_Door_MultiOccupant : UCk_AutoTest_Base
{
    // 8s budget — multi-step occupant flow with several request/signal
    // round-trips needs more than the harness default 5s.
    default _TimeoutSeconds = 8.0f;
```

Mechanics (CDO propagation to the wrapper): `ck-tests-authoring-and-running` §2a.

### 4.4 ActorRelay channel for replicated spawns (17 files)

The autotest entity does not replicate. Spawning a `_Replication = Replicates` entity script
under it trips the framework's `Get_Replication` ensure — the origin incident is BusterBlock
`de3099e1c` (MultiCounterIsolation, verified). The RULE and its production-side rationale
belong to ck-game-replication-patterns; the **test-side mechanics** are: acquire an
`ActorRelay.Generic` channel first, spawn replicated entity scripts under the channel entity,
keep non-replicating composition (frozen clocks, settle timers, `DoesNotReplicate` Adds) on
the autotest entity. Corpus form (`BB_AutoTest_Trashcan_NoPickupBeforeTime.as`, verified):

```angelscript
// Trashcan + Driver entity scripts both replicate, so they need a
// replicated AActor anchor as their lifetime owner — the autotest
// entity itself does not replicate. ActorRelay.Generic gives us one.
_PendingChannel = utils_actor_relay::Request_AcquireChannel(GameplayTags::ActorRelay_Generic);
_PendingChannel.Promise_OnAcquired(
    FCk_Delegate_ActorRelay_Acquired(this, n"OnChannelAcquired"));
```

…then in the acquired callback, validity-gate the channel entity and
`utils_entity_script::Request_SpawnEntity(LifetimeOwner, ...)` +
`Promise_OnConstructed` per spawn, counting down `_PendingConstructs` when spawning several.
Authority/replication gotchas at the harness level: AutoTest spec §8 GOTCHA 8.

### 4.5 `Debug_Force*` bypass hooks — make tests physics- and input-independent (15 files)

When a feature's entry point is physical (probe overlap, raw input), give the **production
feature** a `Request_Debug_Force*` request that enqueues through the same request/processor
pipeline and emits the same signals as the physical path (corpus:
`Script/ECS/Trigger/BB_Trigger_Processor_Requests.as` — "Applies Debug_ForceEnter /
Debug_ForceExit requests and emits the same [signals]"). Tests then drive
`Request_Debug_ForceEnter(...)` instead of simulating physics. This is a deliberate
production API addition, not test pollution: it keeps every occupancy/trigger test
independent of the physics engine, and it is the standard countermeasure to born-state
traps (§5.4). Real raw-input coverage belongs to the Gauntlet layer, not to AutoTests.

### 4.6 File furniture

Every corpus test file carries: a banner comment stating **what is verified** plus an
explicit "Out of scope (separate test files)" list; hard validity gates that
`FinishFailure("reason")` early; `FinishSuccess()`/`FinishFailure()` as the only terminals
(assertions do not auto-finish — AutoTest spec §6). Adopt all three from the first test.

---

## 5. Isolation and contamination — the shared-world trap class

All AutoTests in one map share **one PIE world and one suite run**. A test that is green
alone and red in the full suite is almost always a contamination victim (or perpetrator).
Four verified trap patterns:

### 5.1 Empty detection filter = match-all (orphan probes)

BusterBlock `3688dc72b` (verified): Door/Trigger dedup tests left `DetectionFilter` empty,
which matches every overlapping entity — including interactable-host probes **leaked at the
world origin by earlier CheckoutCounter tests** — inflating counts only in full-suite runs.
The idiom, now boilerplate in every ForceEnter-driven corpus test: give probes a
**non-matching registered tag** so they reject prior tests' orphans:

```angelscript
// ForceEnter-driven test — probe-overlap is incidental. Non-matching
// filter so the probes reject orphans leaked from earlier suite tests.
// Empty filter = "match all".
Params.DetectionFilter.AddTag(utils_gameplay_tag::ResolveGameplayTag(
    n"<Game>.<SomeRegistered>.<NonMatchingTag>"));
```

The dual discipline: **don't leave orphans yourself.** Destroy what you spawn, or compose it
on your own entity so harness teardown takes it.

### 5.2 Log/pump bleed into the NEXT test's capture window

BusterBlock `1ecf1a5cb` (verified): a test that called `FinishSuccess()` while its
just-detonated physics wreck was still constructing pushed a scheduler pump spike — and its
warning breakdown lines — **into the following test's log-capture window**, failing an
innocent test. Discipline: schedule a final quiescence settle before `FinishSuccess()` so
your own transients land in YOUR window, and cover any residual known-noisy lines with
`Get_ExpectedLogErrors` on your test (see §6.2), never with a global suppression.

### 5.3 Count assertions over shared world state

Never assert a count computed by scanning the shared world (tag scans, global queries) —
every prior test's survivors are in your denominator. Corpus lesson (NpcPopulation):
world-tag-scan counting cross-contaminated count-asserting tests; the fix was to make the
count **per-instance** — `Get_ActiveCount` now reads the population's own tracked record
(`Script/ECS/NpcPopulation/BB_NpcPopulation_Utils.as:51-54`, verified 2026-07-03), and each
test composes its OWN population and asserts against that handle
(`BB_AutoTest_TouristSpawner_PopulationAndCullAccounting.as`). If you must count over shared
state: clear at start, destroy your own at the end.

### 5.4 Born-state assumptions

Entities created already inside a trigger/probe volume do not produce an enter event —
overlap events fire on transitions, not on initial state. `[INFERRED — corpus operational
memory; no committed exhibit, but the countermeasure is the verified norm]`. Countermeasure:
never rely on spawn-position to arm a trigger — drive entry explicitly via the feature's
`Debug_ForceEnter` hook (§4.5), or spawn outside and move in. The same class of assumption
applies to any "initial state = event fired" reasoning: bind the signal FIRST, then cause
the transition.

---

## 6. Evidence standards

Aligned with the framework's evidence rules — `ck-tests-authoring-and-running` §4 is the
canonical statement (verdict artifacts, stale-binary trap, net-stub rebuild rule, baseline
capture); root doctrine `Plugins/CkFoundation/CLAUDE.md` non-negotiable #4 (three-environment
verification) and `ck-change-control` §4 govern anything that touches a public API surface.
Consumer-side deltas worth restating:

### 6.1 A test run is evidence only if it postdates your last edit AND you read the real verdict

- Re-run after the FINAL edit. A green run against code older than your last `.as` save or
  C++ build is void — and a failed AS compile means the editor is still running the OLD
  compiled code, so the run silently exercised nothing (check the fresh log for
  `Angelscript: Error` lines naming your file before trusting any post-edit run —
  `ck-tests-authoring-and-running` §4).
- Gate on the artifact, not the wrapper: the `-ReportExportPath` JSON/HTML, the
  `LogAutomationController` per-test verdict lines, or the process exit code (Gauntlet:
  exit code 0 is the contract — Gauntlet spec §9). A toolbox notification or a `.bat`'s
  "completed" chatter is a proxy.
- **Baseline before "no regressions."** Run the target filter before your change; record
  pass/fail counts and the NAMES of failing tests; report the delta afterward
  ("baseline 2 failing {a,b} → still 2 {a,b}"). Corpus expected-failure discipline goes
  further: known-red tests stay registered with an expected-failure marker and are excluded
  from the gate by name, so an unexpectedly-passing test is itself a reportable event.

### 6.2 Warnings fail AutoTests — and the sanctioned opt-out is per-test

Harness policy: any Warning- or Error-level log line captured during the test window fails
the test regardless of your `FinishSuccess()` (AutoTest spec §8 GOTCHA 1). The machinery
(verified 2026-07-03 in `Plugins/CkTests/Source/CkTests/Private/CkAutoTestRunner.cpp`):

- The runner installs a small default noise list (`GDefaultPlainPatterns`,
  CkAutoTestRunner.cpp:400-425 — EOS tick chatter, scheduler pump advisories, etc.) as
  suppress-all expected errors.
- Everything else needs a **per-test** `Get_ExpectedLogErrors` override
  (`CkAutoTestRunner.h:82-105`; opt-out mechanics: `ck-tests-authoring-and-running` §2a).
  Keep patterns narrow — corpus practice deliberately leaves genuine-saturation lines
  ("Pump limit [N] reached" breakdowns) failing while suppressing the bounded-burst advisory.
- Never blanket-disable (`_DisableDefaultLogSuppressions` exists for the opposite direction —
  making a test STRICTER). A warning your test tolerates via a broad pattern is a real
  defect you've stopped hearing about. Diagnose first (ck-game-debugging-playbook), suppress
  only what you can name.

### 6.3 Invocation — generic shape only

Editor (Session Frontend / `Automation RunTests <filter>`), headless
(`<Game>Editor-Cmd.exe ... -ExecCmds="Automation RunTests ..." -unattended -nullrhi`), and
Gauntlet dispatcher runs: exact command lines, filters, and flags are owned by
`ck-tests-authoring-and-running` §3 — cite it, don't memorize drifting flag lists. One
consumer-side addition: on multi-session machines, headless boots use `-skipcompile` so a
killed UBT can't delete module DLLs another session is using (corpus operational rule;
environment detail in `ck-build-and-env`).

---

## Common mistakes

1. **Feature commit without its AutoTest.** The corpus norm is same-commit (§1); a follow-up
   "tests later" commit historically means never.
2. **Reaching for Gauntlet when a ticking world suffices.** ~60 s/boot vs amortized PIE;
   Gauntlet is for real-process/input/boot integration only (§2).
3. **Asserting the same tick as a deferred mutation.** Requests and tag/fragment Adds are
   deferred — settle or `WaitOneFrame` first (§4.1; `ckecs-architecture-contract` §3).
4. **Negative assertion without a settle window.** "Did not fire" is only meaningful after
   the pipeline drained (§4.1).
5. **Replicated entity script spawned under the autotest entity.** Trips the replication
   ensure; use the ActorRelay channel (§4.4). An older `_NonReplicating`-subclass workaround
   exists in the corpus — do not extend it; it bypasses the production replication path.
6. **Empty `DetectionFilter` on test probes.** Match-all + shared world = counting other
   tests' orphans (§5.1).
7. **`FinishSuccess()` while your own transients are still landing.** They fail the next
   test, not yours (§5.2).
8. **Broad `Get_ExpectedLogErrors` patterns** that mute real defects along with the noise
   (§6.2).
9. **Claiming green from a run that predates your last edit**, or from a wrapper's "done"
   signal instead of the verdict artifact (§6.1).
10. **Blueprint test assets.** Never (§1.2).

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock superproject `52a75e13d` (corpus) and the CkTests
framework docs dated 2026-07-02. Grep/Glob tools are blind under `Script/` dirs (repo
`.ignore`) — all corpus searches below MUST use `rg --no-ignore` (Git Bash).

Re-verify volatile claims:

```bash
# Same-commit norm exemplars + wiring incidents (run at a BusterBlock checkout)
git show --stat 01300d524 9a6bfb897 0ddfada98 c27dfc88a
git show -s e0de34899 3688dc72b 1ecf1a5cb de3099e1c
git show --stat 68616a8a5 | tail -3

# Pattern prevalence counts (corpus test root)
rg --no-ignore --files Plugins/BusterBlockTests/Script/Tests -g "*.as" | wc -l   # 216
rg --no-ignore -l "_TimeoutSeconds"        Plugins/BusterBlockTests/Script/Tests -g "*.as" | wc -l  # 155
rg --no-ignore -l "ScheduleSettle"         Plugins/BusterBlockTests/Script/Tests -g "*.as" | wc -l  # 114
rg --no-ignore -l "Request_AcquireChannel" Plugins/BusterBlockTests/Script/Tests -g "*.as" | wc -l  # 17
rg --no-ignore -l "Debug_Force"            Plugins/BusterBlockTests/Script/Tests -g "*.as" | wc -l  # 15

# Warnings-fail machinery
rg -n "GDefaultPlainPatterns|Get_ExpectedLogErrors" Plugins/CkTests/Source/CkTests -g "*.h" -g "*.cpp"
```

Cited documents: `ck-tests-authoring-and-running` (CkTests skill — §1 layer table, §2a–2e
authoring, §3 running, §4 evidence rules); `Plugins/CkTests/Script/Common/CkAutoTest_CreationSpecification.txt`
(§4 naming, §6 base API, §7 patterns, §8 gotchas, §9 per-project setup, §11 scaling);
`CkGym_CreationSpecification.txt` (§2/§5 templates — check `Plugins/CkTests/CLAUDE.md`'s
trust table for its stale rows); `CkGauntlet_CreationSpecification.txt` (§7 dispatcher,
§9 exit codes, §10 gotchas); `Plugins/CkFoundation/CLAUDE.md` (non-negotiable #4);
`ck-change-control` §4; `ckecs-architecture-contract` §3. Adjudication watch: A2 (C++ test
naming). If BusterBlockTests moves again or the harness suppression list changes, update
§3.1/§6.2 and the counts above.
