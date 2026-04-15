# CkTween

**Purpose:** Tweening/interpolation entities — smoothly interpolates a value (float, vector, color) over time with an easing curve, writing the result to a target fragment field. Driven by `CkTimer` internally.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTimer`.
**Used by:** UI animations, ability VFX, camera transitions.

---

## Key API

- `UCk_Utils_TweenEasing_UE` — easing curve queries.
- Tween entities specify From/To values and an easing function; the tween processor writes the current interpolated value each tick.

---

## Pattern

Create a tween entity for a specific property; the tween processor reads `Alpha` from `CkTimer` and evaluates the easing; result is written back to the target fragment field.

---

## Anti-patterns

Don't manually lerp values in a processor over multiple frames — that's exactly what `CkTween` is for.

---

## See also

- `CkTimer/Claude.md` — tween duration/completion.
- `CkCore/Math/ValueRange/README.md` — `FCk_ValueRange` for clamped ranges.
