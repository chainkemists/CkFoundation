# CkVisualLod — PLAN.md (executive gate index)

> **Written:** 2026-08-27. Status rows update in the SAME commit as each gate landing.
> **This doc dies when:** the campaign ships (post-ship cleanup deletes gate plans;
> `CLAUDE.md` is the permanent survivor).

Mission: [PROMPT.md](PROMPT.md) · Design: [DESIGN_CkVisualLod.md](DESIGN_CkVisualLod.md) ·
Living state: [PROGRESS.md](PROGRESS.md)

| Gate | Name | Status |
|---|---|---|
| 0 | Scaffold + data surface (module boilerplate, fragments, config asset, handles; compiles C++/BP/AS) | ✅ Done (2026-08-27) |
| 1 | Mechanism (pools, acquisition, flip, fades, budgets, locks, whole-query ranking; EndPlay + sweep reconciliation) + C++ automation tests | ✅ Done (2026-08-27) |
| 2 | Game seam (signals + timing verification), `TryGet_LocalViewInfo` in CkCamera, full request surface, CkTests autotests + gym | 🟡 In progress — viewer discovery + gym ✅ (CkTests 98f670ef); remaining: flip-lifecycle AS autotests |
| 3 | Debugger surface (Gen 3 overlay providers: member state + arbiter budgets) | ✅ Done (2026-08-27, CkGameplayDebugger 880c822) |
| 4 | BB adoption + parity + duplicate deletion (executes in the BusterBlock host, tracked here) | ⏳ Pending |

Gate contracts (`Plan/Gate_NN_*.md`) are authored at each gate's entry, per ck-methodology — a
contract written months early snapshots conventions that move. Gate 0's contract is authored first,
immediately after design sign-off.

Estimates are recorded per-gate at entry and re-dated on entry (campaigns overrun; the docs must
survive that).
