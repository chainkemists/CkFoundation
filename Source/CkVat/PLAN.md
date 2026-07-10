# CkVat — gate index (PLAN.md)

> **Written:** 2026-07-09. **Gate status lives HERE and only here** (gate files carry contracts,
> not status). Update the row in the same commit that lands the gate.
> **This doc dies when:** the campaign ships (post-ship cleanup deletes Plan/, PROMPT, PROGRESS;
> `Claude.md` is the survivor).

| Gate | Scope (one line) | Status |
|---|---|---|
| [0 — Foundation](Plan/Gate_00_Foundation.md) | Campaign docs; shared `ck::anim_bake` extraction + Iskm refactor (zero test delta); CkVat/CkVatEditor scaffolds compile; uplugin rows | ✅ Done (2026-07-09) — UNCOMMITTED working tree |
| [1 — Bake](Plan/Gate_01_Bake.md) | CkVatEditor bake: vertex+bone texture atlases, static-mesh build w/ lookup UVs, serialized clip table, re-bake overwrite | 🟡 Code-complete (2026-07-09) — [EDITOR-VERIFY] bake on real content pending (human) |
| [2 — Material](Plan/Gate_02_Material.md) | VAT decode `.ush` (both modes) as CkUsf looks; generator UV1/UV2/LocalPosition/basis wiring; masters generated headless | 🟡 Code-complete (2026-07-09) — [EDITOR-VERIFY] visual pass pending |
| [3 — Playback](Plan/Gate_03_Playback.md) | Subsystem (shared MID + transient renderer); IsmProxy composition; 12-float GPU-time packing; crossfade; phase offset; OnClipFinished | 🟡 Code-complete (2026-07-09) — [EDITOR-VERIFY] pending |
| 4 — Hardening | AS/BP parity verification; Source/CLAUDE.md + module Claude.md rows; perf sanity (stat capture); campaign doc tombstones | ⏳ Pending |

Gate contracts are authored **at gate entry** (only Gate 0's exists today — writing all five up
front would snapshot conventions that drift). Each gate re-derives its entry baseline per
[PROGRESS.md](PROGRESS.md).

Post-ship cleanup: delete `Plan/`, `PROMPT.md`, `PROGRESS.md`, fold the permanent contract into
`Source/CkVat/Claude.md` + a row each in `Source/CLAUDE.md`'s decision tree and tier table.
