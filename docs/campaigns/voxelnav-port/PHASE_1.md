# PHASE 1 — Octree core + Jolt voxelization

> Freshness: authored 2026-08-04 at the 0→1 boundary. Status of record: PROGRESS.md.
> The implementation spec of record is `research/phase1-port-map.md` (§ references below point
> there). Where this file and the port map disagree, PROGRESS.md § Decisions arbitrates —
> [C-D12] adopted the port map's recommendations wholesale, then [C-D14] amended OQ-3
> (static-domain only, no `_QueryChannel`) and [C-D16]/[C-D17] postdate it.

## Entry criteria

- [ ] Phase 0 closed in PROGRESS.md (gate evidence + commits recorded).
- [ ] CkFoundation on `feature/ckvoxelnav-port` (clean), CkTests on `feature/ckvoxelnav-port`
      (clean), superproject carries the [C-D16] uproject fix.
- [ ] Phase 1 baseline = Phase 0's closing gate numbers (recorded in PROGRESS.md).

## Units (dispatch: opus; 1A ∥ 1B, then 1C → 1D; tests split across 1A/1E)

### 1A — Octree core types (`Public/CkVoxelNav/Octree/`)

Port per port-map §1 (type translation table, field-by-field) + §2 (function-level map) for the
pure-data half: Morton addressing (uint64, libmorton), layer/leaf containers, packed-uint32 node
refs, the `FCellId` abstraction (layer nibble 14 reserved for merged cells; search never calls
`Get_NodeAddress()` — the four kind-dispatching free functions are the merge seam), octree
`Initialize` (layer sizing), neighbor-link math. Engine-light: no UObject, no UPROPERTY, no world.
**Carry all six upstream bug fixes named in the port map** (corrupt free-space address result
struct; backwards unpack ctor; cube-vs-edge neighbor bounds; one-past-last layer in GetRandomPoint;
8x-duplicated blocked-parent propagation; full-cube RasterizeLayer scan).
→ verify: hermetic C++ unit tests (part of this unit, `UnitTests/CkVoxelNav/`): Morton roundtrip,
layer sizing vs hand-computed cases, packed node-ref roundtrip incl. the serialization form,
neighbor addressing on a known 2-layer octree. Pattern `Ck.VoxelNav.Octree`.

### 1B — Geometry backend (`Public/CkVoxelNav/Backend/`)

Port-map §2's `ICk_VoxelNav_GeometryBackend` (JPH-free, house-named) + the CkJolt-backed
implementation over `ck::jolt::FCk_Jolt_QuerySession`/`FCk_Jolt_BoxProbe` (Phase 0's surface).
[C-D14]: no channel parameter — static domain only. Include the port-map's ruling to DROP the
L1 overlap cache (the hierarchy provides the pruning; Jolt any-hit replaces the triangle
narrowphase) — the backend exposes exactly what the rasterizer needs, nothing speculative.
→ verify: compiles; backend impl grep-clean of JPH includes; unit-testable via a stub backend
(a lambda/struct test double over a hardcoded box list) proving the interface suffices for 1C.

### 1C — Voxelization pipeline (`Octree/`, builds on 1A+1B)

The port-map §3 14-stage resumable machine: Initialize → gather → FirstPass → leaf rasterization →
layer rasterization → parent/neighbor links, budgeted by probe count (wall-clock only as guard),
stage boundaries as resumable checkpoints. Pure logic operating through the backend interface —
still no ECS/world coupling in this unit (the driver arrives in 1D).
→ verify: hermetic C++ test: bake a synthetic scene via the stub backend (known box layout) and
assert occupancy of hand-picked cells at multiple layers, blocked-parent propagation, and
resume-equivalence (full-budget bake == many-small-budget bake, bit-identical octree).

### 1D — Volume entity + build processor (`Volume/`)

Port-map §5: `FFragment_VoxelNavVolume_BuiltOctree` (+ epoch per CkPathNetwork precedent),
`FTag_VoxelNavVolume_NeedsSetup`/`NeedsBuild` flow re-point (rewrites Phase 0's placeholder Setup —
expected, recorded in 0C's acceptance), `Request_Build` (completion delegate last, AutoCreateRefTerm,
no C++ default), budgeted build processor(s) placed per [C-D11]: `FGroup_Transform`,
`RunAfter FProcessor_JoltWorld_WaitForAsync` + `RunBefore FProcessor_JoltWorld_Step`. Budget knob in
`Settings/` project settings per house shape. Landscape fence per [C-D12]/OQ-4: `WITH_EDITOR`-less
worlds have no Jolt landscape — document + `Get_NumStaticBodies` sanity ensure at build start.
→ verify: compiles; processor registered; the PIE bake test in 1E.

### 1E — PIE bake test (CkTests)

A PIE test (fixture now healthy post-[C-D16]): spawn N static JoltBody boxes in a known layout,
`Add` a VoxelNavVolume over them, `Request_Build`, wait on the completion delegate, then assert
occupancy spot-checks through the volume's query API: cells inside each box occupied, cells in
known free space navigable, count sanity vs the synthetic-scene expectation. Also assert the build
respected its budget (multi-frame completion, no single-frame stall beyond budget — port-map §3's
worked numbers are the reference). Pattern `Ck.VoxelNav.Bake`.

## Exit criteria (orchestrator re-runs everything)

- [ ] Full suite delta-zero vs Phase 1 entry baseline ([C-D17]: full suite + targeted patterns
      `Ck.VoxelNav`, `Ck.Jolt.Query` all green).
- [ ] All 1A/1C hermetic tests + the 1E PIE bake test green, named in PROGRESS.md.
- [ ] `rg --no-ignore -l "Jolt/" Source/CkVoxelNav` → zero, still.
- [ ] Comment audit run over the phase's diff (root doctrine closing step).
- [ ] PROGRESS.md updated (evidence, decisions, session log); PHASE_2.md authored at the boundary.

## Fences

- No pathfinding/search code (Phase 2). No chunking/partitioning (Phase 3). No merging (Phase 5).
- Do not modify CkAStar, CkNavigation, CkCrowd, or existing CkJolt behavior.
- The port map's §6 open questions are RULED ([C-D12]/[C-D14]) — deviations need new decision
  entries, not executor judgment.
- Serialization of the octree is Phase 3 scope (chunk payloads); Phase 1 keeps the octree
  in-memory only — but the packed-uint32 node-ref form lands in 1A because the types carry it.
