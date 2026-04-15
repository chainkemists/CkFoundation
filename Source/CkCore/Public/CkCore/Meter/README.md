# CkCore / Meter

`FCk_Meter` — a value range with a current value, mask-based dirty tracking, and optional Chrono-driven auto-regen. Plain struct, no entity. Reach for this when you'd otherwise invent "health = (min, max, current, regenRate)" yet again.

**Key files:** `CkMeter.h`, `CkMeter_Data.h`, `CkMeter_Utils.h`

## Public types

```cpp
UENUM(meta = (Bitflags, ScriptName = "ECk_Meter_Mask"))
enum class ECk_Meter_Mask : uint8 { None = 0, Min = 1<<0, Max = 1<<1, Current = 1<<2 };

// FCk_Meter declared in CkMeter.h — composes FCk_ValueRange + current + chrono/rate.
```

## When to use

- Health / mana / stamina **plain values**. (For entity-scoped attributes with signals and providers, use `CkAttribute`.)
- Cooldown bars, progress bars where you want a single struct that answers both "how full" and "how close to empty."
- Any `(min, max, current)` triple with clamp+normalize behavior.

## When NOT to use

- If you need `OnChanged` signals across entities → `CkAttribute`.
- If you just want a `[0, 1]` lerp → `FCk_ValueRange<float>` directly (Math/ValueRange).
- If you need a timer with a callback → `CkTimer` or `FCk_Chrono`.

## Depends on
`Chrono/`, `Macros/`, `Math/ValueRange/`.

## Used by
UI progress widgets, non-attribute gameplay bars, any "bar-shaped" value that doesn't warrant a full entity.

## See also
- `CkAttribute/` module — entity-scoped attributes with providers and signals.
- `Math/ValueRange/` — the underlying bounded-value primitive.
