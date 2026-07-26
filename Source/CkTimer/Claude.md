# CkTimer

**Purpose:** Entity-scoped timers — countdown timer entities with `OnFinished` signals, auto-destroy, BP/AS delegate binding, repeat, and pause support. Built on `FCk_Chrono` internally.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProfile`, `CkRecord`.
**Used by:** `CkAudio`, `CkCue`, `CkStateMachine`, `CkTween`, `CkVfx`, and any system with delays or cooldowns.

---

## Key API

- `UCk_Utils_Timer_UE` (inherits `UCk_Utils_Ecs_Base_UE`) — add timer entity, set duration, bind `OnFinished`.
- Standard Add / Has / Cast helpers.
- Signals: `OnFinished`, `OnTick` (optional per-tick callback).

---

## Pattern

```cpp
auto TimerHandle = UCk_Utils_Timer_UE::Add(InHandle, TimerParams);
UCk_Utils_Timer_UE::BindTo_OnFinished(TimerHandle, MyDelegate,
    ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
    ECk_Signal_PostFireBehavior::Unbind);
```

---

## Anti-patterns

1. Don't use UE's `GetWorldTimerManager().SetTimer` for ECS-related delays — they're not tied to entity lifetime.
2. Don't hold timer entity handles past `OnFinished` if the timer auto-destroys.

---

## Save/load persistence

`FCk_SaveData_Timer` (`CkTimer_Fragment_Data.h`) is registered `Register_SaveOnly` in
`CkTimer_Fragment.cpp` — `Produce` + `HydrationApply`, **no net Apply**: timers are unreplicated, so the
payload never enters a replicated container and the authority-side `FProcessor_Hydration_Dispatch` is its
sole applier. The `FCk_SaveData_` prefix (rather than `FCk_RepData_`) marks exactly that and keeps the type
off the RepData census.

**Why a payload at all:** the runtime position lives in the chrono's `_CurrentValue`, which is
`UPROPERTY(Transient)` and does not survive the v3 rebuild+hydrate load. The entity is re-Constructed from
its spawn recipe (Params re-derive GoalValue/direction/behavior) but the chrono resets to its start — 0 for
CountUp, GoalValue for CountDown-after-Setup. The payload captures the three things the rebuild loses:

- `_Elapsed` — the chrono position (`Get_TimeElapsed()`) at capture.
- `_CountDirection` — the RUNTIME direction (the `FTag_Timer_Countdown` tag).
  `Request_ChangeCountDirection` / `Request_ReverseDirection` flip that tag **without touching Params**, so
  a runtime flip is otherwise lost on rebuild. (Despite the `Request_` name, `Request_ChangeCountDirection`
  mutates the tag synchronously — it is not queued.)
- `_RunState` — running vs paused (the `FTag_Timer_NeedsUpdate` tag). "Done" is not a distinct run-state;
  it is encoded by `_Elapsed` reaching GoalValue.

**COVERAGE LIMITATION — only Construct-composed timers persist.** A timer is a child entity created by
`UCk_Utils_Timer_UE::Add`. When that `Add` ran during the owner's Construct (recipe capture), the child gets
a spawn recipe and is rebuilt on load, so Produce/HydrationApply run for it. Timers added at RUNTIME
(post-BeginPlay — e.g. an SM task's `WaitForNewTimer`) are RuntimeSpawned-with-no-recipe under the v3
rebuild model, are NOT rebuilt on load, and therefore do not persist. That is the rebuild-model contract,
not a bug in the handler.

`HydrationApply` re-drives state through DEFERRED requests only — the chrono's `_CurrentValue` is
friend-gated and is never written directly — and every `NotReady` return precedes every request:

1. **Gate until Setup has run** (`FTag_Timer_NeedsSetup` cleared). `FProcessor_Timer_Setup` Completes a
   CountDown chrono to GoalValue when it consumes that tag; re-driving the position first would let Setup
   mutate the chrono *after* the Jump delta was computed, landing the timer at the wrong elapsed. Waiting
   makes the current-elapsed baseline stable and guarantees the enqueued Jump is not clobbered. NotReady is
   transient — Setup runs every authority tick.
2. **Reposition via an ABSOLUTE `Request_Jump`.** The Jump handler
   (`FProcessor_Timer_HandleRequests::DoHandleRequest`) owns the direction-dependent delta math and is the
   single source of truth: in absolute mode `JumpDuration` is the TARGET elapsed and the handler Ticks
   (CountUp) / Consumes (CountDown) by the gap against the current elapsed. For CountUp it Resets to 0 first,
   because `Tick` early-outs on an already-Done chrono and a backward absolute jump would silently no-op.
   Jump broadcasts OnTimerJump/OnTimerUpdate — never OnTimerDone — so positioning a terminal timer at
   GoalValue does not re-fire completion.
3. **Restore run-state with Resume/Pause — NEVER `Request_Complete`.** It is the only request that
   re-broadcasts OnTimerDone. A restored terminal timer is already at GoalValue from the Jump, and its
   persisted run-state is Paused (PauseOnDone/StopOnDone remove NeedsUpdate on completion), so the Update
   processors never run it; even in a pathological Running+done case `FProcessor_Timer_Update` /
   `_Update_Countdown` early-out at the top.

---

## Profiling stat-id

`ck::MakeStatIdFromParams` derives a timer's `TStatId` ("Timer Broadcast Event [\<name\>]") from its Params
on the fly inside the processors, under `#if STATS`, to scope each signal broadcast's CPU cost. It is
deliberately **not** cached as a fragment: a cached fragment did not survive snapshot restore (restored
timers came back without it), and per-broadcast derivation has no such gap. The cost is a STATS-only
string-format plus dynamic-stat lookup per broadcast, never present in Shipping/Test.

---

## See also

- `CkCore/Chrono/README.md` — `FCk_Chrono` for in-processor countdown without an entity.
- `CkCore/Time/README.md` — world time retrieval.
