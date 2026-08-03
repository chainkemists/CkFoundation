# Interpretation guide — reading what you measured

Reference for `ck-performance-and-analysis`: turning a captured trace or stat dump into a defensible claim.

## 3. Interpretation guide — reading what you measured

### 3.1 Attributing cost to a processor (inclusive vs self)

- `STAT_Tick` (in `stat CkProcessors`) is INCLUSIVE of everything the processor body calls — most
  importantly **signal broadcasts execute every bound listener synchronously inside the broadcast**
  (the per-signal stat "wraps the entire publish — i.e. every bound listener's execution",
  `CkSignal_Utils.h` comment above `CK_DEFINE_STAT(STAT_SignalBroadcast, ...)`). A "hot processor"
  is often actually hot EntityScript/AS listener code billed to it.
- Drill-down ladder: `stat CkProcessors` (who) → `stat CkSignals` (is it a signal publish) →
  `ck.Signal.StatListeners 1` + `stat CkSignals_Listeners` (WHICH listener, named
  `ClassName::FunctionName`) → `stat CkScript` (which AS scope).
- Scheduler self-time: the per-processor stats nest inside `stat CkScheduler`'s scopes, so
  `Scheduler::MainPass`/`Dispatch` self-time is pure orchestration. In non-Shipping builds a large
  `Dispatch` self-time is usually the scheduler debugger's own 2x `QueryPerformanceCounter` per
  processor — default on; kill it while profiling with `ck.Scheduler.DebugTiming 0`
  (`CkProcessorScheduler.cpp:20-30`).

### 3.2 Dirty-set size vs full-view iteration (`MarkedDirtyBy`)

Mechanics (`CkProcessorScheduler.cpp`, `CkProcessorDescriptor.h:60-115`):

- **Main pass: EVERY processor Ticks once per frame** — dirty markers do NOT gate the main pass
  (comment at `CkProcessorScheduler.cpp:103-106`). An "idle" processor still pays its view setup +
  storage walk each frame; that floor scales with storage size, not live matches.
- **Pump passes** (same-frame drain of cascading reactive work, DeltaT=0) re-invoke ONLY processors
  whose `MarkedDirtyBy` fragment currently has entities, with a version short-circuit to skip the
  rescan (default on via `_EnableDirtyMarkerPumpShortCircuit`, `CkEcs_Settings.h`). Since 2026-07-06
  the per-node version cache PERSISTS across frames (no per-frame reset — idle marker processors
  skip on the version compare alone), and a pump that provably visited zero entities does not count
  as work when scheduling further passes (`Pump()` returns the visited count; -1 = custom DoTick,
  treated as work).
- Perf smell: `Pump limit [N] reached. Still dirty: [...]` warning (`:338`) — a processor that never
  drains its marker is being re-pumped to the per-frame limit, multiplying its measured cost. Fix
  the marker contract (consume it, or `PumpPolicy = SkipPump` for sticky-marker time-steppers), not
  the symptom. Contract details: `ckecs-architecture-contract`.
- Per-processor visited-entity counts: `ck::GDebug_LastProcessedEntityCount` is recorded per
  tick/pump in non-Shipping builds (`CkProcessor.h:229-231`) and surfaced by the Scheduler Debugger
  window (CkGameplayDebugger's `CkSchedulerDebugger` module — load `ck-gameplaydebugger-extension`).
  Cost with a zero entity count = view/storage overhead, not body work.

### 3.3 Fragment iteration density — every fragment is `in_place_delete`

Every fragment pool is tombstone-mode: `CkHandle.h:71-77` globally forces `in_place_delete` — a
deliberate design decision (`06938bba3`, "fragments are always pointer stable"; DECISIONS.md §45),
not debug residue. Consequence for hot loops: removing a fragment leaves a tombstone (no
swap-and-pop compaction — that is what keeps fragment pointers stable across deletes), so a
heavily-churned fragment's storage stays large and iteration cost tracks storage size + tombstone
skipping, not the live count you see in a debugger. Empty types (tags) get `page_size` 0 —
presence-only, no per-entity payload array, so tag checks are cheap. The trait itself, storage
lifecycle, and GC interactions: `ckecs-domain-reference` §1.3; measuring the tombstone cost is
`ck-feature-frontier` candidate 5.

### 3.4 Common false reads

| False read | Reality |
|---|---|
| "Editor PIE says 9 ms, so the game costs 9 ms" | Development-editor compiles in handle debugging, log context, tag-staleness validation that Development-game does not (§1.6) — and PIE adds editor+Slate tick on top. Compare like with like; claim game numbers only from `-game`. |
| "stat shows nothing in my Test build" | `STATS` is compiled out in Test/Shipping (§1.6). Use Insights named events instead. |
| First seconds of a run in the average | Warm-up (shader/PSO, async loads, first-touch allocations) pollutes it — the house harness discards 3 s before sampling (§2.1). |
| "`-nullrhi` run proves rendering is fine" | `-nullrhi` measures game-thread cost only; render submission is absent (stated in the perf test header). Visual/GPU claims need a real RHI run. |
| "The perf autotest is green, so perf is fine" | The house perf tests have NO timing threshold by design — green means "ran and logged". The claim is in the numbers. |
| "This processor got slower" (from one editor session) | `ck.Scheduler.DebugTiming` and `ck.Signal.StatListeners` toggles, open debugger windows, and editor background work all move numbers. Re-run with identical toggles, then A/B. |

---

