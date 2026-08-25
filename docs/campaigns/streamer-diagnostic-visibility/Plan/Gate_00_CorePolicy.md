# Gate 0 — CkCore diagnostic visibility policy

> **Status:** ✅ Done (2026-08-24)
> **Depends on:** None

## Goal

After this gate, all consumers ask one typed CkCore policy whether runtime diagnostics are suppressible for a streamer capture.

## Work items

1. Create the public CkCore policy contract with the CVar/launch names and a typed visibility mode.
   → verify: a C++ automation test proves CVar-on, CVar-off, and launch override precedence.
2. Migrate CkCore immediate draw wrappers and HUD queue flushing to the contract.
   → verify: existing debug-draw test remains green against the new API.
3. Retain a forwarding compatibility predicate only until all current callers migrate.
   → verify: repository search reports no runtime owner relying on draw-helper policy ownership.

## Expected observations

| I will run | I expect | If instead | Response |
|---|---|---|---|
| Focused C++ policy test | CVar off restores enabled, on suppresses, launch remains absolute | CVar state leaks between tests | add scoped CVar restore and rerun fresh process |
| Host editor build | CkCore and consumers link | dependency cycle/link failure | revise public dependency direction before continuing |

## Exit criteria

- [ ] Policy test evidence recorded in PROGRESS.md.
- [ ] Gate 1 can consume the public contract without depending on CkDebugDraw.
- [ ] PLAN.md and this status update in the same landing commit.
