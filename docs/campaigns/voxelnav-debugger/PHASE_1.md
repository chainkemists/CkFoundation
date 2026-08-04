# PHASE 1 - exact cooked-Jolt editor preview and shared authoring

> Status: implementation complete; rendered editor acceptance pending. Authored 2026-08-04 after the Phase 0 snapshot gate.
> Depends on: Phase 0 value snapshots and the existing versioned Jolt static-world cook.

## Goal

After this phase, a level can contain an editor-visible VoxelNav volume whose exact values compose
the runtime ECS volume at BeginPlay, and editor code can build the same VoxelNav octree outside PIE
from validated cooked Jolt shapes without starting a game world or approximating through Chaos.

## Locked implementation boundary

1. `ACk_VoxelNavVolume_UE` is a placed runtime authoring actor. Its box transform and build fields
   produce `FCk_Fragment_VoxelNavVolume_ParamsData`; BeginPlay passes that same value to
   `UCk_Utils_VoxelNavVolume_UE::Add`.
2. CkJolt owns cooked-shape restore and exact overlap/segment queries. Its public editor-facing
   contract is value-only and JPH-free, with explicit missing/version/restore failures.
3. A new editor-only `CkVoxelNavEditor` module owns discovery, invalidation, build pacing, and
   snapshot publication. CkGameplayDebugger only asks it for immutable snapshots.
4. Only a successfully validated cooked map may publish `Current`. Missing or stale cook never
   falls back to Unreal collision. Script-created volumes remain `RuntimeOnly` outside PIE.

## Work items

- [x] Add the placed runtime authoring actor and BeginPlay/EndPlay ECS lifetime seam.
- [x] Factor a strict cooked-Jolt query from the existing runtime restore path.
- [x] Add `CkVoxelNavEditor` module and preview editor subsystem.
- [x] Discover authored volume actors and build exact VoxelNav snapshots outside PIE.
- [x] Invalidate on map/property changes, revalidate on refresh, and retain stale values only with visible status.
- [x] Add focused snapshot, authoring, fail-closed query, and editor-overlay lifecycle tests.

## Required failure states

| Condition | Snapshot status | Cell policy |
|---|---|---|
| No placed authored volume | `RuntimeOnly` | none |
| Cooked index or required cell missing | `MissingCook` | none |
| Cook/version/source validation stale | `StaleCook` | retain last complete values, visibly stale |
| Exact preview build active | `Building` | retain last complete values, visibly building |
| Restore/query/build failure | `Failed` | no newly partial values |
| Valid cook and completed build | `Current` | whole replacement snapshot |

## Exit criteria

- [ ] Outside PIE can produce `Current` merged/raw/occupied cells from a placed volume.
- [x] Runtime and editor preview use the same authored params and exact Jolt collision definition.
- [x] CkVoxelNav contains no JPH include and CkGameplayDebugger contains no world/Jolt ownership.
- [x] Map teardown clears live objects; retained snapshots remain values only.
- [ ] Focused build/tests and direct missing/stale/failed rejection coverage are green. Fail-closed unready
      query behavior is automated; asset-backed stale/corrupt fixtures remain future hardening.
- [x] Permanent docs and PROGRESS updated; comment audit complete.

## Next phase

Phase 3 lands the grouped Crowd Debugger controls, real 3D viewport, camera presets, and snapshot
renderer. Phase 4 connects Live PIE, retained, and editor-preview sources plus the Level Editor
overlay.
