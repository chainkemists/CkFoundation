# Phase 1 — DynamicMesh tri-mesh dispatch + loud terminal

**Goal:** `ExtractComponent` handles `UDynamicMeshComponent` (Chaos-parity tri-mesh/AggGeom via
`BuildShape_FromBodySetup`), and the dispatch chain no longer ends in silence. Resolves decisions
D1 (ordering), D2, D2b, D5 (the loud half). Requirements 1–3 and 7 are satisfied when this phase
ships, because `Request_BakeComponent` already provides the runtime entry point and the
replace-on-rebake contract.

## Entry criteria

1. Clean tree (`git -C Plugins/CkFoundation status` and the CkTests equivalent show no
   modifications you did not author). Record both HEAD SHAs in PROGRESS.md.
2. Baseline captured: headless `Automation RunTests Ck.Jolt` on the untouched tree; full
   name+verdict list recorded in PROGRESS.md. If any test is already red, name it in the baseline
   — it is not yours to fix.
3. Symbol re-verification (target checkout may differ from planning checkout):
   - `rg -n "else if \(const auto\* Brush = Cast<UBrushComponent>" Source/CkJolt` → exactly 1 hit
     in `CkJoltBakeExtraction.cpp` (the branch you extend after).
   - `rg -n "No cooked tri-mesh on BodySetup" Source/CkJolt` → 1 hit (the ensure the async-cook
     test expects).
   - `ls <EngineRoot>/Engine/Source/Runtime/GeometryFramework` → exists.
   - Anything missing → STOP, record in PROGRESS.md § Blockers.

## Step 1 — dependencies

1. `Source/CkJolt/CkJolt.Build.cs`: add `"GeometryFramework"` to `PublicDependencyModuleNames`,
   next to the existing `"Landscape"` entry, extending the existing comment block that explains
   the Chaos-parity engine deps (e.g. append: `// ... and runtime dynamic meshes
   (UDynamicMeshComponent) for the explicit-bake dispatch.`).
2. CkTests `Source/CkTests/CkTests.Build.cs`: add `"GeometryFramework"`. If the Phase-1 test
   later fails to compile on `FDynamicMesh3` types, also add `"GeometryCore"` — add it only if
   the compiler asks.

## Step 2 — the executable spec (commit red first)

Create `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkJolt/Test_JoltBake_DynamicMesh.cpp`.
Mimic the file conventions of `Test_Jolt_BakeExtraction_MobilityPolicy.cpp` (same `#if WITH_EDITOR
&& WITH_DEV_AUTOMATION_TESTS` guard, same flag constants, same throwaway-editor-world harness —
copy how that file creates/destroys its world verbatim) and the ray-probe helper of
`Test_JoltBake_HeightField_KnownHeightsZUp.cpp` (`CastDownAt`). Three tests:

```cpp
// Test A: "Ck.Jolt.Bake.DynamicMesh.ComplexAsSimpleProducesBody"
//   - Spawn ADynamicMeshActor in the editor world; grab GetDynamicMeshComponent().
//   - Author a 2-triangle quad, corners (0,0,50) (200,0,50) (200,200,50) (0,200,50), via
//     UE::Geometry::FDynamicMesh3 (AppendVertex x4, AppendTriangle x2, winding so the face
//     normal points +Z), then Component->SetMesh(MoveTemp(Mesh)).
//   - Component->SetCollisionProfileName(TEXT("BlockAll"));
//     Component->SetComplexAsSimpleCollisionEnabled(true, false);
//     Component->UpdateCollision(false);   // force a SYNCHRONOUS cook (bUseAsyncCooking
//                                          // defaults false; editor world => sync path)
//   - ck::jolt::bake::ExtractComponent(*Component, Cache, Bodies, {}, ExplicitActor)
//   - EXPECT: return == 1, Bodies.Num() == 1, Bodies[0]._Shape valid.
//   - EXPECT: CastDownAt(*Bodies[0]._Shape, 100.0, 100.0) hits at Z ~= 50 (tolerance 1.0)
//     — shape-local probe; the body transform is identity for an actor spawned at origin.
//     This pins the winding swap: a wrong winding makes the down-ray miss or hit the far side.

// Test B: "Ck.Jolt.Bake.DynamicMesh.AsyncCookInFlightFailsLoudly"
//   - Same authoring, but set Component->bUseAsyncCooking = true BEFORE UpdateCollision(false).
//   - The first cook is queued, not complete: the component's const GetBodySetup() is null or
//     holds no cooked tri-mesh.
//   - AddExpectedError for the ensure text (match a distinctive substring, e.g.
//     "has collision enabled but no BodySetup" OR "No cooked tri-mesh on BodySetup" — see
//     decision gate G2 below; mimic how Test_JoltBake_BoxConvexRadius registers expected
//     ensures).
//   - EXPECT: ExtractComponent returns 0, Bodies stays empty, the expected error was seen.

// Test C: "Ck.Jolt.Bake.DynamicMesh.UnknownClassExplicitFailsLoudly"
//   - Spawn a plain AActor with a USphereComponent root (SetCollisionProfileName BlockAll,
//     RegisterComponent).
//   - AddExpectedError("no extraction path for this component class") — substring match.
//   - EXPECT: ExtractComponent(..., ExplicitActor) returns 0 with the ensure seen.
//   - Second sub-assert: same component, LevelSweep policy with a default filter → returns 0
//     with NO error expectation (Verbose-only path).
```

Commit this file (with the Build.cs edits from Step 1) as the red spec. Build, run:

```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Ck.Jolt.Bake.DynamicMesh; Quit" -unattended -nosplash -nullrhi -log
```

**Decision gate G1 (red confirmation):** expected — Test A FAILS with "return == 1" (actual 0:
the component falls off the dispatch chain); Test C FAILS on the missing expected error (no
terminal branch exists yet); Test B may already pass (the null-BodySetup case has no ensure yet
→ if B fails on "expected error not seen", that is the same defect, fine). Paste the actual
output into PROGRESS.md. If Test A fails on anything else (compile error, world-boot error,
`SetComplexAsSimpleCollisionEnabled` missing) → fix the TEST, not the framework; if the engine
API surface differs from the above, STOP and record.

**Gate G2 (which ensure does B hit):** run Test B once and record which failure path fires with
no implementation: if `GetBodySetup()` (const) returns null there is nothing to ensure yet and B
red-fails; after Step 3 it must hit the new "no BodySetup" ensure. Pin B's expected-error string
to whichever ensure the implemented branch actually fires — do not leave both.

## Step 3 — implement

All in `CkJoltBakeExtraction.cpp` (+ forward declare/include at top: `#include
"Components/DynamicMeshComponent.h"` in the .cpp only — do NOT add GeometryFramework types to
`CkJoltBakeExtraction.h`; the header must stay engine-light).

Insert after the `UBrushComponent` branch, before the chain end:

```cpp
else if (const auto* DynamicMesh = Cast<UDynamicMeshComponent>(&InComponent))
{
    // The const overload returns MeshBodySetup WITHOUT create-on-demand — a never-cooked
    // component must read null here and fail loudly, not conjure an empty setup.
    const auto* BodySetup = static_cast<const UDynamicMeshComponent*>(DynamicMesh)->GetBodySetup();
    const auto DebugName = ck::Format_UE(TEXT("DynamicMesh on {}"), InComponent.GetPathName());

    CK_ENSURE_IF_NOT(ck::IsValid(BodySetup),
        TEXT("DynamicMeshComponent [{}] has collision enabled but no BodySetup — collision was never "
             "cooked (call UpdateCollision after authoring the mesh; with bUseAsyncCooking the cook "
             "may still be in flight, which this bake cannot wait for)."), DebugName)
    { return 0; }

    // Runtime-recooked geometry: UpdateCollision assigns a fresh BodySetupGuid every sync recook,
    // so the guid-keyed shared cache would leak one entry per edit — build directly, the same
    // reason the SplineMesh branch bypasses it.
    const auto Shape = BuildShape_FromBodySetup(*BodySetup, ComponentTransform.GetScale3D(), DebugName);
    EmitBody(Shape, ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BodySetup);
}
else
{
    if (InPolicy == ECk_Jolt_ExtractionPolicy::ExplicitActor)
    {
        CK_TRIGGER_ENSURE(TEXT("Explicit bake of [{}] ([{}]) — no extraction path for this component "
            "class. Supported: StaticMesh, ISM/HISM, SplineMesh, Brush, DynamicMesh. Baked NOTHING. "
            "Opt the component out (DoNotBake / bake-filter tag) or bake supported geometry."),
            InComponent.GetName(), InComponent.GetClass()->GetName());
    }
    else
    {
        ck::jolt::Verbose(TEXT("Static bake skipping [{}] ([{}]) — no extraction path for this "
            "component class"), InComponent.GetName(), InComponent.GetClass()->GetName());
    }
}
```

Notes that are load-bearing:
- `CTF_UseComplexAsSimple` routing, empty-geometry ensures, and AggGeom support all come free
  from `BuildShape_FromBodySetup` — do not reimplement any of it.
- The exact cast spelling for the const `GetBodySetup()` overload may need adjusting to whatever
  compiles cleanly (`Cast<UDynamicMeshComponent>` on a `const UPrimitiveComponent&` already
  yields a const pointer; the `static_cast` above may then be redundant — prefer the simplest
  form that selects the CONST overload; verify with a deliberate `UpdateCollision`-less test run
  that no BodySetup is created as a side effect).
- If the ensure/verbose text is reworded, update the Test B/C expected-error substrings in the
  same commit — they must reference the same wording.

## Step 4 — gate

1. Build (editor target). Expected: clean. UHT is not involved (no reflected types changed).
2. Run `Automation RunTests Ck.Jolt` (full suite, not just the new file).
   - **Expected:** baseline verdicts unchanged, PLUS `Ck.Jolt.Bake.DynamicMesh.*` 3/3 green.
   - **If Test A green but a baseline test flipped red** → your change; the prime suspect is the
     terminal else firing inside an existing test (e.g. a test actor carrying an unexpected
     primitive under ExplicitActor). Fix by scoping, never by deleting the baseline test.
   - **If Test A red on the ray probe only** → winding or transform bug in the branch; compare
     against `Build_TriMeshShape`'s winding-swap comment before touching anything.
   - Anything else → STOP, PROGRESS.md § Blockers.

## Exit criteria (all measurable)

- `Ck.Jolt` suite: baseline verdicts + 3 new greens; counts recorded as
  "baseline N pass/M fail {names} → N+3 pass/M fail {same names}".
- `rg -n "no extraction path" Source/CkJolt` → exactly 2 hits (ensure + verbose).
- `rg -n "GeometryFramework" Source/CkJolt/CkJolt.Build.cs` → 1 hit.
- Diff touches ONLY: `CkJolt.Build.cs`, `CkJoltBakeExtraction.cpp`, CkTests Build.cs + the new
  test file. Anything else in the diff is scope creep — revert it.
- Comment audit done (root CLAUDE.md rule): every comment in the diff is a *why*.

## Fences (do NOT)

- Do NOT add a generic `GetBodySetup()` catch-all branch (killed — PROMPT.md D2).
- Do NOT route the DynamicMesh branch through `FCk_Jolt_ShapeCache` (guid churn leak — D2).
- Do NOT include GeometryFramework headers in `CkJoltBakeExtraction.h` — .cpp only.
- Do NOT add retry/deferral for async cook (D5); the ensure IS the contract.
- Do NOT touch the Landscape special case in `ExtractActor`, the skip-reason gate, or any
  existing branch — the diff to existing code is: one `else if`, one `else`, includes, Build.cs.
- Do NOT add Chaos/Jolt exclusivity checks to the bake path (D6 — non-conflict, confirmed).

## Risks + rollback

- **Risk:** the ExplicitActor terminal ensure fires in existing game content. CTO-identified
  likeliest vector: `Request_BakeActor` on actors carrying **QueryOnly trigger shape components**
  — the eligibility gate rejects only `NoCollision` (`Get_ComponentSkipReason`), so a QueryOnly
  `UShapeComponent` reaches the terminal. Secondary vector: CkUnrealComponent `Automatic`
  hosting an unsupported-class primitive (same subsystem entry). Both judged correct-loud (they
  surface a component the caller believes is baked but is not) and the opt-outs exist
  (`DoNotBake`, `Ck.Jolt.NoBake` tag). **Rollback:** downgrade that one ensure to a Warning log
  — single-line change, test C adjusts its expectation. Extending extraction to cover
  `UShapeComponent` properly is a **tracked follow-up outside this campaign** (it changes
  level-sweep body counts and cooked hashes) — do not implement it here.
- **Risk:** engine API drift on `UDynamicMeshComponent` (5.7.x point differences). All claimed
  symbols were verified against the planning engine tree; if the target engine differs, STOP at
  G1 and record.
- **Whole-phase rollback:** revert the single commit; no persisted data formats are touched
  (cooked static-world data is unaffected — dynamic meshes never appear in level sweeps at cook
  time by definition of runtime-transient actors).
