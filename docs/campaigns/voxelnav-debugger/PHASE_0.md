# PHASE 0 - contracts, baseline, and value snapshots

> Status: complete. Authored 2026-08-04 at campaign open; closed 2026-08-04.
> Depends on: completed `voxelnav-port` campaign at `eef267b` plus CkGameplayDebugger `e05812b`.

## Goal

After this phase, the exact source/status contract is locked, the regression baseline is recorded,
and CkVoxelNav can export deterministic bounded value snapshots without exposing an octree, ECS
registry, UObject, or Jolt object to debugger clients.

## Entry criteria

- [x] Root, CkFoundation, CkGameplayDebugger, and CkTests HEADs and dirty paths recorded.
- [x] Current VoxelNav, Jolt cooked-world, Crowd Debugger viewport, and test exemplars inspected.
- [x] Neighboring patterns named: VoxelNav's ECS-free `FBuildState`, Jolt cooked-world restore,
      CkGridEditor Level Editor drawing, and CkDebuggerCommon viewport/lifetime helpers.
- [x] Full pre-change Toolbox build/all-tests baseline completed and failure names recorded in
      [BASELINE_20260804-152229.md](BASELINE_20260804-152229.md).

## Work items

1. Add plain debug snapshot types to CkVoxelNav:
   - source kind and `MissingCook` / `StaleCook` / `Building` / `Current` / `Failed` /
     `RuntimeOnly` status;
   - volume/build metadata and fingerprints;
   - value-only `FBox` cell arrays for merged, raw free, occupied, chunks, portals, dirty bounds;
   - total/shown/truncated counts for every bounded layer.
2. Add a pure snapshot builder over a published `FOctree` and build metadata. Filter by requested
   layers, octree depth, optional clip bounds, and deterministic cell cap before copying.
3. Add an epoch/fingerprint cache that republishes only when source identity, source epoch,
   requested layer/filter state, or budget changes. Missing/failed sources retain no live pointers.
4. Add C++ automation coverage for fidelity, ordering, truncation, invalid caps, and generation
   changes. Exercise one real bake and one repair epoch through existing CkTests fixtures.
5. Update the CkVoxelNav permanent module doc with the debug snapshot contract.

## Expected observations and branches

| Run | Expected | If instead | Response |
|---|---|---|---|
| Pure snapshot tests | Counts/bounds match the source; cap is deterministic | Order or counts depend on container iteration | Define and test stable ordering before proceeding |
| Real bake snapshot | Published epoch 1 produces one generation | Repeated reads rebuild | Stop and fix cache identity before UI work |
| Repair snapshot | New epoch atomically replaces prior value arrays | Readers observe mixed generations | Move publication behind whole-snapshot swap |
| Invalid/missing source | Explicit status and zero unsafe access | Empty space appears Current | Treat as a production validation defect and add direct rejection coverage |

## Exit criteria

- [x] Full baseline and focused post-change tests recorded in PROGRESS.md.
- [x] Snapshot API contains no `FCk_Handle`, UObject pointer, UWorld pointer, JPH type, or exposed
      `TSharedPtr<const FOctree>`.
- [x] Snapshot/cap/cache tests green after the final Phase 0 edit.
- [x] `CkVoxelNav/CLAUDE.md`, PROGRESS.md, and this status block updated for the Phase 0 landing.
- [x] Comment audit complete for Phase 0 files.

## Next phases

- Phase 1: strict cooked-Jolt editor query and editor-visible shared volume authoring.
- Phase 2: CkVoxelNavEditor preview subsystem and exact outside-PIE builds.
- Phase 3: Crowd Debugger grouped controls, embedded 3D viewport, and camera presets.
- Phase 4: Live PIE, retained snapshot, Recast/Crowd integration, and Level Editor overlay.
- Phase 5: stress gym/manual acceptance, performance evidence, final gates, docs, and delivery.
