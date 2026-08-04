# PHASE 5 — Perf: node merging + benchmark gate + close-out coverage

> Freshness: authored 2026-08-04 at the 4→5 boundary. Status of record: PROGRESS.md. Binding:
> [C-D4] (the cell seam was BUILT for this — layer nibble 14, kind-dispatch functions, search
> never touches Get_NodeAddress), [C-D12] (McGill evidence: merging is the ~4× leaf-count /
> sub-1ms lever), [C-D18], [C-D20]/[C-D21] (conditional items below).

## Entry criteria

- [ ] Phase 4 closed. Baseline: full suite delta-zero; `Ck.VoxelNav` 57/57.

## Units

### Wave 1 — 5A: node merging behind the cell seam (one agent)

1. A post-bake (and post-repair) MERGE pass: greedily coalesce adjacent free cells into
   axis-aligned merged boxes (McGill-style; exact algorithm is the executor's by-evidence call —
   greedy box-growing over the free-cell set is sufficient; optimality is not the bar, cell-count
   reduction is). Output: a merged-cell table on the octree (id = the reserved nibble-14 FCellId
   space), each merged cell knowing its box + neighbor merged-cells (face adjacency), plus a
   cell→merged-cell lookup the kind-dispatch functions use.
2. The graph layer: when merging is ON (settings knob, default ON after A/B proves it),
   `FPathGraph` enumerates merged cells via the EXISTING kind-dispatch seam — search code
   unchanged (that is [C-D4]'s payoff; if search needs edits beyond the dispatch functions, the
   seam failed — STOP and report). Refinement/raycast operate on the underlying octree unchanged.
3. Repair composes: a local repair re-merges only the affected region (the merge pass over the
   dirty neighborhood; the rest of the table survives — mirror the repair's locality contract).
4. A/B BENCHMARK (VALIDATION's perf gate): on the reference scenes (the hermetic known-layout +
   a larger generated scene — author one at ~6400uu/50uu per the port map's worked example),
   record bake time, cell count (plain vs merged), search time (representative routes), and
   bake-budget frame counts. Numbers go into the test as pinned assertions where stable
   (cell-count reduction ≥ a conservative floor) and into PROGRESS as the recorded A/B.
5. All 57 existing tests stay green with merging ON and OFF (both configurations gated).

### Wave 2 — 5B: close-out coverage (one agent, small)

1. AS autotest exercising the public API end-to-end (`utils_voxel_nav_volume` +
   `utils_voxel_nav_path` namespaces: Add volume → Request Build → wait → Request FindPath →
   read result) — the AngelScript leg of VALIDATION's three-environments item. AS traps apply
   (no NOT, uppercase Math constants, _TimeoutSeconds on the actor, --discover-fresh).
   BP leg: recorded as covered by the [EDITOR-VERIFY] gym steps (the BPFL nodes ARE the BP
   surface) — write that interpretation into VALIDATION.md.
2. Conditional [C-D20]: IF 5A's benchmark shows a bake stage exceeding the per-tick budget at
   reference scale, thread the cursor+budget through Stage_RasterizeLayer/BuildNeighbourLinks;
   else record the measured numbers and leave the code alone.
3. Comment audit over the whole campaign diff (root-doctrine closing step, both repos).

## Exit criteria (campaign-final)

- [ ] `Ck.VoxelNav` full pattern green (57 + new), merging ON and OFF.
- [ ] Full suite delta-zero — ONE sample. `Ck.Jolt.Query` green.
- [ ] A/B numbers recorded in PROGRESS.md; VALIDATION.md perf-gate items checked with evidence.
- [ ] VALIDATION.md swept top-to-bottom: every item checked or explicitly `[EDITOR-VERIFY]`
      (human-owned) / deferred-pool (decision-referenced).
- [ ] PROGRESS.md final status board + session log; campaign close-out summary.

## Fences

- No async-search upgrade ([C-D21] leftover — deferred pool unless a benchmark forces it).
- No cross-volume routing, cooked bake, WP streaming, tactical port, debugger inspector,
  CkSpatialHash, kinematic-domain filter — all deferred pool with decision references.
- Merging must not change any functional test's outcome — it is a representation change under
  the seam, and the 57 are the proof.
