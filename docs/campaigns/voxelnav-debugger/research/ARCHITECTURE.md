# voxelnav-debugger architecture research - 2026-08-04

## Current behavior

- CkVoxelNav runtime volumes are ECS entities. The runtime builder gets geometry through a
  game/PIE Jolt subsystem and fails when that session is unavailable.
- The VoxelNav core build machine itself is ECS-free and backend-driven. It publishes a whole
  immutable octree only when a build completes.
- CkJolt already cooks per-map index/cell assets containing versioned Jolt shapes and actor data,
  but runtime restore currently tolerates some stale actors to keep gameplay running. An exact
  editor preview needs stricter selected-volume validity.
- CkCrowdDebugger currently paints a two-dimensional Slate map from copied Recast triangles and
  copied Crowd values. It has no depth-tested viewport or pitch-capable camera.
- CkGridEditor already demonstrates editor-world overlays through an EdMode and
  `FPrimitiveDrawInterface`.

## Required boundaries

1. CkJolt owns restoring and querying cooked Jolt shapes. Public consumers receive a JPH-free
   query value and explicit validation result.
2. CkVoxelNav owns turning geometry queries into an octree and turning a published octree into a
   bounded value snapshot.
3. CkVoxelNavEditor owns editor-visible authored volume discovery, preview build scheduling,
   source fingerprints, and preview epochs.
4. CkGameplayDebugger owns layer controls, 3D viewport resources, camera interaction, and the
   optional Level Editor overlay registration.

## Data flow

```text
Editor map + authored VoxelNav volume
        |
        v
current cooked Jolt index/cells --strict validation--> JPH-free cooked query value
        |                                                   |
        |                                                   v
        +------------------------------------------> CkVoxelNav backend
                                                            |
                                                            v
                                            budgeted ECS-free FBuildState
                                                            |
                                                            v
                                                  immutable FOctree
                                                            |
                                      layer/depth/clip/cap snapshot builder
                                                            |
                                                            v
                                         immutable value render snapshot
                                                /                       \
                                               v                         v
                                Crowd Debugger SEditorViewport    Level Editor overlay
```

Live PIE replaces the editor builder with the published runtime volume octree. Retained Snapshot
copies the last value snapshot and never retains the octree or registry.

## Performance shape

- Snapshot generation is keyed by source identity, source epoch/fingerprint, requested layers,
  filter state, and budget.
- Merged boxes are the default layer. Raw free and occupied arrays are generated lazily.
- Filtering occurs before copying and before the deterministic cap.
- Rendering uses cached batched/instanced geometry for high-count cells. PDI is reserved for
  low-count bounds, portals, selection, labels, and Level Editor overlay lines where appropriate.
- No generation occurs from Slate `OnPaint`; no cell has a dedicated UObject.

## Rejected alternatives

- Chaos or render-mesh editor occupancy: not equivalent to Jolt collision.
- Gameplay subsystems in Editor worlds: violates intentional world/lifetime gates.
- Debugger-owned bake: breaks CkFoundation/debugger dependency direction.
- Retained-only outside PIE: stale by construction and cannot inspect new authoring.
- Per-frame PDI/Slate cubes: unbounded CPU and draw cost.

## Risks to prove at phase gates

- Cook strictness for World Partition and unloaded actors.
- Landscape parity between editor extraction and the cooked data runtime actually loads.
- Shared authoring without duplicating absolute world bounds between an asset and placement.
- Jolt cooked-shape narrowphase queries without leaking JPH types across module boundaries.
- Preview scene context versus Level Editor context; both must consume the same snapshot.
