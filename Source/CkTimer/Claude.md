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

## See also

- `CkCore/Chrono/README.md` — `FCk_Chrono` for in-processor countdown without an entity.
- `CkCore/Time/README.md` — world time retrieval.
