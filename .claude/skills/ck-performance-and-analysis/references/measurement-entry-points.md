# Measurement entry points

Reference for `ck-performance-and-analysis`: what instrumentation exists, where it lives, and when to reach for each — stats, Unreal Insights, scheduler pumps, signal counters, memory.

## 1. Measurement entry points — what exists, where, when to reach for it

| Tool | What it gives | Cost / availability | Reach for it when |
|---|---|---|---|
| `stat Ck*` console commands | Live per-frame cycle counters, named per processor / signal / scope | Free; needs `STATS` builds (Debug/Development; NOT Test/Shipping — see §1.6) | First look: "which Ck system is hot this frame?" |
| Unreal Insights (`-trace=cpu,frame`) | Full timeline `.utrace` capture, works even without `STATS` | Trace overhead while recording | Frame spikes, ordering, cross-thread, anything you want to keep as an artifact |
| CkInsightsAnalyzer | Automated hot-path/category markdown reports FROM a `.utrace` | UncookedOnly module (editor/commandlet only) | Turning a capture into a shareable, diffable report — or letting an agent read a trace |
| Perf A/B autotests | Repeatable scripted scenario logging avg/max ms + fps | One PIE run each | A/B claims: "batched vs per-instance", before/after an optimization |
| `UCk_Utils_Stats_UE` / `utils_stats` | FPS, frame ms, RAM/VRAM, ping as plain numbers in BP/AS | Free | In-test sampling, debug overlays |
| Memory snapshot subsystem | OS-level physical/virtual/RHI GB, 1 s poll | `STATS` builds | Coarse leak detection across a scenario |

### 1.1 Per-processor cycle stats (already instrumented — you get these for free)

Every processor deriving `ck::TProcessor` / `ck_exp::TProcessor` auto-defines two cycle stats named
after the processor type itself:

- `CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors)` and
  `STAT_ForEachEntity` in `STATGROUP_CkProcessors_Details` —
  `Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h:89-90` (`ck_exp` variant :269-270).
- `CK_STAT(STAT_Tick)` wraps the whole view iteration (`CkProcessor.h:209`); `CK_STAT(STAT_ForEachEntity)`
  wraps each per-entity body call (`CkProcessor.h:218`).
- The stat NAME is derived from the C++ type at compile time via `cleantype::clean<T>()` with
  template noise stripped (`Source/CkProfile/Public/CkProfile/Stats/CkStats.inl.h:17-24`) — so the
  row you see in `stat CkProcessors` literally reads `ck::FProcessor_Timer_Update`.
- Parallel processors additionally get phase-suffixed stats — ` [CollectEntities]`,
  ` [ParallelDispatch]`, ` [FlushCommands]`, ` [ForEachEntity]` — via `CK_DEFINE_PHASE_STAT`
  (`Source/CkEcs/Public/CkEcs/Processor/CkParallelProcessor.h:36-40`).

`CK_STAT` compiles to `FScopeCycleCounter` under `STATS` and to a named CPU event
(`SCOPED_NAMED_EVENT_TCHAR`) otherwise (`Source/CkProfile/Public/CkProfile/Stats/CkStats.h:89-90,
109-110`) — **your scopes still show on the Insights timeline in non-STATS builds; only the `stat`
console readout dies.**

To add a finer scope inside one processor, use the standard UE macros against the existing groups
(guidance: `Source/CkProfile/Claude.md` — don't invent new top-level groups):

```cpp
#include "CkProfile/Stats/CkStats.h"

DECLARE_CYCLE_STAT(TEXT("MyProcessor::ResolvePhase"),
                   STAT_MyProcessor_ResolvePhase,
                   STATGROUP_CkProcessors_Details);

auto
    FProcessor_MyFeature_Update::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_MyFeature_Current& InCurrent)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_MyProcessor_ResolvePhase);
    // ...
}
```

### 1.2 The real Ck stat groups (console: `stat <name>`)

`stat <name>` matches by prepending `STATGROUP_` (engine `StatsCommand.cpp:958`), so the command is
the enum suffix, NOT the `TEXT("...")` display string. Infrastructure groups, all verified
2026-07-02:

| Command | Declared at | Shows |
|---|---|---|
| `stat CkProcessors` | `CkEcs/Processor/CkProcessor.h:19` | Per-processor whole-Tick cost (display header "Tick") |
| `stat CkProcessors_Details` | `CkProcessor.h:18` | Per-processor ForEachEntity body + parallel phases |
| `stat CkScheduler` | `CkEcs/Scheduler/CkProcessorScheduler.cpp` | Scheduler orchestration: `MainPass`, `Dispatch`, `Pump`, `PumpDispatch`, `PumpDirtyCheck`, `DebugRecord` — processor stats nest INSIDE these, so their self-time is pure scheduler overhead (`ResetPumpVersions` removed 2026-07-06: the pump version cache now persists across frames) |
| `stat CkEcsWorldActor_Tick` | `CkEcs/Subsystem/CkEcsWorld_Subsystem.cpp:22,65` | One dynamic stat per ECS world actor, named `[<TickGroup>] EcsScheduler_Actor` — the whole-ECS top-line per ticking group |
| `stat CkEcs` | `CkEcs/CkEcs_Stats.h:7` + `CkEntityLifetime_Utils.cpp:17-22` | Entity destroy/world-lookup cycle stats + `Ecs Entities Spawned/Destroyed` frame counters |
| `stat CkSignals` | `CkEcs/Signal/CkSignal_Utils.h:15` | Per-signal-TYPE broadcast cost — wraps the ENTIRE publish including every listener's execution |
| `stat CkSignals_Listeners` | `CkEcs/Signal/CkSignal_Fragment.h:16` | Per-LISTENER cost, named `ClassName::FunctionName`. Opt-in: `ck.Signal.StatListeners 1` (default off; `CkSignal_Fragment.cpp:9-15`) |
| `stat CkScript` | `CkProfile/Stats/CkProfile_Stats.h:10` | AngelScript `ck::ScopedStat` scopes (§1.4) |
| `stat CkTimer_Details` | `CkTimer/CkTimer_Utils.cpp:25` | Per-timer-NAME broadcast cost (§1.3) |

Feature modules additionally declare per-module groups (`stat CkCrowd`, `stat CkGoap`,
`stat CkIskmRenderer`, …) in their `<Module>_Stats.h`. 42 groups total on 2026-07-02. Enumerate
them all (Git Bash, cwd `d:\Repos\BusterBlock`):

```bash
rg --no-ignore -n "DECLARE_STATS_GROUP" Plugins/CkFoundation/Source -g '*.{h,cpp}'
```

`[EDITOR-VERIFY]` Reading them: PIE → `` ` `` console → `stat CkProcessors` → observe the
per-processor rows (InclusiveAvg is the number you quote). `stat unit` first for the GT/RT/GPU
split; a Ck stat only matters if Game thread is the bound one.

### 1.3 Per-INSTANCE dynamic stat ids (the Timer pattern) — and its cost caveat

Type-derived stats aggregate all instances. When you need cost split per instance (per timer name,
per listener), create a dynamic `TStatId` at runtime:

```cpp
// CkTimer/CkTimer_Utils.cpp:29-36
auto
    ck::MakeStatIdFromParams(
        const FCk_Timer_Spec& InParams)
    -> TStatId
{
    const auto& StatString = ck::Format_UE(TEXT("Timer Broadcast Event [{}]"), InParams.Get_TimerName());
    return CK_CREATE_DYNAMIC_STAT_ID(STATGROUP_CkTimer_Details, StatString);
}

// Consumed at every signal broadcast site, always under the STATS guard
// (CkTimer/CkTimer_Processor.cpp:88-93 — the canonical usage):
{
#if STATS
    auto TimerStatCounter = FScopeCycleCounter{MakeStatIdFromParams(InTimerEntity.Get<FFragment_Timer_Params>())};
#endif // STATS
    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
}
```

Two costs to respect:

1. **Per-call cost:** each call is a string format + FName registration + dynamic-stat lookup.
   Timer pays it per broadcast, deliberately uncached — caching the TStatId as a fragment created a
   snapshot-restore gap (restored timers lost it; rationale comment `CkTimer_Utils.cpp:61-65`). The
   `#if STATS` guard means the cost exists only in Debug/Development.
2. **If you call one in a genuinely hot loop, cache the TStatId** — the pattern is
   `ck::Get_ScopedStat_StatId` (`CkProfile/Stats/CkScopedStat.cpp:19-41`): a per-thread
   `TMap<FName, TStatId>` in front of `CK_CREATE_DYNAMIC_STAT_ID`, precisely to avoid re-registering
   the FName every scope entry.

`CK_CREATE_DYNAMIC_STAT_ID` itself: `CkProfile/Stats/CkStats.h:84-85` (returns empty `TStatId{}`
without STATS, :104-105).

### 1.4 Scoped stats from all three environments

- **C++:** `SCOPE_CYCLE_COUNTER(STAT_X)` / `CK_STAT(STAT_X)` as in §1.1.
- **Blueprint:** there is no BP profiling-scope macro. The BP-facing surface is the read-only
  numbers library: `[Ck] Get FPS`, `[Ck] Get Frame Time (ms)`, `[Ck] Get Frame Count`,
  `[Ck] Get RAM Used (GB)`, `[Ck] Get VRAM Used (GB) [DEV]` (STATS-only, returns 0 in Shipping),
  `[Ck] Get Build Config` — all BlueprintPure on `UCk_Utils_Stats_UE`, category
  `Ck|Utils|Profile|Stats` (`CkProfile/Stats/CkStats_Utils.h`).
- **AngelScript:** `ck::ScopedStat` — RAII value type, records to `stat CkScript` on scope exit:

```angelscript
{
    auto _Stat = ck::ScopedStat();                       // auto-named "<Class>::<Method>"
    // ...work to measure...
}
auto _Sub = ck::ScopedStat("AI::EvaluateGoals::Phase2"); // explicit name — prefer in hot loops
                                                         // (skips the per-call context lookup)
utils_stats::Get_FPS();                                  // generated numbers accessor
```

The no-arg form derives the name from the active script context; non-copyable so it records exactly
once. Full contract: `Source/CkProfile/Claude.md`. Pinning test (also your reference for asserting
scope names): `Plugins/CkTests/Script/CkProfile/CkAutoTest_Profile_ScopedStat.as`. Many CkTests
autotests open with `auto _CkPerfScope = ck::ScopedStat();` — free per-test timing under
`stat CkScript`.

### 1.5 Unreal Insights — capture and read

Generic capture shapes (PowerShell; any config, works WITHOUT the STATS system):

```powershell
# Editor/PIE or -game process — trace from boot:
<Editor>.exe <Project>.uproject -game -trace=cpu,frame -tracefile=MyCapture.utrace

# Mid-session from the console instead: Trace.Start cpu,frame  ...  Trace.Stop
```

- `-trace=<channels>` / `-tracefile=<path>` parsing: engine `TraceAuxiliary.cpp:1714,1748`; console
  commands `Trace.Start`, `Trace.File`, `Trace.Stop`, `Trace.Pause`, `Trace.Send` registered at
  `TraceAuxiliary.cpp:1572-1608`.
- **Where the file lands:** a relative `-tracefile` path resolves under `FPaths::ProfilingDir()` =
  `<Project>/Saved/Profiling/` (engine `Paths.cpp:559-562`, `TraceAuxiliary.cpp:845-855`); the
  default name is a `%Y%m%d_%H%M%S_*.utrace` timestamp. Without `-tracefile`, the trace goes to the
  local Unreal Trace Server store (INFERRED default store path `%LOCALAPPDATA%\UnrealEngine\Common\
  UnrealTrace\Store`; confirm by opening Insights' session browser).
- `cpu` channel carries the processor/signal scopes (§1.1 named events); `frame` carries frame
  boundaries — CkInsightsAnalyzer needs both.

`[EDITOR-VERIFY]` Reading a capture in the Insights UI: launch `UnrealInsights.exe` (Engine/Binaries/Win64)
or editor Tools → Run Unreal Insights; open the `.utrace`; Timing view → find the GameThread track;
Ck processor scopes appear under the EcsScheduler actor tick, named by processor type.

### 1.6 When each measurement exists — the define matrix

| Fact | Where pinned |
|---|---|
| `STATS` is ON in Debug and Development (editor and game), OFF in Test and Shipping unless `FORCE_USE_STATS` | engine `Misc/Build.h:258,286,314,342` |
| No `STATS` ⇒ `stat Ck*` commands dead, dynamic stat ids return `TStatId{}`, `FScopeCycleCounter` gone — but `CK_STAT`/`ScopedStat` still emit named events for Insights | `CkStats.h:104-110`, `CkScopedStat.cpp` non-STATS branch |
| `CK_DISABLE_STAT_DESCRIPTION=1` in Test/Shipping (and the "Unknown" config) ⇒ stat descriptions empty; =0 in Debug/Development ⇒ long type-name descriptions for Insights | `CkBuildConfig.Build.cs:79,94,116,129,152,172`; consumed `CkStats.inl.h:35,143` |
| Development-**editor** carries Ck debug overhead Development-**game** does not: `CK_DISABLE_ECS_HANDLE_DEBUGGING=0`, `CK_DISABLE_LOG_CONTEXT=0`, gameplay-tag staleness validation on | `CkBuildConfig.Build.cs:106-132` |

Consequence: **state the config AND editor-vs-game with every number.** A Development-editor PIE
number is not comparable to a Development `-game` number, let alone Test.

### 1.7 CkInsightsAnalyzer — automated trace → report (UncookedOnly)

What it actually is (verified from source, 2026-07-02): a `.utrace` ingester built on the engine's
`TraceAnalysis`/`TraceServices` modules (`CkInsightsAnalyzer.Build.cs`) that produces
markdown performance reports. Registered as `UncookedOnly` (`CkFoundation.uplugin:540-541`) — never
in cooked games.

Pipeline (`Source/CkInsightsAnalyzer/Public/CkInsightsAnalyzer/`):

- `FCk_TraceSession` (`Core/CkTraceSession.h`) — opens the `.utrace`, blocks until analysis
  completes, exposes timing/frame/thread providers; auto-detects the game thread by name.
- `FCk_FrameAnalyzer` (`Core/CkFrameAnalyzer.h`) — per frame (or time range): inclusive ms,
  exclusive (self) ms, call counts, and the parent→child inclusive-time graph per timer.
- `FCk_TimerCategorizer` (`Core/CkTimerCategorizer.h`) — keyword-buckets timer names into categories
  and simplifies verbose UE names.
- `FCk_FrameReport` / `FCk_MultiFrameReport` (`Report/`) — markdown with hot-path trees, category
  summaries, worker threads. Depth presets `Full` (tree depth 8) / `Standard` (5) / `Concise` (3) /
  `HotPathsOnly`; frame budget defaults to 16.67 ms (`Report/CkFrameReport.h`).

Two invocation surfaces:

```powershell
# Headless commandlet (flags verified against CkInsightsAnalyzerCommandlet.h:10-33 and Main()):
UnrealEditor-Cmd.exe <Project>.uproject -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -worst=5
#   -frame=N | -frames=N-M | -worst=N (default 10) | -all      analysis mode
#   -budget=<ms> (default 16.67)  -raw  -top=<N> (default 50)  content knobs
#   -output=<path>  -clipboard                                  report destination (stdout always)
```

`[EDITOR-VERIFY]` Editor tab: registered as nomad tab "Insights Analyzer" in the Developer Tools →
Debug category (`CkInsightsAnalyzer_Module.cpp:37-44`) — expect it under Tools → Debug →
Insights Analyzer; open a `.utrace` from the tab and generate the same reports interactively.

The commandlet is the agent-friendly path: an agent that cannot PIE **can** analyze a trace a human
captured — commandlet runs were not exercised this session, so treat the exact invocation as
[not machine-verified] and read the commandlet's log output on first use.

### 1.8 Memory tracking — what is real and what is vestigial

Real and available today:

- `UCk_Stats_Subsystem_UE` (CkMemory) — background thread polls `FPlatformMemory::GetStats()` + RHI
  memory once per second under `#if STATS`; read via `UCk_Utils_Memory_UE::Get_MemoryCountSnapshot`
  (BlueprintPure; physical/virtual/RHI used/available/total, GB floats). Diff two snapshots across
  a scenario for coarse leak detection (`CkMemory/CkMemory_Subsystem.cpp`).
- `UCk_Utils_Stats_UE::Get_RAM_*` / `Get_VRAM_UsedGB` (§1.4) for spot reads.
- Engine Memory Insights (`-trace=memory`) for real allocation attribution.

Vestigial — do NOT build a plan on it (verified 2026-07-02):

- `CK_ENABLE_MEMORY_TRACKING` is `=0` in EVERY shipped configuration
  (`CkBuildConfig.Build.cs:77,92,114,127,150,170`). It is `=1` only under the `Profile` build
  override — a hand-edit of the `BuildConfigurationOverride` const at `CkBuildConfig.Build.cs:47`
  (which also disables ensures and log context, :190-195).
- Even if enabled, the path is broken: `FProcessor_Memory_Stats` sets an "ECS Memory" stat from
  `ck::detail::BytesAllocated` (`CkMemory/CkMemory_Processor.cpp:23`) — **that symbol is defined
  nowhere in the codebase** (repo-wide search, 1 hit = the use site), and the custom allocator is a
  TODO stub (`CkMemory/Allocator/CkMemoryAllocator.h:14`). Expect a compile error, not data.
  `CkMemory/Claude.md`'s "allocation counting" description oversells; trust this section.

---

