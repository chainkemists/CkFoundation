# Phase 5 — Debugger inspector (CkGameplayDebugger)

> **Status:** ⏳ Pending
> **Depends on:** Phase 1 ✅ (stats surface); best after Phase 2 for real content

## Goal

After this phase: a CkGameplayDebugger inspector enumerates the ObjectPooling subsystem — one row
per pool (class, archetype, free/in-use/live/high-water/hits/misses/prewarm-remaining) plus the
non-pooled vended count; per-instance drill-down (name, pooled-vs-vended, in-use/free).

## Entry criteria

- [ ] Load `ck-gameplaydebugger-extension` skill (CkGameplayDebugger repo) — do NOT wing the
      extension surface from memory.
- [ ] CkGameplayDebugger submodule on a clean branch off its dev.

## Work items

1. Subsystem exposes an enumeration API for tooling (read-only stats snapshot per pool) — lands in
   CkFoundation if not already shaped that way by Phase 1.
2. Inspector/view registration + table UI per the debugger plugin's existing inspector pattern
   (mimic the nearest existing inspector; name it in PROGRESS.md when chosen).
3. [EDITOR-VERIFY] exact steps: open debugger, spawn poolable scripts in a gym, observe counts move
   on acquire/release.

## Exit criteria

- [ ] Both repos compile; inspector visible and live-updating [EDITOR-VERIFY].
- [ ] PROGRESS.md dated entry; CkGameplayDebugger branch name recorded.
