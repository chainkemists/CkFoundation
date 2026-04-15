# CkCore / Time

World-time retrieval and the `FCk_Time` / `FCk_Time_Unreal` types. This is the canonical way to ask "what time is it in this world, with/without dilation, with/without pause?"

**Key files:** `CkTime.h`, `CkTime_Utils.h`

## Public types

```cpp
UENUM() enum class ECk_Time_WorldTimeType : uint8 { /* ... Paused/Dilated variants ... */ };

USTRUCT() struct FCk_Time        { /* CkFoundation time representation */ };
USTRUCT() struct FCk_Time_Unreal { /* wrapper with UE conversion */ };

USTRUCT() struct FCk_Utils_Time_GetWorldTime_Params
{
    explicit FCk_Utils_Time_GetWorldTime_Params(UObject* InObject);
    explicit FCk_Utils_Time_GetWorldTime_Params(UWorld*  InWorld);

    CK_PROPERTY_GET(_Object);     // world-context object
    CK_PROPERTY_GET(_World);      // or an explicit UWorld
    CK_PROPERTY(_TimeType);       // default = PausedAndDilatedAndClamped
};

USTRUCT() struct FCk_Utils_Time_GetWorldTime_Result
{
    CK_PROPERTY_GET(_WorldTime);  // FCk_Time_Unreal
};
```

## Canonical usage

```cpp
const auto TimeParams  = FCk_Utils_Time_GetWorldTime_Params{World};
const auto TimeResult  = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
const auto CurrentTime = TimeResult.Get_WorldTime().Get_Time();
```

Pick the `ECk_Time_WorldTimeType` that matches what your system cares about:

- `PausedAndDilatedAndClamped` — respects both pause and dilation, clamped to `>= 0`. Use for most gameplay timers.
- Unpaused variants — e.g., UI or ambient systems that should keep advancing when the game is paused.
- Dilated vs undilated — pick based on whether slow-mo should affect the subsystem.

Exact list of policies is in `CkTime.h`'s `ECk_Time_WorldTimeType` definition.

## Why not `World->GetTimeSeconds()`?

1. Different call sites end up picking different pause/dilation behavior by accident. Centralizing behind `ECk_Time_WorldTimeType` makes the choice explicit and auditable.
2. `FCk_Time` is the type every CkFoundation time-consuming API (Chrono, Timer, Tween, animation, probes, VFX cues, …) expects. Bouncing through `float` loses precision and semantic meaning.
3. AngelScript sees `FCk_Time` / `FCk_Time_Unreal` directly; raw `float` seconds wouldn't register.

## Depends on
`Macros/`.

## Used by
Every system that schedules, animates, or decays over time: `CkTimer`, `CkTween`, `CkCue`, `CkAnimation`, `CkVfx`, `CkStateMachine`, `CkObjective`, many more.

## See also
- `Chrono/README.md` — uses `FCk_Time` as its tick delta type.
- `CkTimer/` module — entity-based timers.
