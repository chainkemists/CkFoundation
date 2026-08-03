# AutoTest patterns and the contamination trap class

Reference for `ck-game-testing-discipline`: the patterns every consumer AutoTest needs (§4) and the shared-world isolation failures (§5).

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

