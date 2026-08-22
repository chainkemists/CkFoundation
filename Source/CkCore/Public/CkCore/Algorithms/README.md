# CkCore / Algorithms

Small header-only helpers in `ck::algo` — `Overload` for inline `std::visit`, `ToTransform` for range adapters, `BasicDereference`, plus `IsValid`/`Is_NOT_Valid` as functors (defined in `Validation/` but exposed here).

**Key files:** `CkAlgorithms.h`, `CkAlgorithms.inl.h`

## Summary statistics

| Call | Returns |
|---|---|
| `Mean(container)` | arithmetic mean |
| `Median(container)` | 50th percentile |
| `Percentile(container, fraction)` | linear-interpolated percentile; `fraction` in `[0, 1]`, clamped |
| `MedianAbsoluteDeviation(container)` | median of the absolute deviations from the median — a spread estimator outliers do not drag around, which is what makes it usable for *finding* outliers |
| `MeanAbsoluteDeviation(container)` | mean of the same deviations — less robust, but it does not collapse when most values agree |

**Choosing between the two deviation measures.** `MedianAbsoluteDeviation` returns **zero whenever
more than half the values are identical**, which is common in tightly clustered data — and taken
literally as an outlier threshold, that switches detection off precisely when the data is cleanest.
Prefer it, but fall back to `MeanAbsoluteDeviation` when it returns zero; only when both are zero is
a set genuinely without spread.

All four take any numeric container by const reference, copy and sort internally (the caller's
container is never reordered), and return `TOptional<double>`. **An empty container yields an unset
optional, not a zero** — zero is a legitimate statistic and "nothing to measure" is not, so the
distinction lives in the type rather than in a comment.

For the full use-case table and how this folder fits with the rest of CkCore, see [`../../Claude.md`](../../Claude.md).
