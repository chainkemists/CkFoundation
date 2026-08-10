# CkInsightsAnalyzer

**Purpose:** UI-free Unreal Insights trace analysis, reporting, and commandlet support. The editor tab lives in CkGameplayDebugger's `CkInsightsDebugger` module.

**Depends on:** `CkCore`, `CkLog`, JSON, and Unreal Trace analysis/services.
**Used by:** `CkInsightsDebugger` and developer commandlet workflows; not referenced by gameplay modules.

---

## Key API

- No `_Utils.h`. Exposes C++ trace-analysis/report types plus `UCkInsightsAnalyzerCommandlet`.
- `FCk_FrameReport::BuildHotPathTree / ComputeCategorySummary / ComputeTopTimers` —
  structured (non-markdown) analysis data backing debugger and commandlet consumers.

---

## Pattern

Run headlessly through `-run=CkInsightsAnalyzer`; use CkGameplayDebugger's Insights Analyzer tab for Slate workflows.

---

## Packaged QA boundary

`CkInsightsAnalyzer` is a `DeveloperTool` module so packaged Development and DebugGame QA builds can
analyze captures. UBT excludes it from Test and Shipping targets. Keep it UI-free and do not move it
to `Runtime` or introduce gameplay-module dependencies.

---

## Implementation notes — hot-path tree (`FCk_FrameReport`)

- **Never collapse a `script::<Class>` scope into its dispatch child.** That scope IS the
  attribution the tree exists to show. Without the guard in `CollapseWrappers`, every script
  processor (near-zero self-time, one dominant VM child) collapses into the same anonymous
  `ForEachBatch` timer and dedup then merges them all into one unattributable row.
- **`ShownTimers` is per-root, not global.** A single shared set was too aggressive: a timer
  appearing in one root's subtree got hidden from every later root, even when the roots
  represented different call paths. Both `GenerateHotPaths` and `BuildHotPathTree` allocate the
  set inside the per-root loop.
- The markdown report's ordering (categories and timers sorted by exclusive time, descending)
  was carried over from the Python script this tool replaced; keep it stable so old and new
  reports stay comparable.

---

## See also

- `CkProfile/Claude.md` — stat groups captured by Insights.
- `../../../CkGameplayDebugger/Source/CkInsightsDebugger/CLAUDE.md` — editor-tab ownership and UI contracts.
