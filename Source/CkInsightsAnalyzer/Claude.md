# CkInsightsAnalyzer

**Purpose:** Unreal Insights integration — tooling for analyzing capture files and correlating them with CkFoundation processor timings. Editor-only analysis tool.

**Depends on:** `CkCore`, `CkLog`.
**Used by:** Developer workflow only; not referenced by gameplay modules.

---

## Key API

- No `_Utils.h`. Exposes editor commands and subsystem for trace analysis.

---

## Pattern

Run via editor menu or console command during a profiling session.

---

## Anti-patterns

Don't ship with Insights integration enabled.

---

## See also

- `CkProfile/Claude.md` — stat groups captured by Insights.
