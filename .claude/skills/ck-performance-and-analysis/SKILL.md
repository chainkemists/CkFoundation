---
name: ck-performance-and-analysis
description: "Use when profiling, benchmarking, or making Ck performance claims with stats, Unreal Insights, A/B tests, scheduler pumps, signals, or memory; not for functional bugs."
---

# ck-performance-and-analysis

## Overview

The framework's rule is **measure, don't eyeball** (root doctrine non-negotiable #7: no performance
claims without a benchmark). This skill maps every measurement entry point that already exists in
the code — most of the instrumentation is free because the processor and signal templates emit it
automatically — then makes "a valid perf claim" operational: baseline, N≥3 runs, one axis per
change, numbers with units.

Jargon used below (full lingo table: root `Plugins/CkFoundation/CLAUDE.md`): a **processor** is an
ECS system iterating entities via `ForEachEntity`; a **fragment** is an ECS component; a **signal**
is the fragment-based event system; **AS** = AngelScript.

## When NOT to use this skill

| Situation | Load instead |
|---|---|
| Behavior is wrong / crashes / test fails | `ck-debugging-playbook` |
| Deciding whether a feature/optimization is worth building | `ck-feature-frontier` |
| Writing/running the tests themselves (harness mechanics) | `ck-tests-authoring-and-running` (CkTests) |
| ECS lifetime/GC/storage theory behind a measurement | `ckecs-domain-reference` |
| Gating/landing the optimization change | `ck-change-control` |

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Measurement entry points | `references/measurement-entry-points.md` |
| Interpretation guide — reading what you measured | `references/interpretation-guide.md` |

## 2. The benchmark-before-claiming discipline

A performance claim is valid only when it states ALL of:

1. **Config + environment**: build config, editor-vs-game, PIE vs standalone vs `-nullrhi`, machine.
2. **Baseline captured first**, same content/scenario, before the change.
3. **N≥3 runs** (or explicitly "reproduced across N runs") — one run is an anecdote.
4. **Numbers with units and spread** — avg AND max ms (or fps), not adjectives.
5. **One variable** between A and B.

"~2x faster" without those five is a review rejection under root non-negotiable #7.

### 2.1 The A/B harness exemplar — clone this

`Plugins/CkTests/Script/CkIskmRenderer/CkAutoTest_IskmRenderer_BatchedPerf.as` (commit `b89f110`,
2026-07-02) is the house benchmark template. One file, two paired autotests forming the A/B:

- **A** `UCk_AutoTest_IskmRenderer_BatchedPerf` — 600 moving batched renderer members (worst-case
  per-frame write path). **B** `UCk_AutoTest_IskmRenderer_SkmcPerf` — 150 moving per-SKMC proxies.
- Shape of each: `DoBeginPlay` uncaps the frame rate (`t.MaxFPS 0`, `r.VSync 0`), spawns the
  scenario via the same EntityScript the gyms use, then a tick timer **warms up 3 s** and **samples
  per-tick delta for 6 s**, accumulating sum/max/count.
- Report = one log line, always this shape:
  `[CkIskm PERF][Batched moving 600] frames=N avg=X ms  max=Y ms  fps=Z`.
- **No pass/fail timing threshold — deliberately.** Numbers are machine-dependent; the test fails
  only on setup failure. The claim lives in comparing the two logs from the SAME machine/run, and
  normalizing per instance (600 vs 150). A green run is NOT a perf claim by itself.
- Both tests hand-author their `ACk_AutoTestRunner` wrapper with `default _TimeoutSeconds = 45.0f`
  (warmup+sample needs more than the harness default 5 s); the wrapper generator skips classes that
  already have a hand-authored wrapper of the conventional name
  (`Source/CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.cpp:497-507`).

What an honest result readout looks like (that commit's message, measured, two runs): batched 600 ≈
6.0-6.2 ms avg (~162-166 fps) vs per-SKMC 150 ≈ 8.4-9.2 ms avg (~109-119 fps) — 4x instances at
~2/3 frame time, with the stated caveat that the harness floor inflates the batched per-instance
figure. Numbers + variance + caveat: copy that reporting style.

The sibling `CkAutoTest_IskmRenderer_BatchedVisual.as` is the visual-evidence pattern: spawn the
scenario, `viewmode unlit`, aim the controller, two `HighResShot 1280x720` captures ~1.2 s apart to
`Saved/Screenshots/`, pass on capture completion — **image judgement is explicitly the operator's**,
requires real RHI (under `-nullrhi` it passes but writes nothing).

To clone for a new hypothesis: copy the file into `Script/<FeatureModule>/`, rename per
`CkAutoTest_<Feature>_<Scenario>` with Scenario naming what is MEASURED, swap the spawn block, keep
warmup/sample/uncap/log-shape verbatim, keep the hand-authored wrapper + generous timeout. Harness
mechanics, run commands, and evidence rules: load `ck-tests-authoring-and-running`.

`[EDITOR-VERIFY]` Running the pair: Session Frontend → Automation tab → refresh → check both rows
(`Project.Functional Tests.<map>.<class-minus-_Actor>`) → Start Tests → pull both
`[CkIskm PERF]` lines from the log. Run ≥3 times; quote all runs or the spread.

### 2.2 When you (an agent) cannot run it — write the benchmark request

Agents cannot launch the editor or PIE. Do not guess; hand the human an executable request:

```
BENCHMARK REQUEST — <one-line hypothesis>
Scenario:   <map / autotest name / spawn count / what must be moving>
Config:     Development -game (NOT editor PIE), t.MaxFPS 0, r.VSync 0
Command:    <exact run line or Session Frontend row name>
Collect:    <the exact log-line tag to grep, or stat group to screenshot>
Runs:       3 minimum, cold-start each
Report back: avg ms, max ms, fps per run + machine name/config
```

---

## 4. Optimization protocol

1. **Measure** — capture the baseline with one of §1's tools; record config, scenario, N runs, the
   numbers. No baseline ⇒ you cannot claim the delta later.
2. **Hypothesize** — name the mechanism ("tombstone-bloated storage walk", "listener X inside
   OnTimerUpdate"), not just the symptom.
3. **Change ONE axis.**
4. **Re-measure on the SAME harness** — same test, same config, same machine, same run count.
5. **Report the pair**: `baseline avg 8.4 ms (max 11.2) → now 6.1 ms (max 7.9), N=3 each,
   Development -game, <machine>`. Both numbers, always — a lone "after" number is unverifiable.

No optimization lands without its measurement pair — that is a merge gate; classification and
done-criteria: load `ck-change-control`. When a hypothesis is ruled out by a measurement, log it in
the task notes so it is not re-litigated.

## Common mistakes

- Quoting adjectives ("way faster", "negligible") where §2's five-part claim is required.
- Comparing across configs/machines/editor-vs-game, or PIE vs standalone, and calling it a delta.
- Creating a dynamic stat id (string format + FName) per call in a hot loop instead of caching a
  `TStatId` (§1.3) — the profiler becomes the hotspot.
- Declaring a new top-level stat group for feature work — use `STATGROUP_CkProcessors_Details` or
  your module's existing group (`Source/CkProfile/Claude.md` anti-pattern #1).
- Copying `ck::ScopedStat` by value in AS (it is non-copyable for a reason — a copy would record a
  bogus second sample); the idiom is `auto _S = ck::ScopedStat();` as a local.
- Planning around `CK_ENABLE_MEMORY_TRACKING` — it is off in every config and its enabled path does
  not compile (§1.8).
- Treating a green perf autotest, a completed toolbox notification, or a single run as evidence.

## Provenance and maintenance

Written 2026-07-02 against CkFoundation working tree (superproject `d:\Repos\BusterBlock`), CkTests
detached HEAD `b89f110`, engine UnrealEngine-Angelscript 5.7.4 at `D:/Repos/UnrealEngineAngelscript`.
The Grep tool is blind under `Plugins/*/Script` (superproject `.ignore`) — use Bash `rg --no-ignore`.
Re-verify volatile facts (Git Bash, cwd `d:\Repos\BusterBlock`):

- Stat group census (42 on 2026-07-02):
  `rg --no-ignore -n "DECLARE_STATS_GROUP" Plugins/CkFoundation/Source -g '*.{h,cpp}'`
- Processor auto-stats + CK_STAT sites:
  `rg -n "CK_DEFINE_STAT|CK_STAT\(" Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h`
- Timer dynamic-stat pattern: `rg -n "MakeStatIdFromParams" Plugins/CkFoundation/Source/CkTimer`
- Define matrix rows: `rg -n "CK_ENABLE_MEMORY_TRACKING|CK_DISABLE_STAT_DESCRIPTION" Plugins/CkFoundation/Source/CkBuildConfig/CkBuildConfig.Build.cs`
- `BytesAllocated` still undefined (expect 1 hit = the use site):
  `rg --no-ignore -n "BytesAllocated" Plugins/CkFoundation/Source`
- Commandlet flags: `rg -n "ParsedParams.Find|Switches.Contains" Plugins/CkFoundation/Source/CkInsightsAnalyzer/Public/CkInsightsAnalyzer/Commandlet/CkInsightsAnalyzerCommandlet.cpp`
- Perf A/B exemplar present:
  `rg --no-ignore -n "CkIskm PERF" Plugins/CkTests/Script/CkIskmRenderer/CkAutoTest_IskmRenderer_BatchedPerf.as`
- Cvars: `rg -n "ck.Scheduler.DebugTiming|ck.Signal.StatListeners" Plugins/CkFoundation/Source/CkEcs`
- Engine facts (STATS matrix, stat-command matching, trace flags):
  `rg -n "define STATS" "D:/Repos/UnrealEngineAngelscript/Engine/Source/Runtime/Core/Public/Misc/Build.h"`;
  `rg -n 'STATGROUP_' "D:/Repos/UnrealEngineAngelscript/Engine/Source/Runtime/Core/Private/Stats/StatsCommand.cpp" | rg 958`;
  `rg -n '"-trace="|"-tracefile="' "D:/Repos/UnrealEngineAngelscript/Engine/Source/Runtime/Core/Private/ProfilingDebugging/TraceAuxiliary.cpp"`
