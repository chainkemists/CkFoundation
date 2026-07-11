# Phase 5 — Debugger inspector (CkGameplayDebugger)

> **Status:** 🟡 Module built + linked (2026-07-11); tab is [EDITOR-VERIFY]
> **Depends on:** Phase 1 ✅ (stats surface); best after Phase 2 for real content
> **Branch:** `feature/object-pooling-inspector` off CkGameplayDebugger dev (37e4066)
> **Approach (chosen):** a dedicated Slate debugger **tab** (Gen-2 pattern), NOT a per-entity
> inspector — the ObjectPooling subsystem is world-level, not per-entity. Mimicked the lean
> `CkInputDebugger` (6-file) template. The per-entity `ICkDebuggerComponentInspector_Base` and the
> in-game overlay-provider patterns were both rejected (pools aren't entity-anchored).

## Goal

After this phase: a CkGameplayDebugger inspector enumerates the ObjectPooling subsystem — one row
per pool (class, archetype, free/in-use/live/high-water/hits/misses/prewarm-remaining) plus the
non-pooled vended count; per-instance drill-down (name, pooled-vs-vended, in-use/free).

## Entry criteria

- [ ] Load `ck-gameplaydebugger-extension` skill (CkGameplayDebugger repo) — do NOT wing the
      extension surface from memory.
- [ ] CkGameplayDebugger submodule on a clean branch off its dev.

## Work items

1. [x] Subsystem enumeration API — already public from Phase 1/4: `ForEach_Pool`, `Get_PoolStats`,
   `Get_NumPinnedUnique` on `UCk_ObjectPooling_Subsystem_UE`.
2. [x] New `CkObjectPoolingDebugger` UncookedOnly module (7 files) + `CkDebugger.uplugin` entry.
   Console command `ck.ObjectPoolingDebugger`, nomad tab under Tools → Developer Tools → Debug.
   POD snapshot gathers from the subsystem; window renders a per-pool table (Class, Archetype,
   Free/InUse/Live/High/Hits/Miss/Prewarm) + summary line (N pools · M pinned-unique), world
   selector + refresh-gate controls, rebuilt each gated tick. Mimics `CkInputDebugger`.
3. [ ] [EDITOR-VERIFY] — see exact steps below.

## [EDITOR-VERIFY] steps (agents cannot open a Slate tab)

1. Open the editor; console `ck.ObjectPoolingDebugger 1` (or Tools → Developer Tools → "CK Object
   Pooling Debugger"). Tab opens.
2. Start PIE. Select the PIE world in the tab's world selector.
3. Enter/exit an area that spawns pooled content (any poolable EntityScript, or a PMG/audio/
   world-space-widget feature — all now DestroyOnRelease-pinned). Confirm: rows appear; the summary
   "N pools · M pinned-unique" tracks; on a poolable script's entity destroy+respawn the row's Free
   then Hits increment.
4. Confirm the refresh controls (Use Global / Hz / OnlyWhenVisible) behave.

## Exit criteria

- [x] Both repos compile — VERIFIED 2026-07-11: full toolbox build green (0 errors); the module's
  4 .cpps compiled + `BusterBlockEditor-CkObjectPoolingDebugger.dll` linked.
- [ ] Tab visible + live-updating — [EDITOR-VERIFY] above (human step).
- [x] PROGRESS.md dated entry; branch `feature/object-pooling-inspector` recorded.
