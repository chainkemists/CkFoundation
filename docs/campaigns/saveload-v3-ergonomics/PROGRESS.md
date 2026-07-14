# PROGRESS — saveload-v3-ergonomics

Living doc. The EXECUTOR updates this at every phase boundary and whenever reality diverges from a phase
doc. Planner: Fable session 2026-07-14 (design discussion with Adam + 3-agent terrain survey). Branch:
`feature/save-load-improvements` (CkFoundation submodule; CkTests changes ride its same-named branch).

Adam's constraints: **harness first; minimize builds (one per phase, 4 total); each phase independently
shippable; quick.** Nothing is pushed by the executor (Class-4 → Adam review).

## Baseline (recorded at Phase 1 entry — executor fills)

- CkFoundation HEAD: ____ (clean tree: Y/N)
- CkTests HEAD: ____ (clean tree: Y/N)
- Parity campaign Phase 5 committed: Y/N (hashes: ____)
- Ck.Snapshot: ____ pass / ____ fail (names: ____) — planner expectation 30/30
- Ck.Net: ____ pass / ____ fail (names: ____) — planner expectation 90/90

## Phase status

| Phase | Status | Commit(s) | Gate result vs baseline | Notes |
|---|---|---|---|---|
| 1 — harness + cast sweep + Jump absolute | NOT STARTED | | | |
| 2 — rename bundle (absorbs parity PHASE_6) | NOT STARTED | | | parity PROGRESS 6A/6B rows annotated: Y/N |
| 3 — symmetry class (a), 7 features | NOT STARTED | | | |
| 4 — symmetry class (b), fold | NOT STARTED | | | |
| 5 — CkEcs/Persistence header split | NOT STARTED | | | RunAfter seam outcome (fwd-decl vs include): ____ |
| VALIDATION | NOT STARTED | | | |

## Blockers (STOP-and-record; do not improvise past these)

_(none yet)_

Format per entry: date / phase+step / what was expected / what was observed (verbatim error or grep
output) / what you did NOT do / question for the planner or Adam.

## Deviations from plan (executed differently than written, with reason)

_(none yet)_
