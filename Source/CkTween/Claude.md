# CkTween

**Purpose:** Tweening/interpolation entities — smoothly interpolates a value (float, vector, color) over time with an easing curve, writing the result to a target fragment field. Driven by `CkTimer` internally.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkSpline`, `CkTimer`.
**Used by:** UI animations, ability VFX, camera transitions.

---

## Key API

- `UCk_Utils_TweenEasing_UE` — easing curve queries.
- Tween entities specify From/To values and an easing function; the tween processor writes the current interpolated value each tick.

---

## Pattern

Create a tween entity for a specific property; the tween processor reads `Alpha` from `CkTimer` and evaluates the easing; result is written back to the target fragment field.

---

## Spline-follow

`Create_TweenEntity_FollowSpline` moves an entity along a `CkSpline` path (an
`FCk_Handle_Spline`, created via `UCk_Utils_Spline_UE`). It is a single float-progress
(0→1) tween — **not** a chain of per-segment tweens, so there is no per-segment duration to
compute; duration is just the total travel time.

- **Consumes an FCk_Handle_Spline:** `FFragment_Tween_SplineFollow` holds the spline
  handle; the processor resolves progress → transform every tick via
  `UCk_Utils_Spline_UE::Get_LocationAtDistance` / `Get_RotationAtDistance` (same "store a
  reference, resolve each frame" shape as the Follow-Target tweens). A destroyed spline
  entity cleanly stops the follower via the now-invalid handle.
- **Constant speed:** with `ECk_TweenEasing::Linear` (the default for this util) the
  follower moves at constant speed, because the spline's distance API is
  arc-length-parameterized. Any other easing eases the whole journey.
- **Orientation:** `ECk_Tween_SplineOrientation::OrientToSpline` also rotates the entity
  to face the spline tangent; `PositionOnly` leaves rotation untouched.
- `FProcessor_Tween_ApplySplineFollow` (group `FGroup_Transform`, before
  `Transform_HandleRequests`) resolves progress → transform each tick; the completion
  snap is handled in `FProcessor_Tween_Update::DoCheckLoopCompletion`.

---

## Anti-patterns

Don't manually lerp values in a processor over multiple frames — that's exactly what `CkTween` is for.

Don't chain one tween per spline control point — straight-line segments approximate a
polyline, not the curve. Use `Create_TweenEntity_FollowSpline`.

---

## See also

- `CkTimer/Claude.md` — tween duration/completion.
- `CkCore/Math/ValueRange/README.md` — `FCk_ValueRange` for clamped ranges.
