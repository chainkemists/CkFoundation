# CkProfile

**Purpose:** CPU profiling integration — declares stats groups used by CkFoundation processors and subsystems. Essentially a thin module containing `DECLARE_STATS_GROUP` macros that register named stat groups with Unreal Insights / Stat command.

**Depends on:** `CkCore`, `CkLog`, `RenderCore` + `RHI` (the `stat unit` counters — see *Thread timings*).
**Used by:** `CkEcs` (processor tick stats), `CkTimer`, and any module that wants named stat groups.

---

## What's here

```
CkProfile/Public/CkProfile/
└── Stats/
    ├── CkStats.h          – stat-id template machinery + CK_STAT / CK_CREATE_DYNAMIC_STAT_ID macros
    ├── CkProfile_Stats.h  – STATGROUP_CkScript (script-driven scope timings)
    └── CkScopedStat.h/.cpp – FCk_ScopedStat: AngelScript-facing RAII scope guard
```

`CkStats.h` holds the stat-id machinery and the `CK_STAT` / `CK_CREATE_DYNAMIC_STAT_ID` macros. Feature modules use standard UE stat macros (`SCOPE_CYCLE_COUNTER`, `DECLARE_CYCLE_STAT`) against groups they declare in their own `<Module>_Stats.h`.

---

## Usage

```cpp
#include "CkProfile/Stats/CkStats.h"

DECLARE_CYCLE_STAT(TEXT("MyProcessor::ForEachEntity"),
                   STAT_MyProcessor_ForEachEntity,
                   STATGROUP_CkProcessors_Details);

auto FProcessor_MyFeature::ForEachEntity(...) -> void
{
    SCOPE_CYCLE_COUNTER(STAT_MyProcessor_ForEachEntity);
    // ...
}
```

Use `STATGROUP_CkProcessors` for per-processor tick overhead (coarser) and `STATGROUP_CkProcessors_Details` for per-ForEachEntity body (finer).

---

## Scoped stats from AngelScript

`ck::ScopedStat` (C++ `FCk_ScopedStat`) is the script equivalent of `CK_STAT`. C++ names a stat by type at compile time (`cleantype::clean<T>()`); the script version derives the name at runtime.

```angelscript
{
    auto _Stat = ck::ScopedStat();                       // auto-named "<Class>::<Method>"
    // ...work to measure...
} // recorded on scope exit -> `stat CkScript` / Unreal Insights

auto _Sub = ck::ScopedStat("AI::EvaluateGoals::Phase2"); // explicit name (sub-method scopes)
```

`auto _S = ck::ScopedStat()` is the intended idiom — AngelScript constructs the value in place (no copy), so the non-copyable guard records exactly once on scope exit. The no-arg form reads the calling script function from the active context (`ck::Get_ActiveScriptScopeName()`, also bound to script) — no string to type. `UFUNCTION(BlueprintOverride)` handlers report as `Method_Implementation`; the suffix is stripped so the name matches the clean handler name. Prefer the explicit-string form in genuinely hot per-frame loops, where skipping the per-call context lookup is worth it.

It's a non-copyable value type: `FScopeCycleCounter` under `STATS`, a named CPU event otherwise (mirrors `CK_STAT`'s non-STATS fallback). The resolved name goes through a per-thread `TStatId` cache (`ck::Get_ScopedStat_StatId`) so hot loops don't pay the `CreateStatId` FName-registration cost on every scope entry. Stats land in `STATGROUP_CkScript`.

---

## Thread timings — the `stat unit` numbers as data

`UCk_Utils_Stats_UE::Get_ThreadTimings()` returns `FCk_Stats_ThreadTimings`: a coherent
single-frame snapshot of frame, game-thread, render-thread, RHI-thread and GPU time. It exists
because the four numbers are only comparable when they come from the same frame, and because a
missing GPU measurement has to be distinguishable from a cheap one.

Two contracts to preserve when touching it:

**It mirrors `stat unit`'s RAW values by construction.** Every expression is copied from
`FStatUnitData::DrawStat` (`Engine/Source/Runtime/Engine/Private/UnrealClient.cpp:350`) —
`FPlatformTime::ToMilliseconds` over `GGameThreadTime` / `GRenderThreadTime` / `GRHIThreadTime`,
`RHIGetGPUFrameCycles()` for the GPU, and `FApp::GetCurrentTime() - FApp::GetLastTime()` for the
frame delta (the engine uses that form rather than `GetDeltaTime()` because it accounts for
end-of-frame idling and so lines up with the thread times). `stat unit` additionally displays an
exponentially smoothed variant (`X = 0.9*X + 0.1*RawX`); **that smoothing is deliberately not
reproduced** — it would destroy any percentile, 1%-low or outlier statistic computed over a window
of these samples. If you ever add a smoothed accessor, add it beside the raw one, never in place of
it.

**A metric that could not be measured reports a reason, never a zero.** `Get_GpuAvailability()`
returns `ECk_Stats_MetricAvailability`, and `Get_GpuTimeMs()` is meaningful only when that is
`Available`. The availability test is derived from the cycle count exactly as the engine derives its
own (`bHaveGPUData = RawGPUFrameTime > 0`, `UnrealClient.cpp:514`), so `Available` and a zero time
cannot co-exist — a pinned invariant, not a convention. A GPU time of zero presented as data would
read as an infinitely fast GPU and would corrupt every figure derived from it. `-nullrhi` runs
report `Unavailable_NullRhi`, which is why the tests assert semantics rather than values.

Reads GPU index 0 only. Multi-GPU splitting is a deliberate omission: it would need its own
reporting axis, not just another field.

Tests: `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkProfile/Test_Profile_ThreadTimings.cpp`.

---

## Anti-patterns

1. Don't declare new top-level stat groups here unless they genuinely belong to CkFoundation infrastructure. Feature-module stats should use the existing `CkProcessors` / `CkProcessors_Details` groups. (`STATGROUP_CkScript` is sanctioned: it's script-profiling infrastructure, the script-side peer of `CkProcessors`.)
2. Don't leave `SCOPE_CYCLE_COUNTER` calls in hot paths in shipping builds without guarding — the stat system has near-zero overhead when disabled but the guard is good practice.

---

## See also
- `CkMemory/Claude.md` — memory tracking (orthogonal to CPU profiling).
- UE Insights documentation — the visualization tool that consumes these stat groups.
