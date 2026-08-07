# CkMemory

**Purpose:** Memory tracking subsystem for CkFoundation. Provides allocation counting, snapshot comparisons, and a custom allocator that routes through the subsystem. Used to detect leaks and track per-frame allocations in development.

**Depends on:** `CkCore`, `CkLog`.
**Used by:** `CkEcs`, `CkWatermark`.

---

## Public API

```cpp
// Get a snapshot of current memory counts
UFUNCTION(BlueprintPure)
static FCk_Utils_Memory_MemoryCountSnapshot_Result
Get_MemoryCountSnapshot(const UObject* InContext = nullptr);
```

`FCk_Utils_Memory_MemoryCountSnapshot_Result` is a struct capturing allocation counts at the moment of the call. Diff two snapshots to find allocations between two points.

`CkMemory_Subsystem.h` / `CkMemory_Processor.h` — the world subsystem and the processor that ticks allocation tracking each frame.

`Allocator/` — custom UE allocator implementation routed through the subsystem counters.

---

## Typical use

```cpp
const auto Before = UCk_Utils_Memory_UE::Get_MemoryCountSnapshot(this);
// ... do work ...
const auto After = UCk_Utils_Memory_UE::Get_MemoryCountSnapshot(this);
// Compare before/after to detect unexpected allocations in the work block.
```

---

## Note on scope

`CkMemory` is a low-level infrastructure module. Most game code never calls it directly. If you're hitting memory issues, prefer the Insights Analyzer tab in CkGameplayDebugger (backed by CkFoundation's `CkInsightsAnalyzer`) rather than adding custom snapshot calls.

---

## See also
- `CkProfile/Claude.md` — CPU profiling (stats groups). Memory and profile are orthogonal.
- `CkInsightsAnalyzer/Claude.md` — the tooling surface for Insights analysis.
