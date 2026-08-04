# Campaign: voxelnav-debugger - outside-PIE 3D navigation inspection

> Stable mission brief. Moving state lives in [PROGRESS.md](PROGRESS.md).
> Freshness: authored 2026-08-04 at campaign open. Death condition: the campaign ships and the
> permanent contracts have moved into the CkVoxelNav, CkJolt, and CkGameplayDebugger module docs.

## Mission

Make CkVoxelNav inspectable before, during, and after PIE from the existing Crowd Debugger and the
Level Editor viewport. The tool must show the exact volumetric navigation representation built
from CkJolt collision, support a real 3D camera, organize the existing noisy debug controls, and
remain responsive on volumes with tens of thousands of raw cells and hundreds of agents.

## Success criteria

1. Outside PIE, an authored VoxelNav volume can be built from the current map's cooked Jolt data
   and displayed as volume bounds, merged search cells, raw free cells, occupied cells, chunks,
   portals, and validity state.
2. The preview never labels Chaos-derived, missing, corrupt, version-mismatched, or stale Jolt
   data as current. Missing and stale inputs are explicit states, not empty free-space fallbacks.
3. The Crowd Debugger center uses a depth-tested 3D editor viewport with orbit/pan/zoom, frame
   selection/all, and Perspective, Top, Bottom, Left, Right, Front, and Back presets.
4. The existing row of Crowd Debugger checkboxes is replaced by grouped Navigation, Crowd, and
   Diagnostics layer controls with per-user persistence and visible source/status information.
5. PIE uses live VoxelNav epochs and shows builds, repairs, dirty regions, paths, and registered
   occluders. Stopping PIE leaves an explicitly retained value snapshot without retaining ECS,
   Jolt-world, actor, or UWorld references.
6. The same snapshot drives an optional Level Editor overlay so outside-PIE inspection has actual
   map context comparable to Unreal's `P` visualization.
7. Merged cells are the default. Raw-cell rendering is capped, filterable, cached by source epoch,
   and visibly reports shown versus total counts. No per-cell actor/component or per-frame octree
   enumeration is introduced.
8. Focused automated tests cover snapshot fidelity, truncation, cache invalidation, source states,
   layer grouping, and camera math. Toolbox gates are delta-zero against the recorded baseline.
9. The 400-agent VoxelNav stress gym remains the visual load scenario and exposes the live source,
   repair, epoch, and render-budget observations needed by the editor checklist.

## Locked decisions

| Decision | Choice | Why |
|---|---|---|
| Product home | Extend the Gen-2 CkCrowdDebugger and retain `ck.CrowdDebugger` compatibility | It already owns Recast, Crowd paths, agent selection, and navigation diagnostics. |
| Runtime/editor boundary | CkFoundation owns debug data; CkGameplayDebugger owns UI and rendering | Prevents debugger dependencies from entering gameplay modules and keeps snapshots reusable. |
| Outside-PIE bar | A fresh exact editor bake is required; retained PIE snapshots are supplemental | A last-known picture cannot satisfy Unreal-`P` parity or validate current geometry. |
| Geometry source | Current map's cooked Jolt shapes only | Runtime VoxelNav occupancy is Jolt-defined; Chaos overlap would be a misleading approximation. |
| Editor ownership | New editor-only CkVoxelNavEditor service | Runtime ECS/Jolt subsystems intentionally exclude Editor worlds. |
| Authored source | A shared editor-visible volume definition consumed by runtime composition and editor preview | Preview and gameplay settings must not drift into two independent copies. |
| Data handoff | Immutable value snapshot with explicit source/status/epoch/fingerprint | Slate and editor renderers must never retain registry-bound handles or physics-world pointers. |
| Renderers | Embedded SEditorViewport plus optional Level Editor overlay | The embedded view provides focused 3D inspection; the Level Editor overlay provides map context. |
| Cell rendering | Cached batched/instanced layers, merged cells by default | A measured reference scene contains 91,752 raw cells but only 359 merged search cells. |
| Dynamic behavior | Visualize registered VoxelNav occluders and repair epochs; do not imply ordinary NPC bodies modify the bake | Current occupancy includes Static-domain Jolt geometry; kinematic NPC bodies are not voxelized. |

## Non-goals

- Replacing CkNavigation/Recast for grounded agents.
- Making every dynamically scripted runtime-only volume previewable before it has an editor-visible
  definition. Such volumes remain available through Live PIE and Retained Snapshot sources.
- Serializing VoxelNav octrees as gameplay/cooked assets in this campaign. The editor preview is a
  derived debug artifact, not a runtime loading feature.
- Changing VoxelNav pathfinding, merging, clearance, or repair semantics to simplify visualization.
- Creating a second approximation using Chaos or render meshes.

## Reading list and exemplars

- [research/ARCHITECTURE.md](research/ARCHITECTURE.md) - current source evidence and rejected alternatives.
- `Source/CkVoxelNav/CLAUDE.md` - VoxelNav representation, Jolt boundary, merging, and repair contracts.
- `Source/CkJolt/CLAUDE.md` and `Source/CkJoltEditor/Claude.md` - game-world ownership and cooked data.
- `CkGameplayDebugger/Source/CkCrowdDebugger` - existing view model, Recast collection, and controls.
- `Source/CkGridEditor/Private/EdMode/Ck2dGridSystem_EdMode.cpp` - Level Editor PDI overlay precedent.
- `CkGameplayDebugger/Source/CkDebuggerCommon` - refresh, selection, viewport, icon, and teardown contracts.

## Things ruled out - do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| Add pitch and cubes to the current Slate map | It is an XY projection with no camera projection, depth buffer, or occlusion. | `SCkCrowdDebugger_ViewportPanel.{h,cpp}` |
| Use Chaos editor overlaps | It would not represent the Jolt collision used by runtime VoxelNav. | CkVoxelNav module boundary and backend contract |
| Force gameplay Jolt/ECS subsystems into Editor worlds | Their world-type exclusion and lifecycle are intentional. | `CkGameWorldSubsystem.cpp`, `CkJolt/CLAUDE.md` |
| Let CkGameplayDebugger own the bake | Bake validity and geometry ownership belong to CkVoxelNav/CkJolt, not UI. | debugger extension boundary |
| Treat a retained PIE snapshot as completion | It can be stale before the next PIE run and cannot preview a newly authored map. | success criteria 1-2 |
| Emit twelve PDI lines per raw cell every frame | It scales with duplicated edges and bypasses caching/instancing. | measured 91,752-cell reference volume |
