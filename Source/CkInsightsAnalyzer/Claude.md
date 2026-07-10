# CkInsightsAnalyzer

**Purpose:** Unreal Insights integration — tooling for analyzing capture files and correlating them with CkFoundation processor timings. Editor-only analysis tool.

**Depends on:** `CkCore`, `CkEditorTools` (shared `CkStyle::` tokens for the tab UI), `CkLog`.
**Used by:** Developer workflow only; not referenced by gameplay modules.

---

## Key API

- No `_Utils.h`. Exposes editor commands and subsystem for trace analysis.
- `SCkInsightsAnalyzerTab` — the "Insights Analyzer" editor tab (Tools → Debug):
  frame bar chart, structured hot-path tree, category/top-timer panels,
  worst-frames drill-down, stat-tile summary strip. Styled entirely via `CkStyle::`.
- `FCk_FrameReport::BuildHotPathTree / ComputeCategorySummary / ComputeTopTimers` —
  structured (non-markdown) analysis data backing the tab's views.

---

## Pattern

Run via editor menu or console command during a profiling session.

---

## Anti-patterns

Don't ship with Insights integration enabled.

---

## See also

- `CkProfile/Claude.md` — stat groups captured by Insights.
