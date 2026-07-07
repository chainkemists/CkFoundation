# CkInsightsAnalyzer

Parses Unreal Insights `.utrace` files and generates markdown performance reports with hot-path trees, category summaries, and per-thread breakdowns. A JSON mode emits the same data machine-readable for scripts, CI gates, and AI consumption.

## Key Concepts

- **TraceSession** — Opens a `.utrace` file, exposes frame/timing/thread data for analysis.
- **FrameAnalyzer** — Extracts timing events from frames, builds parent-child call graphs, computes inclusive/exclusive times.
- **TimerCategorizer** — Maps timer names to categories (Physics, Render, AI, etc.) and simplifies verbose function names.
- **FrameReport** — Generates Slack-friendly markdown with hot-path trees and category summaries. Configurable depth: Full (8), Standard (5), Concise (3), HotPathsOnly.
- **JsonReport** — Generates machine-readable JSON from the same analysis results (see JSON Reports below).
- **Wrapper Collapsing** — Automatically unwraps thin wrapper functions to show the real work underneath.

## Example: Analyzing a Performance Capture

```mermaid
flowchart LR
    A["Capture .utrace<br/>in editor"] -->|"Open TraceSession"| B["FrameAnalyzer<br/>builds call graph"]
    B -->|"Generate report"| C["Markdown with<br/>hot paths + categories"]
```

## Usage Examples

### Open a trace and analyze

```cpp
auto Session = FCk_TraceSession::Open(TraceFilePath);
auto FrameData = FCk_FrameAnalyzer::AnalyzeFrame(Session, FrameIndex);
```

### Generate a markdown report

```cpp
auto Report = FCk_FrameReport::Generate(FrameData, ReportConfig);
// Report is a markdown string ready for Slack/docs
```

### Categorize timers

```cpp
auto Category = FCk_TimerCategorizer::Categorize(TimerName);
auto SimpleName = FCk_TimerCategorizer::SimplifyName(TimerName);
```

## JSON Reports

The headless commandlet accepts `-json` to emit JSON instead of Slack markdown:

```
UnrealEditor-Cmd.exe <Project.uproject> -run=CkInsightsAnalyzer -trace=session.utrace -all -json -output=report.json
```

Conventions: times in milliseconds (3-decimal precision), camelCase keys, empty sections omitted.
Every report carries a `trace` overview (file, duration, game/render frame counts, thread list)
and `budgetMs`. Single-frame mode (`-frame=N`) adds `singleFrame`; range/worst/all modes add
`multiFrame`:

```json
{
  "trace": { "file": "...", "durationSeconds": 0.0, "gameFrameCount": 0, "renderFrameCount": 0,
             "threads": [ { "id": 0, "name": "GameThread" } ] },
  "budgetMs": 16.67,
  "singleFrame": {
    "frameIndex": 0, "durationMs": 0.0,
    "callTree":   [ { "name": "...", "inclusiveMs": 0.0, "exclusiveMs": 0.0, "count": 0, "children": [] } ],
    "topTimers":  [ { "name": "...", "exclusiveMs": 0.0, "inclusiveMs": 0.0, "count": 0 } ],
    "categories": [ { "name": "...", "exclusiveMs": 0.0, "pctOfFrame": 0.0 } ],
    "workerThreads": [ { "id": 0, "name": "...", "wallTimeMs": 0.0, "eventCount": 0, "topTimers": [] } ]
  },
  "multiFrame": {
    "frameCount": 0, "avgMs": 0.0, "minMs": 0.0, "maxMs": 0.0, "p95Ms": 0.0, "p99Ms": 0.0,
    "worstFrames": [ { "frameIndex": 0, "durationMs": 0.0, "dominantCost": "...", "dominantCostMs": 0.0 } ],
    "categoryAverages": [ { "name": "...", "avgExclusiveMs": 0.0, "p95ExclusiveMs": 0.0, "pctOfTotal": 0.0 } ]
  }
}
```

Unlike the markdown hot-path tree, `callTree` is the **true aggregated call tree** built from raw
timing events (merged by call path, raw timer names, no wrapper collapsing), pruned below the
config's `MinInclusiveMs` — pruned children still count toward their parent's totals. The one
naming exception: `workerThreads[].topTimers` reuses the shared summary computation and carries
simplified names, matching the markdown report.

## Tests

No tests found for this module in CkTest.
