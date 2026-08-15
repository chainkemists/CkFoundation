# Phase 4 / VALIDATION — acceptance protocol

Run this as its own session after Phase 3, with `ck-change-control` loaded. Nothing here designs;
it proves, documents, and audits.

## 1. Full headless gate

```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Ck.Jolt; Quit" -unattended -nosplash -nullrhi -log -ReportExportPath=<dir>
```

Parse the report (never the exit code alone). **Definition of "no regressions":** every test
name in the session-1 baseline has the same verdict now; the only delta is the 9 new greens:

- `Ck.Jolt.Bake.DynamicMesh.ComplexAsSimpleProducesBody`
- `Ck.Jolt.Bake.DynamicMesh.AsyncCookInFlightFailsLoudly`
- `Ck.Jolt.Bake.DynamicMesh.UnknownClassExplicitFailsLoudly`
- `Ck.Jolt.HeightField.RegionPlan.MappingAndAlignment`
- `Ck.Jolt.HeightField.Update.RegionEditRoundTrip`
- `Ck.Jolt.HeightField.Update.HolePunchAndSurvival`
- `Ck.Jolt.HeightField.Update.EnvelopeExceededRejected`
- `Ck.Jolt.HeightField.Feature.BakeQueryUpdateRemove`
- `Ck.Jolt.HeightField.Feature.CrossRepresentationGuard`

For reference, the planning checkout's baseline suite was 23 tests (target may differ — the
session-1 recorded list governs): BoxConvexRadius x2, Bake.HeightField.KnownHeightsZUp,
Bake.ShapeBlob.RoundTrip, BakeExtraction.{FilterExclusions, MobilityPolicy},
Body.Benchmark.FrameCostMatrix, Body.Lifecycle x3, Body.OwnershipExclusivity x3,
Layers.TableBuild, MeshShape.Utils, Query.BoxOccupancy, Query.GeometryParitySampler,
World.FixedTimestep x5.

Also run the broader plugin gate once (whatever suite prefix the target project uses for
CkFoundation-wide tests, e.g. `Automation RunTests Ck.`) and diff against a captured
before-picture if session 1 recorded one; otherwise record the Ck.Jolt-scoped claim only — do
not claim wider than you measured.

## 2. Three-environment checks

- **C++** — the test suite above IS the C++ proof.
- **Blueprint** — `[EDITOR-VERIFY]` (below): the new nodes exist and compile in a BP graph.
- **AngelScript** — after one editor boot (the generator runs editor-time), inspect the
  generated wrapper:
  `rg -n "Request_BakeHeightField|Request_UpdateRegion|JoltHeightField" <Project>/Script/Generated/`
  → hits in the generated `utils_*` file, and `Request_UpdateRegion`'s wrapper must carry an
  emitted `= FCk_Delegate_Request_OnCompleted()` default (the AutoCreateRefTerm contract). A
  fresh editor-boot log must show zero `Angelscript: Error` lines naming CkJolt symbols.

## 3. `[EDITOR-VERIFY]` — human steps (exact clicks)

1. **BP surface:** open any Blueprint → right-click → search "Bake Height Field" → the three
   `[Ck][Jolt]` heightfield nodes + "Request Bake Component Into Static World" appear; drop
   `Request Bake Height Field`, compile — no errors.
2. **Dynamic-mesh bake, visually:** in a PIE-able map, run a BP/AS snippet that spawns a
   `DynamicMeshActor`, authors any mesh (e.g. GeometryScript box), enables complex-as-simple,
   calls `UpdateCollision`, then `[Ck][Jolt] Request Bake Component Into Static World`. Set
   `ck.Jolt.DebugDraw.Enabled 1` → the mesh's tri-mesh renders in the static-body debug draw.
3. **Heightfield + crater:** call `Request Bake Height Field` with a small grid (e.g. 16x16,
   flat), debug draw shows the sheet; call `Request Update Height Field Region` digging a few
   cells; the debug draw updates within a frame and a character/probe walking the surface drops
   into the crater. (Debug draw rebuilds its batch on geometry change — if the visual does NOT
   update but a ray does, record it: that is a debug-renderer staleness issue
   (`GeometryRef`-keyed batches), not a collision bug; file it as a follow-up, do not fix here.)
4. **Chaos untouched:** with the source `UDynamicMeshComponent` baked, engine `LineTraceByChannel`
   against it still hits (its Chaos body is intact) — req 7 observed, not assumed.

## 4. Documentation (part of done)

- `Source/CkJolt/Claude.md` § "Static world": add two bullets — the DynamicMesh dispatch branch
  (+ async-cook caller contract from D5, + terminal-else loudness split) and the JoltHeightField
  surface (handle ownership, deferred UpdateRegion + the WaitForAsync edge, envelope semantics,
  the D4 vertical-geometry limitation, the one-representation-per-surface guard).
- `Source/CLAUDE.md`: extend the CkJolt row's parenthetical if the module's dep list changed
  (`GeometryFramework` is engine-side, so the Ck-deps column is UNCHANGED — verify, then leave
  the tier table alone; add a "bake runtime dynamic meshes / heightfields into the Jolt static
  world" line to the "I need to..." table pointing at CkJolt).
- This campaign's PROGRESS.md closed out with final gate numbers.

## 5. Final audits

- **Comment audit** over the whole campaign diff (root CLAUDE.md closing step): no gate/phase/
  campaign breadcrumbs, no what-comments; contract comments on the new public utils survive.
- **Scope audit:** `git -C Plugins/CkFoundation diff --stat <baseline-sha>` and the CkTests
  equivalent — every touched file appears in a phase's inventory; anything else is reverted or
  explained in PROGRESS.md.
- **Staging discipline:** stage only authored files; never `git add <dir>`. Do NOT commit or push
  beyond what the phase instructions committed, and do NOT bump any superproject gitlink — a
  sibling session may be mid-work in the same submodule.
- **Before-send re-read** (root CLAUDE.md): confirmed-vs-inferred separation in the final report;
  the one claim most likely wrong, named. Candidates to check honestly: the odd-N padding-row
  behavior (PHASE_2 risk), the debug-draw staleness observation (step 3.3).

## Known-unverified list (planning session could not confirm; executor must)

1. Whether `FJoltWorld` already exposes a temp-allocator accessor (PHASE_3 adds one if not).
2. Vendored Jolt's exact sample-count rounding for odd N (PHASE_2 odd-N test pins it).
3. ~~`AddExpectedError` x `CK_ENSURE` fire-once interplay~~ — RESOLVED at CTO review: repeat
   suppression is disabled under automation (`CkEnsure.cpp`); house pattern is
   `AddExpectedError(..., Occurrences = -1)`. See PHASE_3 risks.
4. The target checkout's engine/project names and its baseline test list (session 1 records).
5. Whether an editor-world `UDynamicMeshComponent::UpdateCollision(false)` cooks synchronously in
   the target engine build with default cvars (PHASE_1 gate G1 confirms; planning verified the
   code path reads `bUseAsyncCooking=false` default + game/editor-world cvar gate).
