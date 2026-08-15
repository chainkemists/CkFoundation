# Campaign: runtime dynamic-mesh + heightfield geometry in the CkJolt static world

**Executor profile:** Opus/Sonnet-class session, zero memory of the planning session. Follow the
phases in order; do not redesign. Every architecture decision is already made and argued below —
if reality diverges from a step, STOP and record it in `PROGRESS.md` § Blockers instead of
improvising.

**Review status:** CTO-reviewed 2026-08-14 (`docs/reviews/2026-08-14-jolt-runtime-terrain-bake-CTO-review.md`,
committed `d8fe7e65b`) — CHANGES REQUESTED, all four blockers applied to these docs same day
(stale-entry liveness rule, `Deinitialize` drain + `_HeightFieldEntities`, debug-name macro
placement, cast-fence rewording), plus two review finds folded in (`AddExpectedError`
`Occurrences=-1` pattern; D2b vector ranking). Per the review, that flips it to **GREEN-LIGHT,
no re-review**. Both architecture questions (D3 attribution reuse; sync-bake/deferred-update
split) were signed off.

**Planning provenance:** every file/symbol claim below was read and verified 2026-08-14 against
CkFoundation @ the `feature/ckui-split` checkout (BusterBlock superproject) and the
UnrealEngine-Angelscript 5.7 engine tree. The TARGET checkout may be a different superproject
(`D:\Repo\Venus`) at a different CkFoundation commit — **anchor on symbol names, not line numbers**,
and re-verify each cited symbol exists before editing (Phase entry criteria include this).

---

## Problem

CkJolt's static world bakes level geometry into Jolt bodies. Runtime-generated geometry on
`UDynamicMeshComponent` (engine `GeometryFramework`, an Engine/Source/Runtime **module**, not a
plugin) currently produces **zero bodies silently**: `ck::jolt::bake::ExtractComponent`
(`Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltBakeExtraction.cpp`) dispatches
SplineMesh → ISM/HISM → StaticMesh → Brush and **ends with no terminal else** —
`UDynamicMeshComponent` derives `UMeshComponent`, not `UStaticMeshComponent`, so it falls off the
end with no ensure and no log. Additionally there is no way to hand the static world a
CPU heightfield directly, no region-update path, and therefore no cheap deformation story.

## Requirements (acceptance)

1. `Request_BakeComponent` on a `UDynamicMeshComponent` produces Jolt bodies or fails loudly.
2. Runtime-only entry points (no cook, no level sweep dependency). — *already satisfied by the
   `Request_BakeComponent` surface once dispatch exists.*
3. Wholesale re-bake replaces the previous generation with no leaks. — *already the
   `Request_BakeComponent` re-bake contract; extended to heightfields.*
4. Partial region updates without full reconversion (deformation/craters).
5. A direct heightfield bake path (row-major float grid, square lattice).
6. Holes (cells with no collision).
7. Chaos collision on the source components is untouched (navigation/engine traces keep working).

## Chosen architecture (do not re-litigate)

Two capabilities, four phases, each independently shippable:

- **Phase 1 — tri-mesh dispatch**: an explicit `UDynamicMeshComponent` branch in
  `ExtractComponent`, plus a terminal else that ends the silent fall-off. Delivers reqs 1–3, 7.
- **Phase 2 — heightfield shape core**: an updatable variant of `CreateHeightFieldShape` that
  exposes the inner `JPH::HeightFieldShape` and takes a deformation height-range envelope, plus a
  **pure** region-update planning function (UE-rect → Jolt-rect mapping, block alignment,
  envelope validation). No ECS, no world. Delivers the mechanics of reqs 4–6.
- **Phase 3 — the JoltHeightField feature**: typesafe handle + params + deferred UpdateRegion
  request + processor + utils + subsystem wiring. Delivers reqs 4–6 as public API (C++/BP/AS).
- **Phase 4 — validation + docs**: full gate, three-environment checks, `[EDITOR-VERIFY]`, doc
  rows, comment audit.

### Decision record (each argued; the executor implements, never re-decides)

**D1 — Both tri-mesh dispatch AND heightfield surface ship; dispatch first.**
They serve different requirements: dispatch is a small, self-contained fix for reqs 1–3 (the
existing `Request_BakeComponent` replace-on-rebake contract does the rest); the heightfield
surface is the only thing that delivers cheap region updates (req 4) and the memory/query win
(req 5). Sequencing dispatch first gives a shippable win in one small phase and gives vertical
geometry (walls/overhangs) a supported path before the heightfield docs point at it (see D4).

**D2 — Explicit `UDynamicMeshComponent` branch; the generic `GetBodySetup()` terminal branch is
KILLED.** Kill reason: a generic "any primitive with a BodySetup" branch changes level-sweep
admission for every BodySetup-bearing class at once (UShapeComponent, UProceduralMeshComponent,
skinned meshes) — unaudited blast radius on existing maps' body counts and cooked-data staleness
hashes, and it quietly absorbs component classes whose instance/transform semantics were never
audited (non-negotiable #3: fallbacks that hide problems). The dependency cost that motivated the
generic option is gone: `GeometryFramework` lives at `Engine/Source/Runtime/GeometryFramework`
(verified) — a Build.cs one-liner exactly like the existing `Landscape` dep, not a plugin dep.
The new branch **bypasses `FCk_Jolt_ShapeCache`** and calls `BuildShape_FromBodySetup` directly:
`UDynamicMeshComponent::RebuildPhysicsData` assigns a **fresh `BodySetupGuid` per sync recook**
(verified in engine source, `DynamicMeshComponent.cpp`, sync branch: `BodySetup->BodySetupGuid =
FGuid::NewGuid()`), so the guid-keyed cache would leak one dead entry per edit — the same reason
the SplineMesh branch already bypasses it (in-file precedent).

**D2b — Terminal else loudness is policy-split.** Under `ExplicitActor`
(`Request_BakeActor/Component` — caller declared intent) an unsupported component class is
`CK_TRIGGER_ENSURE` naming the class and the supported set; under `LevelSweep` it is a Verbose
log (sweeps legitimately visit every primitive class in a level; ensure-spam would be noise).
Known consequence, accepted (vector ranking per CTO review): the likeliest real-content fire is
`Request_BakeActor` on actors carrying **QueryOnly trigger shape components** (the eligibility
gate rejects only `NoCollision`, so QueryOnly `UShapeComponent`s reach the terminal); secondary,
CkUnrealComponent `Automatic` hosting an unsupported-class primitive. Both surface a real silent
gap (the caller believes the geometry is baked; it is not), and two opt-outs already exist
(`DoNotBake` policy; `Ck.Jolt.NoBake` component tag, which the Automatic path honors). Proper
`UShapeComponent` extraction is a tracked follow-up OUTSIDE this campaign (level-sweep body
counts + cooked hashes change). Rollback if the ensure proves too hot in practice: downgrade the
ExplicitActor branch to a Warning log — one-line change, flagged in PHASE_1 risks.

**D3 — Heightfield API shape.** A new mini-feature `JoltHeightField` inside
`Source/CkJolt/Public/CkJolt/StaticWorld/`:
- **Ownership/addressing:** `Request_BakeHeightField` returns `FCk_Handle_JoltHeightField`, a
  typesafe handle over an attribution entity that carries BOTH the existing
  `ck::FFragment_JoltStaticActor_Current` (so ray-hit attribution, `Request_RemoveBodiesForEntity`
  idempotent teardown, and `FProcessor_JoltStaticActor_EndPlay` all work unchanged) AND the new
  heightfield fragments. Destroying the handle's entity tears the body down through the existing
  funnel for free.
- **Bake/remove are synchronous subsystem-backed calls** (mirroring `Request_BakeActor/Component`
  — the neighboring surface; these mutate subsystem-owned world state, not ECS feature state, and
  the existing bake surface is synchronous). **UpdateRegion is a deferred ECS request** drained by
  a processor — deliberately NOT synchronous, because `JPH::HeightFieldShape::SetHeights` is
  documented unsafe against in-flight collision queries and the async physics step; the
  processor carries the mandatory `RunAfter FProcessor_JoltWorld_WaitForAsync` edge (CkJolt
  CLAUDE.md: "Any new processor that touches Jolt state must add the same edge"). This also
  batches multiple edits per frame naturally.
- **Re-bake/replacement:** params carry an optional `_SourceComponent`. When set, the subsystem
  keys the heightfield in a `_ManualHeightFields` map by component — re-baking the same component
  REPLACES (same semantics as `_ManualComponentEntities`), and a **reciprocal guard** ensures
  loudly if the same component is already tri-mesh-baked via `Request_BakeComponent` (and vice
  versa) — a caller cannot hold both representations of one surface. When unset, addressing is
  handle-only: caller removes explicitly; a second bake is a second independent surface.
- **Block alignment** (Jolt `SetHeights` requires block-multiple rect): NOT rejected —
  **expand-and-overlay**. The update plan expands the caller's rect outward to block alignment,
  reads the current heights for the border cells via `HeightFieldShape::GetHeights`, overlays the
  caller's sub-rect, and submits the aligned rect. Exact for every caller-specified cell; border
  cells pay only the re-quantization loss Jolt's own `SetHeights` already documents. Holes
  (`cNoCollisionValue`) read back through `GetHeights` and survive the overlay.
- **Envelope validation is loud and OURS:** Jolt `SetHeights` **silently clamps** values outside
  `[GetMinHeightValue(), GetMaxHeightValue()]` (verified in vendored `HeightFieldShape.h`) —
  silent clamping is a silent failure, so the processor validates the incoming heights against
  the stored encodable range BEFORE calling SetHeights and rejects the whole request loudly
  (`CK_ENSURE` + completion `Failed`) on violation. The bake params carry an explicit
  deformation envelope (enum-mode + min/max pair, house optionality style) that maps to Jolt's
  `HeightFieldShapeSettings::mMinHeightValue/mMaxHeightValue` (verified present) so craters can
  dig below the initial surface.
- **After every `SetHeights`: `BodyInterface::NotifyShapeChanged`** (verified at
  `BodyInterface.h`, `NotifyShapeChanged(...)`) — height edits change the shape's bounds; a stale
  broadphase AABB drops queries silently.

**D4 — Vertical geometry: documented limitation + the tri-mesh path; NO auto-generated companion
bodies.** A heightfield stores one height per sample and cannot express overhangs or vertical
faces — synthesizing "wall" bodies from grid data would be approximated collision, which this
module explicitly bans ("NEVER a bounding-box substitute"; `MakeScaleValid` rejected for the same
reason). The honest contract: per surface, the caller chooses — heightfield for terrain-shaped
geometry, `Request_BakeComponent` (tri-mesh, works after Phase 1) or explicit primitive bodies
for anything with walls. Both primitives exist in the same module after Phase 3; the choice is a
caller decision CkJolt cannot make for them. This is documented on the utils API and in
CkJolt/Claude.md (Phase 4).

**D5 — Async-cook ordering is the caller's contract; the ensure stays loud.** Verified engine
behavior: with `bUseAsyncCooking=true` (default **false**), `RebuildPhysicsData` queues a NEW
BodySetup and swaps it in later via `FinishPhysicsAsyncCook` — so a bake immediately after an
edit reads the PREVIOUS setup (silently stale geometry), and a bake after the FIRST async cook
reads a null/empty setup (trips the existing loud ensure in `Build_TriMeshShape`). CkJolt cannot
distinguish stale-cooked from current-cooked on the BodySetup surface, so no retry machinery:
the documented contract is "bake only after a synchronous `UpdateCollision`; leave
`bUseAsyncCooking` at its false default on components you intend to bake." A quiet retry that
can never detect staleness would be a silent failure. The first-cook async case is pinned by a
test (loud ensure, zero bodies).

**D6 — Chaos coexistence: confirmed non-conflict; do NOT "fix" it.** The Chaos-XOR-Jolt ownership
rule is enforced at **JoltBody composition time** (pinned by the three
`Ck.Jolt.Body.OwnershipExclusivity.*` tests) and governs dynamic-body fragments on entities.
Static-world bakes add no `FFragment_JoltBody_*` to anything; extraction READS the component's
BodySetup and never touches its Chaos physics state. Source components keep full Chaos collision
(req 7) by construction. If you find yourself adding an exclusivity check to the bake path, stop
— that is scope creep against an explicit requirement.

## Executable spec

Phase 1 leads with a **failing test** (full source in PHASE_1.md § Step 2):
`Ck.Jolt.Bake.DynamicMesh.ComplexAsSimpleProducesBody` — commit it red, run it, paste the failing
output into PROGRESS.md, then implement until green. Phases 2–3 each lead with their own failing
tests (source in their files). This planning session could not itself run the red tests (running
them requires the Build.cs edits that are themselves implementation), so the "current failing
output" is predicted in each phase and must be confirmed-and-recorded by the executor at the
phase's first decision gate.

## File inventory (verified 2026-08-14; re-verify symbols at each phase entry)

| File | Why it matters |
|---|---|
| `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltBakeExtraction.h/.cpp` | Dispatch chain (`ExtractComponent`), `BuildShape_FromBodySetup`, `Build_TriMeshShape` (loud ensure on empty `TriMeshGeometries`), `CreateHeightFieldShape` (+row-flip + `RotatedTranslatedShape` wrap), `HeightFieldNoCollisionValue`, `FCk_Jolt_ShapeCache` (guid-keyed — the leak reason), `FCk_Jolt_ExtractedBody` |
| `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h/.cpp` | `Request_BakeComponent` (replace-on-rebake pattern to mirror), `_ManualComponentEntities`, `DoCreate_ComponentEntity`, `DoCreate_BodiesFromExtracted`, `DoBatchAdd_Bodies`, `Request_RemoveBodiesForEntity` (idempotent funnel), `DoNote_BodiesChanged` |
| `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Utils.h/.cpp` | The BP/AS bake surface to mirror for the heightfield utils |
| `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticActor_Fragment_Data.h` / `_Fragment.h` / `_Processor.*` / `_Utils.*` | The attribution entity the heightfield entity reuses; the typesafe-handle declaration pattern; `FProcessor_JoltStaticActor_EndPlay` teardown funnel |
| `Source/CkJolt/Public/CkJolt/CollisionLayers/CkJoltCollisionLayer_Utils.h` | `ck::jolt::TryDerive_SignatureFromProfile(FName, ECk_Jolt_BodyDomain)` — how a profile name becomes a `FCk_Jolt_CollisionSignature` |
| `Source/CkJolt/Public/CkJolt/World/CkJoltWorld.h` | `ck::FJoltWorld` — owns `JPH::TempAllocatorImpl* _TempAllocator` (SetHeights needs a TempAllocator; add a `Get_TempAllocator()` accessor if none exists — verify at Phase 3 entry) |
| `Source/CkJolt/CkJolt.Build.cs` | Gains `GeometryFramework` (mirrors the existing `Landscape` engine-module dep, comment included) |
| `Source/CkJolt/Claude.md` | Doc rows for both capabilities (Phase 4) |
| `Source/CkThirdParty/.../JoltPhysics/Jolt/Physics/Collision/Shape/HeightFieldShape.h` | READ-ONLY. `SetHeights`/`GetHeights` (block-aligned rects, silent clamp), `HeightFieldShapeSettings::mMinHeightValue/mMaxHeightValue`, `cNoCollisionValue` |
| `Source/CkThirdParty/.../Jolt/Physics/Body/BodyInterface.h` | READ-ONLY. `NotifyShapeChanged` |
| CkTests: `Source/CkTests/Private/UnitTests/CkJolt/` | All new tests land here (house rule: CkFoundation feature tests live in the CkTests plugin). Molds: `Test_JoltBake_HeightField_KnownHeightsZUp.cpp` (shape-level, `CastDownAt` probing, hole assert), `Test_Jolt_BakeExtraction_MobilityPolicy.cpp` (throwaway editor world + spawned actor), `Test_JoltBody_Lifecycle.spec.cpp` (real world + subsystems via `CkTests/Net/CkNetAutomation_Common.h`, NumClients=1, `/Engine/Maps/Entry`) |
| CkTests: `Source/CkTests/CkTests.Build.cs` | Gains `GeometryFramework` (+`GeometryCore` if the compiler asks) for the Phase-1 test |
| Engine (READ-ONLY): `Engine/Source/Runtime/GeometryFramework/...` | `UDynamicMeshComponent` — `GetBodySetup() const` (returns `MeshBodySetup`, no create-on-demand), `SetMesh(FDynamicMesh3&&)`, `SetComplexAsSimpleCollisionEnabled`, `UpdateCollision(bool bOnlyIfPending)`, `bUseAsyncCooking=false` default, `ADynamicMeshActor` |

**Scope note:** the task brief says "CkFoundation only". Framework code lands in CkFoundation;
tests land in the CkTests sibling plugin because that is where CkFoundation feature tests live by
standing rule (there is no test module inside CkFoundation). "Game-side consumers out of scope"
holds: nothing outside these two plugins is touched.

## Glossary

- **Static world** — `UCk_JoltStaticWorld_Subsystem_UE`'s baked, immovable Jolt bodies (Static
  body domain), separate from dynamic `JoltBody` entities.
- **Attribution entity** — the one ECS entity per baked source (actor/component) whose id is
  stamped as Jolt body user-data so hits resolve to a handle. Fragment:
  `ck::FFragment_JoltStaticActor_Current`.
- **ExplicitActor policy** — extraction mode where the caller declared the geometry
  static-in-intent; the settings-driven bake filter is bypassed.
- **Complex-as-simple** — `CTF_UseComplexAsSimple`: Chaos collides against the cooked tri-mesh;
  `Build_TriMeshShape` converts that exact tri-mesh to a `JPH::MeshShape` (winding-swapped).
- **Block alignment** — Jolt heightfield edits (`SetHeights`) require x/y/size to be multiples of
  `mBlockSize` (default 2).
- **Envelope** — the min/max world-height range the heightfield can encode, fixed at
  construction (16-bit block compression); edits outside it are clamped by Jolt (silently — we
  pre-validate loudly).
- **Row flip** — `CreateHeightFieldShape` maps UE row-major samples onto Jolt's local Y-up X/Z
  grid via +90°-about-X wrap with rows flipped (`r = N-1-y`) and a `-(N-1)*scaleY` local-Z
  offset. Region updates must apply the SAME flip (the single likeliest executor mistake — see
  PHASE_2 fences).

## Skills to load, and when

- `ck-macros-and-codegen` — before Phase 3 (new typesafe handle, request struct, processor
  registration).
- `ck-tests-authoring-and-running` — before writing any test (Phases 1–3).
- `ck-change-control` — at every phase close (gate evidence).
- `ck-debugging-playbook` — only on build/UHT/AS failures.
- Read before any code: root `CLAUDE.md` (ensure discipline, request contract),
  `Source/CLAUDE.md` (tier rules), `Source/CkJolt/Claude.md` (static-world contract, processor
  edge rule).

## Verification reality (target checkout has NO UnrealToolbox)

- **Build:** `<EngineRoot>\Engine\Build\BatchFiles\Build.bat <ProjectName>Editor Win64
  Development -Project=<path>\<Project>.uproject -WaitMutex -FromMsBuild`. Resolve
  `<EngineRoot>`/`<ProjectName>` from the target checkout at session 1 and RECORD both in
  PROGRESS.md — do not guess between sessions.
- **Tests (headless):** `<EngineRoot>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
  <uproject> -ExecCmds="Automation RunTests Ck.Jolt; Quit" -unattended -nosplash -nullrhi -log
  -ReportExportPath=<dir>`. Parse the JSON report / log for per-test results — do NOT trust the
  process exit code alone. Gotchas: never edit source while a run is in flight; avoid commas
  inside the `-ExecCmds` string (comma is a command separator); the `Ck.Jolt` filter prefix runs
  the whole suite.
- **Baseline (mandatory before any change):** run the `Ck.Jolt` suite once on the untouched
  tree, record the full list of test names with pass/fail into PROGRESS.md. On the planning
  checkout the suite is 23 tests (names listed in VALIDATION.md); the target checkout may
  differ — the recorded baseline is the number that "no regressions" is measured against.
- Anything only observable in a live editor is `[EDITOR-VERIFY]` with exact steps (VALIDATION.md).

## Phase index

| Phase | Delivers | Ships alone? |
|---|---|---|
| [PHASE_1](PHASE_1.md) | DynamicMesh dispatch + terminal else + `GeometryFramework` dep + 3 tests | Yes — reqs 1–3, 7 |
| [PHASE_2](PHASE_2.md) | Updatable heightfield shape core + pure region-update plan fn + 4 tests | Yes — additive C++ API |
| [PHASE_3](PHASE_3.md) | JoltHeightField feature (handle/params/request/processor/utils/subsystem) + tests | Yes — reqs 4–6 |
| [PHASE_4 / VALIDATION](VALIDATION.md) | Full gate, three-env checks, `[EDITOR-VERIFY]`, docs, comment audit | Acceptance |
