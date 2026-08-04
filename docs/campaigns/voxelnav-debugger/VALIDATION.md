# voxelnav-debugger validation contract

## Automated

- [x] Host Editor build succeeds after final C++/Build.cs/uplugin changes.
- [x] Full test suite has no persistent new failing names versus the dated baseline. Two untouched-module
      names failed only in the four-lane run and each passed 1/1 in an isolated fresh-process rerun.
- [x] `Ck.VoxelNav.DebugSnapshot.*` covers fidelity, requested layers, counts, bounds, stable order,
      zero/invalid cap, truncation, depth filter, clip filter, and value-only ownership.
- [x] Snapshot cache covers same-epoch reuse, epoch replacement, filter/budget replacement,
      missing/failed/stale source semantics, and explicit clear/source switch.
- [ ] Editor cooked-query tests cover missing index, version mismatch, corrupt shape blob, stale
      intersecting actor/cell, valid selected-cell restore, and exact occupied/free probes.
- [ ] Editor preview tests prove definition edits mark the preview stale and a successful rebuild
      advances one preview epoch.
- [ ] Crowd Debugger tests cover layer group membership/order, group aggregate state, unrelated
      setting isolation, per-user persistence, camera preset transforms, and invalid framing bounds.
- [x] Fresh focused editor/test log contains no new ensure, AngelScript error, or stale-bytecode evidence.

## Performance contract

- [x] No octree/cooked-world enumeration from Slate `OnPaint` or per-frame viewport drawing.
- [x] No actor, scene component, or UObject per VoxelNav cell.
- [x] Merged cells are default; raw free/occupied layers are opt-in and report shown/total.
- [x] Render inputs rebuild only on snapshot publication or layer/filter change.
- [ ] The 91,752-raw-cell reference fixture stays interactive under the configured default cap;
      CPU build and render-update timings are recorded rather than inferred.

## Editor verification

- [ ] `[EDITOR-VERIFY]` Outside PIE, open a map with an authored VoxelNav volume and current cooked
      Jolt data. Open Crowd Debugger, choose Editor Preview, enable Merged Cells, and confirm cells
      appear in correct map positions.
- [ ] `[EDITOR-VERIFY]` Use Perspective, Top, Bottom, Left, Right, Front, and Back controls; confirm
      orthographic presets are not mirrored and Perspective supports standard orbit/pan/zoom.
- [ ] `[EDITOR-VERIFY]` Enable raw free and occupied cells, lower the cell cap, and confirm the
      shown/total banner changes while the viewport remains responsive.
- [ ] `[EDITOR-VERIFY]` Make cooked Jolt data missing or stale and confirm the debugger reports
      Missing/Stale and never displays fabricated Current geometry.
- [ ] `[EDITOR-VERIFY]` Enable the Level Editor overlay and confirm the same snapshot aligns with
      authored level geometry outside PIE.
- [ ] `[EDITOR-VERIFY]` Start the 400-agent VoxelNav stress gym. Choose Live PIE and confirm agent
      paths, merged cells, source badge, epoch, and repair state update without per-frame rebuilds.
- [ ] `[EDITOR-VERIFY]` Move a registered VoxelNav occluder and confirm dirty/repair visualization
      plus one new published epoch. Confirm an ordinary kinematic NPC is not presented as an
      occluder unless registered/configured accordingly.
- [ ] `[EDITOR-VERIFY]` Stop PIE and confirm Retained Snapshot remains visible, clearly labeled,
      with no crash, ensure, or stale handle during the next PIE start.
- [ ] `[EDITOR-VERIFY]` Reopen the debugger and confirm grouped layer settings and camera/source
      preferences persist without restoring transient ECS selection handles.
