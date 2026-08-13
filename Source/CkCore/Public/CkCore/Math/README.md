# CkCore / Math

Umbrella folder containing 8 math subfolders. Each subfolder has its own purpose; this README is the index.

## Subfolders

| Folder | Purpose | Representative types |
|---|---|---|
| `Arithmetic` | Typed arithmetic helpers that work across numeric types (generic min/max/lerp/clamp variants beyond UE's built-ins). | |
| `Comparison` | Typed comparison helpers; `EpsilonEquals` style for floats, structural compare for composite types. | |
| `FloatCurve` | `FRuntimeFloatCurve` evaluation helpers (safe sampling, normalization). | |
| `Geometry` | 2D/3D geometry primitives and intersection tests — AABB, OBB, sphere, capsule, line, plane. | `FCk_Geometry_*` types in `CkGeometry_Types.h`; utilities in `CkGeometry_Utils.h` / `UCk_Utils_Geometry_UE`. |
| `NumericLimits` | Type-parameterized `Min/Max` that don't fall over on custom numeric wrappers. | |
| `Probability` | Weighted pick, dice-style sampling, probability distributions, sampling without replacement. | `ck::TShuffleBag<T>` in `CkShuffleBag.h` |
| `ValueRange` | `FCk_ValueRange<T>` — a typed `[Min, Max]` with clamp / normalize / interpolate / contains. Used for attribute ranges, health/stamina bounds, dynamic probe radii, etc. | `FCk_ValueRange<float>`, `FCk_ValueRange_*` specializations. |
| `Vector` | Vector helpers beyond UE's `FVector`/`FVector2D` built-ins (component-wise ops, directional helpers, swizzles). | |

## Rule of thumb

1. **Check UE's built-ins first** (`FMath::`, `FVector::`, etc.). Only reach for `CkCore/Math/*` when UE doesn't have it or UE's version isn't type-agnostic enough.
2. **Bounded range? → `ValueRange`.** Don't roll your own `(Min, Max, Current)` triplet — meters / attributes / probe radii all converge on `FCk_ValueRange`. This makes clamping and normalization consistent.
3. **Geometry tests? → `Geometry`.** Don't write a new AABB-vs-sphere; it already exists here and is used by physics/overlap/probes.
4. **Weighted random? → `Probability`.** Don't `FMath::RandRange` + if-ladders for weighted picks.
5. **Random that must not streak? → `Probability`'s `ck::TShuffleBag`.** Don't hand-roll Fisher-Yates + refill loops.

## Depends on
`Macros/`, `Enums/` (several math subfolders use central enums), `Format/` (for diagnostic printing).

## Used by
`CkAttribute` (ValueRange for attribute ranges), `CkMeter` (ValueRange), `CkPhysics` / `CkProjectile` / `CkSpatialQuery` / `CkOverlapBody` (Geometry), `CkProbability`-flavored gameplay features.

## See also
- `Meter/` — `FCk_Meter` composes `ValueRange` + current + `FCk_Chrono` for auto-regen metering.
- `Shapes/` (`CkShapes` module) — higher-level shape definitions, wraps `Geometry` primitives.
