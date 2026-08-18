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

## Request completion — the reference implementation

CkTimer is the pilot for the framework-wide request-completion contract (root
[CLAUDE.md](../../CLAUDE.md) § Requests; mechanism in
[CkEcs/CLAUDE.md](../CkEcs/CLAUDE.md) § Signals). The delegate is carried ON the request struct —
no request entity, no signal. Copy these four sites when rolling it out:

- **`CkTimer_Utils.cpp`** — each deferred `Request_*` builds a named request local, stores the
  delegate on it under `if (InDelegate.IsBound()) { Request.Set_CompletionDelegate(InDelegate); }`,
  then enqueues that same local.
- **`FProcessor_Timer_HandleRequests`** — the drain lambda declares
  `auto Result = ECk_Request_OperationResult::Failed;`, then
  `const auto Guard = ck::MakeCompletionGuard(InRequest, InTimerEntity, Result);` (guard AFTER the
  local it references), and sets `Succeeded` after `DoHandleRequest` returns. CkTimer's handlers are
  `void` with no rejection path, so reaching that line IS the success condition; a feature whose
  handlers can fail must thread the result out. The drain iterates a COPY of the queue — the
  delegate rides the copy, which is why firing from there is correct.
- **`FProcessor_Timer_HandleRequests`'s view** — `TExclude<FTag_DestroyEntity_Initiate>` alongside
  `CK_IGNORE_PENDING_KILL`. `Request_DestroyEntity` adds Initiate synchronously, so a timer
  destroyed on the same stack that enqueued a request is already excluded when the drain runs that
  frame: the request deterministically reaches the cancel processor instead of racing it.
- **`FProcessor_Timer_CancelPendingRequests`** (`FGroup_EndPlay`, `CK_IF_END_PLAY`) — calls
  `ck::request::FireCancelledForPending` so a destroyed timer's undrained queue completes with
  `Failed_Cancelled` instead of hanging its caller.

- **Immediate mutators** — `Request_ChangeCountDirection` and `Request_ReverseDirection` flip tags
  inline and enqueue nothing, so there is no request struct and no handler. They take the same
  trailing delegate and fire it synchronously on the caller's stack after the mutation:
  `InDelegate.ExecuteIfBound(InTimerEntity, ECk_Request_OperationResult::Succeeded);`. This is the
  house shape for every trivial setter that mutates at the Utils boundary.

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
not a bug in the handler — and under C5 it is the intended shape: the durable fact is the DEADLINE, held by
the feature that started the timer, and that feature's Setup re-creates the timer from it on load.

It is no longer silent. Each such timer is recorded in `FCk_Snapshot_SaveReport::_UncapturedRuntimeEntities`
(read it off `Get_LastSaveReport`), with one summary Display line per save and per-entity detail at Verbose.
If a timer genuinely must survive, the fix is to give its entity durable identity or to keep the deadline on
the persisted owner — not to make the capture guess.

`HydrationApply` re-drives state through DEFERRED requests only — the chrono's `_CurrentValue` is
friend-gated and is never written directly — and every `NotReady` return precedes every request:

1. **Gate on COMPOSITION only** (`UCk_Utils_Timer_UE::Has`). It must NOT wait on `FTag_Timer_NeedsSetup`:
   a load holds a restored entity out of every non-kernel processor's view until its payloads have applied,
   so waiting for Setup waits for something that cannot happen and the payload is dropped at the apply
   timeout. The baseline concern the old gate was reaching for is handled by the ordering instead — every
   step below enqueues a DEFERRED request, and `FProcessor_Timer_HandleRequests` both `RunAfter`
   `FProcessor_Timer_Setup` and excludes `FTag_Timer_NeedsSetup`, so Setup Completes the CountDown chrono
   first and the absolute Jump is applied on top of that baseline, never before it.
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
