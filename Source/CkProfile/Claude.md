# CkProfile

**Purpose:** CPU profiling integration — declares stats groups used by CkFoundation processors and subsystems. Essentially a thin module containing `DECLARE_STATS_GROUP` macros that register named stat groups with Unreal Insights / Stat command.

**Depends on:** `CkCore`, `CkLog`.
**Used by:** `CkEcs` (processor tick stats), `CkTimer`, and any module that wants named stat groups.

---

## What's here

```
CkProfile/Public/CkProfile/
└── Stats/
    └── CkStats.h  – stat group declarations
```

`CkStats.h` declares the stat groups visible under the `stat` console command (e.g., `stat CkProcessors`, `stat CkProcessors_Details`). Feature modules include this header and use standard UE stat macros (`SCOPE_CYCLE_COUNTER`, `DECLARE_CYCLE_STAT`) against these groups.

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

## Anti-patterns

1. Don't declare new top-level stat groups here unless they genuinely belong to CkFoundation infrastructure. Feature-module stats should use the existing `CkProcessors` / `CkProcessors_Details` groups.
2. Don't leave `SCOPE_CYCLE_COUNTER` calls in hot paths in shipping builds without guarding — the stat system has near-zero overhead when disabled but the guard is good practice.

---

## See also
- `CkMemory/Claude.md` — memory tracking (orthogonal to CPU profiling).
- UE Insights documentation — the visualization tool that consumes these stat groups.
