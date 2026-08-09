# Phase 7 — the debugger: scan diagnostics + timeline/layer/state views

> **Status:** 🟡 OPEN (2026-08-09). **Depends on:** Phases 3-6 (✅ — every surface it
> renders). **Gate regime:** scoped per [P2-D4]; the visual layer is `[EDITOR-VERIFY]`
> (success criterion 5's scrub is human by definition).
> **Scope of record:** PROMPT.md phase-index row 7: extend `SCkDebug_EventTimeline`,
> near-miss display, direction rosette, key-state list, resolution-table view, layer-stack
> view ("why did nothing happen when I pressed X").

## Rulings at phase open

- **[P7-D1] The debugger renders RECORDED FACTS, never recomputes.** Every view reads the
  frame record, the retention rows, the matcher's phase rows, and (new) the scan
  diagnostics — the dossier's B-debugger argument, now doctrine: a poll bug shows up as a
  divergence between record and behavior, not as a shared blind spot.
- **[P7-D2] Near-miss needs data the matcher does not yet produce.** A failed backward
  scan currently leaves no trace. Unit 7-1 adds an OPT-IN scan-diagnostic ring on the
  matcher (last N failed/succeeded scans: intent, terminal frame, per-step outcome —
  matched-at-frame / failed-at-step with the frames examined), toggled by
  `ck.Intent.RecordScanDiagnostics` (CVar, default OFF — zero cost when off; the ring is
  small, ~32 entries). Success criterion 5's "which step timed out and by how many frames"
  is answered from this ring.
- **[P7-D3] The UI rides the existing CkGameplayDebugger category framework** —
  `CkIntentDebugger` module mimicking `CkGoapDebugger`'s shape (category + MVVM +
  `SCkDebug_EventTimeline` reuse for lanes). Views v1: timeline (per-layer spans +
  per-intent phase spans + blocked-by annotations), layer-stack tree (stack order,
  captures, matcher + active-set summary per layer), key/state list (held set, conditioned
  axes, octant, SOCD outputs), resolution-table view (active set's tables + verdicts),
  near-miss list (the [P7-D2] ring, newest first). Direction rosette = part of the
  key/state view (a simple octant dial), not its own view.

## Units (sequential)

**7-1 (CkIntent):** the scan-diagnostic ring + CVar + read API (`Get_ScanDiagnostics`),
recorded at the scan sites in the Match processor; one AutoTest (diagnostics off by
default = ring stays empty; on = a failed 236 scan reports the step that had no matching
octant row and the frames examined; a succeeded scan reports matched-at frames).

**7-2 (CkGameplayDebugger):** the `CkIntentDebugger` module per [P7-D3]. Executor must
load the `ck-gameplaydebugger-extension` and `ck-slate-tools` skills BEFORE writing Slate.
Gate = compile + existing suite green; every view's visual behavior lands on the
`[EDITOR-VERIFY]` queue with exact click steps.

## Exit criteria

- [ ] Scoped gate green (`Ck_AutoTest_In` + the 7-1 test); debugger module compiles in the
      full build
- [ ] `[EDITOR-VERIFY]` queue entry written (exact steps per view, incl. criterion 5's
      scrub scenario)
- [ ] `CkIntent/Claude.md` + the debugger module's `CLAUDE.md` document the surfaces
- [ ] PROGRESS current; comment audit
- [ ] Full suite NOT run ([P2-D4])

## NOT in this phase

No gyms (8), no new signal kinds, no replication, no editor-time authoring tools, no
recompute-based views ([P7-D1]).
