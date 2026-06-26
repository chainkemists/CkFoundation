# CkProfile

**Purpose:** CPU profiling integration — declares stats groups used by CkFoundation processors and subsystems. Essentially a thin module containing `DECLARE_STATS_GROUP` macros that register named stat groups with Unreal Insights / Stat command.

**Depends on:** `CkCore`, `CkLog`.
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

## Anti-patterns

1. Don't declare new top-level stat groups here unless they genuinely belong to CkFoundation infrastructure. Feature-module stats should use the existing `CkProcessors` / `CkProcessors_Details` groups. (`STATGROUP_CkScript` is sanctioned: it's script-profiling infrastructure, the script-side peer of `CkProcessors`.)
2. Don't leave `SCOPE_CYCLE_COUNTER` calls in hot paths in shipping builds without guarding — the stat system has near-zero overhead when disabled but the guard is good practice.

---

## See also
- `CkMemory/Claude.md` — memory tracking (orthogonal to CPU profiling).
- UE Insights documentation — the visualization tool that consumes these stat groups.
