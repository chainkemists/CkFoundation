# CkInsightsAnalyzer

**Purpose:** UI-free Unreal Insights trace analysis, reporting, and commandlet support. The editor tab lives in CkGameplayDebugger's `CkInsightsDebugger` module.

**Depends on:** `CkCore`, `CkLog`, JSON, and Unreal Trace analysis/services.
**Used by:** `CkInsightsDebugger` and developer commandlet workflows; not referenced by gameplay modules.

---

## Key API

- No `_Utils.h`. Exposes C++ trace-analysis/report types plus `UCkInsightsAnalyzerCommandlet`.
- `FCk_FrameReport::BuildHotPathTree / ComputeCategorySummary / ComputeTopTimers` —
  structured (non-markdown) analysis data backing debugger and commandlet consumers.
- `FCk_MultiFrameReport::AnalyzeFrameSet(Session, TArray<FCk_FrameRun>)` — multi-frame selection over
  ascending, non-overlapping, inclusive runs; `AnalyzeAndGenerate` / `AnalyzeWorstFrames` funnel
  through the same worker. Pure statics `DoIs_ValidRunSelection / DoGet_FrameIndices /
  DoGet_SelectedFrameCount / DoGet_FrameRunsLabel` are the spec-pinned selection primitives.
- `FCk_MultiFrameStats::SelectedRuns / AveragedFrame / WaitAverages` — the selection, a synthetic
  mean frame that the category and top-timer panels can consume directly, and one of the two panels
  that must be aggregated instead of derived (see below).
- `FCk_MultiFrameStats::AnalysedFrameIndices / MergedHotPaths` — the frames actually analysed
  (the ordinal space every per-frame series is indexed by) and the per-frame hot-path trees merged
  into one, behind `FCk_MultiFrameReportConfig::BuildMergedHotPaths`. `FCk_MergedHotPathNode` carries
  per-node presence, hit-average, p95/max over present samples, and the per-frame magnitude series.
  `FCk_MultiFrameReport::DoMerge_HotPathTrees` is the pure, spec-pinned reduction behind it.
- `FCk_FrameReport::BuildHotPathTree(Session, Result, TimerNames)` — the overload for a caller that
  already holds a read scope and the name map, mirroring the `ComputeWaitSummaries` pair.

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
  reports stay comparable. Same reason the selection line is emitted only for a DISJOINT
  selection: a contiguous report stays byte-identical to every one generated before multi-frame
  selection existed.

---

## Implementation notes — multi-frame selection (`FCk_MultiFrameReport`)

- **A run selection is admitted whole or not at all.** `DoIs_ValidRunSelection` rejects the entire
  set on any malformation; analysing the salvageable subset would report averages over frames
  nobody asked for, under the label of the selection that was requested.
- **`AveragedFrame` is a mean of quantities, not a frame.** Its per-timer maps merge linearly, so
  `ComputeCategorySummary` and `ComputeTopTimers` are correct on it. Its
  `FrameStartTime`/`FrameEndTime`/`Events` are placeholders, which is what
  `FCk_FrameAnalysisResult::IsSynthesizedAverage` marks.
- **The wait/worker panels cannot be derived from it.** `ComputeWaitSummaries` and
  `ComputeWorkerThreadSummaries` re-read the session over the frame's real time window per thread;
  handed a synthesized average they would silently report the start of the trace, so they ensure
  and return empty instead. The averaged wait panel is aggregated in the worker into
  `FCk_MultiFrameStats::WaitAverages`, behind `FCk_MultiFrameReportConfig::ComputeWaitAverages`
  (off by default — it costs one full per-thread traversal per analysed frame).
- **Neither can the hot-path tree — for a different reason.** It is a threshold problem, not a
  time-window one. Every averaged `ChildrenOf` edge is divided by ALL analysed frames, while
  `UnwrapRoots` and `BuildHotPathTree` apply ABSOLUTE floors (0.5ms, `MinInclusiveMs`), so a path
  present in a minority of frames sinks under the floor and the tree comes back empty — silently, an
  empty root list rather than an error. The merged edge set is also a union of every frame's edges,
  which is a graph, not a tree. So the tree is built PER FRAME and the trees are merged
  (`DoMerge_HotPathTrees`) into `FCk_MultiFrameStats::MergedHotPaths`, keyed by (RawName,
  Breadcrumbs) within a parent. `AveragedFrame->ChildrenOf` is consequently no longer populated and
  `BuildHotPathTree` ensures on a synthesized result.
- **Presence is what the merge adds.** A node's averages divide by every analysed frame (absent
  contributes zero, consistent with `TimerAverages`), while `FramesPresent`, `HitAvgInclusiveMs`, and
  the ordinal-indexed `PerFrameInclusiveMs` separate "expensive on three frames" from "mildly
  expensive on all forty". Absent ordinals are negative, never zero: zero is a legitimate magnitude.

---

## See also

- `CkProfile/Claude.md` — stat groups captured by Insights.
- `../../../CkGameplayDebugger/Source/CkInsightsDebugger/CLAUDE.md` — editor-tab ownership and UI contracts.
