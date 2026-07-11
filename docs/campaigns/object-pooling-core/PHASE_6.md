# Phase 6 — Docs, AS verification, wrap

> **Status:** ⏳ Pending
> **Depends on:** Phases 1-5 ✅

## Work items

1. `Source/CkCore/Claude.md` + `Source/CkCore/ObjectPooling/README.md` — pooling section (when to
   pool, participant contract, reset-skip rule, pin-everything model, precedence of pool-config
   sources). Mine `feature/pool-module`'s `Source/CkPool/Claude.md` prose (the when-to-pool and
   anti-pattern sections survive the pivot).
2. `Source/CLAUDE.md` — "I need to..." row for object pooling → CkCore/ObjectPooling; note the
   CkPool row never existed on dev (no removal needed).
3. Root `CLAUDE.md` — only if a non-negotiable or macro row changed (likely none).
4. `.claude/reports/DECISIONS.md` — entry for the pivot (EntityPool dropped, pin-everything model,
   participant rename) so `ck-failure-archaeology` finds it.
5. AngelScript verification (root non-negotiable #4): utils namespaces generated
   (`utils_object_pooling_participant`, object utils overloads), AS autotest from Phase 4 exercises
   them; grep fresh compile log for `Angelscript: Error` naming new files.
6. Campaign docs: mark all phases ✅ with dates; PROGRESS.md final entry; tombstone-or-keep decision
   for this folder recorded in PROGRESS.md.

## Exit criteria

- [ ] All success criteria in PROMPT.md checked off with evidence links.
- [ ] Final full-suite run recorded vs campaign-start baseline.
- [ ] Branches + push state enumerated (CkFoundation / CkTests / CkGameplayDebugger) — push only on
      user instruction.
