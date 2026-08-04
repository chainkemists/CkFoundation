# PHASE 0 — Scaffold

> Freshness: authored 2026-08-03. Status of record: PROGRESS.md.

## Entry criteria

- [ ] CkFoundation on `feature/ckvoxelnav-port`, based at `c11766760` (origin/dev tip at campaign
      open), working tree clean apart from campaign docs.
- [ ] Baseline recorded in PROGRESS.md § Baseline (full toolbox build+test counts on the unmodified
      branch). **No Source/ edits may land before the baseline run completes.**

## Units

### 0A — Vendor libmorton (route: opus)

1. Copy `F:\Nav3D-2.0\Source\ThirdParty\libmorton\*` (8 headers + LICENSE + README.md, verbatim,
   flat layout) → `Source/CkThirdParty/Public/CkThirdParty/libmorton/`.
   → verify: `ls` shows 10 files; LICENSE is MIT; no file modified (byte-identical to source).
2. `Source/CkThirdParty/CkThirdParty.build.cs`: add
   `Path.Combine(ModuleDirectory, "Public/CkThirdParty/libmorton")` to `PublicIncludePaths`
   (headers are flat in that folder — include form is `#include "morton.h"` namespaced
   `libmorton::`).
   → verify: entry present; style matches the existing five entries.
3. `Source/CkThirdParty/Claude.md`: add a Vendored-libraries table row
   (`libmorton/ | libmorton | Morton/Z-order encode-decode for SVO node keys`) and a usage rule
   naming **CkVoxelNav** as the sole sanctioned direct consumer (mirror the Jolt rule's shape).
   → verify: both edits present.

### 0B — CkJolt occupancy primitive (route: opus)

Add to CkJolt (NOT to CkVoxelNav — decision [C-D5]):

1. `ck::jolt::Get_IsBoxOccupied(const JPH::PhysicsSystem&, const FVector& InCenter,
   const FVector& InHalfExtents, ...filters...) -> bool` — namespace-level (below the BPFL, like
   `CkProbeTrace_Utils` internals): any-hit collector (`JPH::AnyHitCollisionCollector
   <JPH::CollideShapeCollector>`), `NarrowPhaseQuery().CollideShape` with a `JPH::BoxShape`;
   an overload accepting a caller-held `JPH::Ref<JPH::Shape>` for per-voxel-size reuse. No entity
   attribution, no subsystem resolution — callable in a tight loop.
2. `FCk_Jolt_StaticOccupancyFilter` (an `ObjectLayerFilter`: Domain == Static; ~15 lines mirroring
   `FCk_Jolt_DomainQueryFilter`) + a `JPH::BroadPhaseLayerFilter` accepting only
   `broadphase_layers::Static` (the existing wrappers pass accept-all; this is the cheap win).
3. `ck::jolt::Get_BodiesInAABox(const JPH::PhysicsSystem&, const FBox&, filters, TArray<JPH::BodyID>&)`
   — first in-tree use of `GetBroadPhaseQuery().CollideAABox` (currently unused anywhere).
4. Update `Source/CkJolt/Claude.md` (query surface section) — same commit.

Constraints: follow `CkJoltQuery_Utils.cpp` idioms exactly (Conv, filters, formatting); no behavior
change to any existing function; game-thread contract documented on each new function ("safe
off-thread only under a future step-barrier contract — not yet provided").
→ verify: CkJolt compiles; new C++ automation test (unit 0D) exercises all three against a known bake.

### 0C — CkVoxelNav module skeleton (route: opus)

Scaffold by mimicry from CkTimer (copy-rename its 14-file shape; see PROMPT.md layout tree):

1. `CkVoxelNav.Build.cs` (`public class CkVoxelNav : CkModuleRules`; deps per PROMPT.md).
2. `CkVoxelNav_Log.{h,cpp}` (`DECLARE_LOG_CATEGORY_EXTERN(CkVoxelNav, ...)`,
   `namespace ck::voxelnav { CK_DEFINE_LOG_FUNCTIONS(CkVoxelNav); }`).
3. `CkVoxelNav_Module.{h,cpp}` (`IMPLEMENT_MODULE(FCkVoxelNavModule, CkVoxelNav)`).
4. Minimal first feature under `Public/CkVoxelNav/Volume/`: `CkVoxelNavVolume_Fragment_Data.h`
   (`FCk_Handle_VoxelNavVolume` typesafe handle + `FCk_Fragment_VoxelNavVolume_ParamsData` with
   `_VolumeBounds` (FBox) + `_VoxelExtent` (float)), `_Fragment.h/.cpp` (Params bridge alias +
   `FTag_VoxelNavVolume_NeedsBuild`), `_Processor.h/.cpp` (a `Setup` no-op processor,
   `CK_REGISTER_PROCESSOR` at top of .cpp), `_Utils.h/.cpp`
   (`UCk_Utils_VoxelNavVolume_UE : UCk_Utils_Ecs_Base_UE`, `Meta=(ScriptMixin=
   "FCk_Handle_VoxelNavVolume")`, `Add`/`Has`/`CastChecked` following CkPathNetwork's Add-creates-
   child-entity ritual).
5. `CkFoundation.uplugin`: append the standard Runtime/Default/Win64-Mac-Linux entry.
6. `Source/CLAUDE.md`: T4 tier-table row + "I need to…" decision-tree row ("volumetric 3D
   pathfinding for flying/swimming agents → CkVoxelNav").
7. `Source/CkVoxelNav/Claude.md`: purpose, boundary paragraph (vs CkNavigation / CkSpatialQuery /
   CkAStar), MIT attribution note ("core algorithms derived from Nav3D 2.0, © 2025 Darby
   Costello, MIT — LICENSE preserved at docs/campaigns/voxelnav-port/ until the port matures,
   then alongside the module"), phase status pointer to this campaign.

→ verify: editor target compiles with the module enabled; AS boot generates
`Script/Generated/utils_voxel_nav_volume.as` (or equivalently-named) without error.

### 0D — First tests (route: opus; lands in CkTests on its own branch)

1. `git -C Plugins/CkTests checkout -b feature/ckvoxelnav-port` (base: current checked-out SHA —
   record it in PROGRESS.md; do NOT touch other sessions' dirty state there).
2. C++ automation test for 0B (`Test_JoltQuery_BoxOccupancy`): model on
   `UnitTests/CkJolt/Test_JoltBake_*` — assert occupied inside a known baked box, free outside,
   broadphase AABox returns the body.
3. AS (or C++) scaffold autotest for 0C: create entity → `Add` VoxelNavVolume → `Has` true, handle
   valid.
4. Remember: new automation tests need touch+relink; AS autotests need `--discover-fresh`.

## Exit criteria (gate — orchestrator re-runs, never trusts executor green)

- [ ] Full toolbox `--build --test` green on the final artifact; totals **delta-zero vs the
      PROGRESS.md baseline** (plus the new tests, named, green).
- [ ] `rg --no-ignore -l "Jolt/" Source/CkVoxelNav` → zero (no direct JPH includes).
- [ ] CkThirdParty Claude.md allowlist + libmorton row present; CkJolt Claude.md updated.
- [ ] uplugin + Source/CLAUDE.md rows present.
- [ ] PROGRESS.md: baseline table, unit evidence, decisions, session log updated; PHASE_1.md
      authored at the boundary.

## Fences (phase-specific)

- No octree/pathfinding code in this phase — skeleton only.
- Do not modify `CkProbe_Processor.cpp` (the batching fix is a recorded follow-up, not ours).
- Do not adopt `Get_OverlapEntities`/broadphase at existing call sites — expose only.
