# CkInsightsAnalyzer

Parses Unreal Insights `.utrace` files and generates markdown performance reports with hot-path trees, category summaries, and per-thread breakdowns.

## Key Concepts

- **TraceSession** — Opens a `.utrace` file, exposes frame/timing/thread data for analysis.
- **FrameAnalyzer** — Extracts timing events from frames, builds parent-child call graphs, computes inclusive/exclusive times.
- **TimerCategorizer** — Maps timer names to categories (Physics, Render, AI, etc.) and simplifies verbose function names.
- **FrameReport** — Generates Slack-friendly markdown with hot-path trees and category summaries. Configurable depth: Full (8), Standard (5), Concise (3), HotPathsOnly.
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

## Tests

No tests found for this module in CkTest.
