# CkCore / Chrono

Countdown / accumulator primitive. Not ECS-aware — pure value type. Use `FCk_Chrono` when you want "tick toward a goal time, consume delta, detect done" semantics as a plain struct (not a timer entity).

**Key files:** `CkChrono.h`, `CkChrono_Utils.h`

## Public types

```cpp
UENUM() enum class ECk_Chrono_ConsumeState : uint8 { CouldNotConsume, Consumed, FullyConsumed };
UENUM() enum class ECk_Chrono_TickState    : uint8 { Ticking, Done };

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Chrono
{
    using TimeType          = FCk_Time;
    using TickStateType     = ECk_Chrono_TickState;
    using ConsumeStateType  = ECk_Chrono_ConsumeState;

    explicit FCk_Chrono(TimeType InGoalValue);

    auto Tick    (const TimeType& InDeltaT) -> TickStateType;
    auto Consume (const TimeType& InDeltaT) -> ConsumeStateType;
    auto Complete() -> ThisType&;
    auto Reset   () -> ThisType&;
    // ... + more (see CkChrono.h for full API)
};
```

## Semantics

- `Tick(DeltaT)` — advance toward goal. Returns `Ticking` until goal is reached, then `Done` on the tick that crosses it, then `Done` on every subsequent tick (you must `Reset` to start over).
- `Consume(DeltaT)` — try to "spend" `DeltaT` against the remaining. `CouldNotConsume` (not enough time left), `Consumed` (partial), `FullyConsumed` (exactly hit goal).
- `Complete()` — snap to goal, subsequent `Tick` returns `Done`.
- `Reset()` — back to zero progress.

## When Chrono vs. CkTimer

- `FCk_Chrono` is a **value type**. Store it as a member, advance it in a processor, compare its state. No entity, no lifecycle, no signals.
- `CkTimer` (separate module) is an **ECS-scoped timer entity** with lifecycle, signals (`OnFinished`, etc.), and BP-visible delegates.

Rule of thumb: if you want `OnFinished`-style callbacks from BP or AS, use `CkTimer`. If you just need to track progress inside a processor, use `FCk_Chrono`.

## Depends on
`Enums/`, `Time/`, `Format/`.

## Used by
`CkTimer` (as its underlying time-tracking primitive), anywhere that needs a non-entity countdown.

## See also
- `Time/README.md` — `FCk_Time` is Chrono's time representation.
- `CkTimer/` module — higher-level, entity-based timer.
