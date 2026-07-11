# Phase 6 — Docs, AS verification, wrap

> **Status:** ✅ Done (2026-07-11) — one BP node-visibility check remains [EDITOR-VERIFY]
> **Depends on:** Phases 1-5 ✅

## Work items

1. [x] `Source/CkCore/Public/CkCore/ObjectPooling/README.md` — full mechanism doc (pin-everything,
   two policies, participant contract, reset-skip, config precedence, EntityScript integration,
   when-not-to-pool) + CkCore/CLAUDE.md use-case row + subfolder count 48→49.
2. [x] `Source/CLAUDE.md` — "I need to..." row for object pooling (CkPool never existed on dev, so
   no removal).
3. [x] Root `CLAUDE.md` — no change needed (no non-negotiable/macro touched; pooling is additive).
4. [x] `.claude/reports/DECISIONS.md` — entries 98-105 (intrinsic-not-module, pin-everything,
   EntityPool dropped, actors excluded, no veto, participant-mirrors-ContextReceiver, the sweep,
   Get_ScriptInstance-not-exposed).
5. [x] AngelScript verification (root non-negotiable #4): the Phase-4 autotests compile+run against
   the generated `utils_object::` wrappers (`Request_CreateNewObject_Pooled`, `TryReleaseToPool`,
   `Get_ObjectPoolStats`, `Get_IsPoolTrackedObject`) and the participant struct accessors — 4/4
   green, 0 `Angelscript: Error`. AS is DONE.
6. [x] Campaign docs marked ✅; PROGRESS.md final entry; keep-not-tombstone decision below.

## Remaining [EDITOR-VERIFY] (human — agents can't open editor UI)
- **Blueprint** node visibility for the new BP surface: search the four `[Ck] ...` DisplayNames
  (`Request Create New Object (Pooled)`, `Try Release Object To Pool`, `Get Object Pool Stats`,
  `Get Is Pool-Tracked Object`); confirm `FCk_ObjectPooling_PoolParams` Make-node + its
  EditCondition-gated fields; confirm the EntityScript `_PoolParams` shows only under the Poolable
  policy. (C++ ✅ build, AS ✅ tests — this is the third leg of non-negotiable #4.)
- The debugger tab (PHASE_5.md steps).
- The poolable EntityScript policy in a real editor spawn.

## Keep-or-tombstone
KEEP the campaign folder until all three branches merge to their `dev`s. On merge, the permanent
survivor is `CkCore/Public/CkCore/ObjectPooling/README.md`; tombstone this folder then (per the
PROMPT.md death condition).

## Exit criteria

- [x] Success criteria in PROMPT.md — all met except BP node visibility ([EDITOR-VERIFY] above) and
      the deferred net-replicated-poolable test (logged in PHASE_4.md).
- [x] Final full-suite run recorded: 1052/1044/8 (same 8 pre-existing BB failures) on the final
      grace/gate binary — vs the 1048/1040/8 campaign-start baseline (+4 pooling, 0 regressions).
- [x] Branches + push state enumerated in PROGRESS.md current-state (all committed, NONE pushed;
      merge order: CkFoundation FIRST).
