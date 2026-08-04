# PHASE 3 — Chunks & dynamics

> Freshness: authored 2026-08-04 at the 2→3 boundary. Status of record: PROGRESS.md. Spec detail:
> `research/research-coupling.md` §9 (multi-volume/chunking mechanics + the actor-identity problem
> we're replacing) and §8 (dynamic update path). Binding: [C-D6] stable ids, [C-D9] (audit
> upstream dynamic occlusion — issue #39 says it's broken — port the DESIGN, verify the behavior),
> [C-D12] (local repair over full rebuild), [C-D18] speed policy.

## Entry criteria

- [x] Phase 2 closed. Baseline: full suite 981/975/6; `Ck.VoxelNav` 38/38; `Ck.Jolt.Query` green.

## Units (SEQUENTIAL waves — both touch Volume/)

### Wave 1 — 3B: dynamic occluders + local rebuild (single-volume)

1. **Audit first ([C-D9])**: read upstream `Nav3DDynamicOcclusion.cpp` + `RebuildDirtyBounds` /
   `RebuildLeafNodesInBounds` / `PropagateChangesToHigherLayers` against issue #39's symptom
   (dynamic occlusion broken in v2.0) and the coupling report's findings (two dead O(n)
   whole-octree scans per dirty box; `AffectedNodes` computed then unused). Report the audit
   verdict BEFORE porting — what upstream got wrong, what the correct design is.
2. Occluder feature: `FCk_Handle_VoxelNavOccluder` on the occluding entity (transform-watching
   processor — compare vs cached, emit dirty-bounds on real movement; house transform read, no
   actor tick); volumes intersecting the dirty bounds re-rasterize affected leaves through the
   backend (budgeted, same Jolt window as the bake) and propagate up — LOCAL repair, never a full
   rebake. Publish via epoch bump (path staleness is already derived — zero path-walking).
3. Tests: hermetic — move a stub obstacle, dirty-rebuild, occupancy flips locally and ONLY
   locally (assert untouched-leaf identity), epoch bumps; PIE — kinematic JoltBody moves, path
   over the old geometry reads Stale, replan avoids the new position.

### Wave 2 — 3A: chunked volumes + cross-chunk pathfinding

1. Partitioning port (`PartitionVolumeIfNeeded` math): volumes exceeding a settings-driven max
   partition size split into N chunk child entities (stable `FChunkId` = volume id + index —
   NEVER entity/actor identity in serialized or adjacency data), each baking its own octree
   through the existing machinery.
2. Adjacency bake: face-sharing test + boundary-voxel matching (`Nav3DUtils` boundary machinery
   ports; portals keyed by chunk ids). O(N²) prefilter fine at our chunk counts.
3. Cross-chunk search: upstream's decision tree (same chunk → existing path; same volume → BFS
   over the adjacency graph then portal-to-portal in-chunk segments, stitched) — extend
   `Search_PathGraph`'s seam; fix upstream's XY-only `ContainsPoint` (3D contains), no
   `TActorIterator` equivalents (chunk lookup via the volume's record of chunk children).
4. Tests: hermetic — two-chunk bake, path crosses the boundary seamlessly (every segment clear,
   portal transition cells adjacent); adjacency survives a chunk re-bake (stable ids); PIE —
   bake a partitioned volume over the box scene, path across a chunk boundary.

## Exit criteria

- [ ] Targeted `Ck.VoxelNav` all green (38 + new), `Ck.Jolt.Query` green.
- [ ] Full suite delta-zero (six known reds) — ONE sample.
- [ ] Hygiene: `<Jolt/` grep zero; no actor/entity identity in adjacency or serialized data
      (grep for TWeakObjectPtr/FCk_Handle in the chunk adjacency types).
- [ ] Comment audit; PROGRESS updated; PHASE_4.md authored at the boundary.

## Fences

- No crowd/InstallExternalPath (Phase 4). No merging/async (Phase 5). No serialization-to-disk of
  chunk payloads (deferred pool — in-memory only this phase). No World Partition streaming.
- The occluder rebuild must never rasterize outside the dirty bounds' affected leaves (that is
  the local-repair contract — test-pinned).
