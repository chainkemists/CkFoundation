# CkIskmRenderer

**Purpose:** Skeletal-mesh rendering for ECS entities. Two paths, selected by `ECk_IskmProxy_PoseSource`:
- **Plan-1 (per-entity `USkeletalMeshComponent`)** — anim sequences, montages, optional AnimBP, ragdoll, modular outfit
  submeshes, per-instance custom data, sockets, line traces, notify events. The fallback for anything the baked path can't express.
- **Plan-2 (batched GPU-skinned instancing)** — N sequence-mode instances share one baked bone-matrix `Buffer<float4>` SRV,
  are skinned in a custom `FVertexFactory`, and draw through cluster `FPrimitiveSceneProxy`(es) via GPUScene instance data.
  A Skelot port, production-hardened for open-world crowds (see **Plan-2 production guide** below). Status:
  **feature-complete** — baker; GPU render pipeline; per-instance independent animation; spatial tile clustering with
  fixed conservative bounds; per-instance GPU occlusion culling; **moving members** (in-tile light pushes + cross-tile
  migration); manager-owned single-source-of-truth animation state; 4/8-bone-influence skinning with owned renormalized
  weights; baked animated culling bounds; per-member material custom data; real motion vectors; dedicated-server tick
  gates; and the distance-LOD SKMC flip (nearest members leave their batched tile for a real per-SKMC Plan-1 proxy —
  ragdoll/montage — then return). Verified headlessly under `--no-nullrhi` (27 autotests incl. tiling/movement/flip
  bookkeeping); visual verification lives in the `IskmRenderer Batched` gym + `Batched Stress (Moving 600)` gym.

**Plan-2 module layout:** the vertex factory + render resources live in a separate engine-only module `CkIskmRendererVF`
(LoadingPhase `PostConfigInit`) so the VF type registers before the engine seals its vertex-factory list; the rest
(proxy, component, AnimCollection GPU upload) stays in `CkIskmRenderer` (Default).

**Depends on:** `Core,CoreUObject,Engine,GameplayTags,AnimGraphRuntime,CkCore,CkEcs,CkEcsExt,CkLabel,CkLog,CkGraphics,CkProvider,CkRecord,CkSettings,CkAnimation,CkPhysics`.

**Used by:** Any feature that needs animated skeletal entities without per-actor overhead — NPCs, crowds, pawns spawned via ECS rather than `AActor`.

---

## Key API

- `UCk_Utils_IskmRenderer_UE::Add(InOwner, InRendererData)` — register a shared renderer for a `UCk_IskmRenderer_Data` asset (one renderer actor per *Renderer PDA* per world; multiple Renderer PDAs may share one AnimCollection).
- `UCk_Utils_IskmProxy_UE::Add(InOwner, InParams)` — add a per-entity proxy. Allocates a `USkeletalMeshComponent` ("BaseSKMC") on the manager actor.
- `UCk_Utils_IskmProxy_UE::Request_*` — animation playback, montage, ragdoll, outfit, custom data.
- `UCk_Utils_IskmProxy_UE::BindTo_OnAnimationFinished/OnAnimationNotify/OnMontageFinished` — ECS signals fired by the bridging `UCk_IskmNotify_AnimInstance`.

---

## Pattern

Mirror of `CkIsmRenderer` — Renderer (shared per-AnimCollection) + Proxy (per-entity). The Proxy's `BaseSKMC` is owned by the per-world `ACk_IskmRenderer_Actor_UE` manager. Outfit submeshes are child SKMCs with `LeaderPoseComponent` set to the BaseSKMC.

---

## Anti-patterns

1. Don't drive animations on the SKMC directly — route through `Request_*` so processors manage state and emit signals.
2. Don't create your own SKMC — always use `Add(...)` to allocate one from the manager pool.
3. Don't replicate animation state from this module — the caller (StateMachine, gameplay processor) replicates *its* state and re-issues `Request_*` on remotes. Recommended pattern: state machine replicates its current state enum, and on `OnRep_State` re-issues `Request_PlayAnimation` / `Request_PlayMontage`.
4. AnimBP authors **must** derive their AnimInstance class from `UCk_IskmNotify_AnimInstance` for `OnAnimationNotify` / `OnMontageFinished` signals to fire on those entities. The Setup processor logs a warning when it detects a non-derived class.

---

## Notes

- `Add(InOwner, InRendererData)` calls `LoadSynchronous` on the Renderer PDA's `_DefaultAnimInstanceClass` (a soft class ref) on first use — this can cause a brief hitch the first time an entity is added against a fresh Renderer PDA. Pre-warm by issuing the first `Add` outside a hot path (e.g. during level setup).

---

## Async loading

`Add(...)` requires a fully-loaded `UCk_IskmRenderer_Data*` (which transitively requires the AnimCollection it references to be loaded). The renderer never blocks the calling thread on load. For soft references, callers handle the async load themselves and call `Add` once the asset is resident:

```cpp
auto& Streamable = UAssetManager::GetStreamableManager();
Streamable.RequestAsyncLoad(SoftPath, FStreamableDelegate::CreateLambda([Handle, SoftPath]()
{
    auto* Loaded = Cast<UCk_IskmRenderer_Data>(SoftPath.ResolveObject());
    if (ck::IsValid(Loaded) && ck::IsValid(Handle))
    {
        UCk_Utils_IskmRenderer_UE::Add(Handle, Loaded);
    }
}));
```

`FTag_IskmRenderer_PendingAsyncLoad` is reserved for forward-compat. Plan-1 never sets it (the `Add` API takes a hard pointer), but the Setup processor clears it alongside `FTag_IskmRenderer_NeedsSetup` so a future `Request_AddAsync(...)` helper can mark entities as pending without needing to update Setup.

---

## Plan-2 production guide (BusterBlock crowds: 130 NPCs + hundreds ambient)

**Architecture.** One `ACk_Iskm_BatchedCrowd_Actor` per (AnimCollection, world) owns the crowd: members are
registered once (`AddInstance` … `Finalize`) and live in `_Members` — the **single source of truth** for transform,
animation time/frames, visibility, and custom data. The manager ticks all members itself and pushes per-frame data
into per-tile `UCk_Iskm_BatchedClusterComponent`s (self-tick disabled); each tile is one GPUScene proxy at the tile
centre with **fixed conservative bounds** (tile extent + baked animated pose box), so member movement never
recomputes bounds. Rendering is client-local: no replication, and all ticking is skipped on dedicated servers.

**Driving members from game systems (the production API, all AS-exposed via `UCk_Utils_IskmBatched_UE`):**
- `Set_CrowdMemberTransform(i, xf)` — every tick if the NPC moves. In-tile: rides the light `Push_LiveInstances`
  path (no proxy recreate, no bounds work). Tile-border crossing: automatic migration (two `Set_Instances`
  rebuilds; velocity zeroed for one frame to avoid cross-primitive TAA smear).
- `Set_CrowdMemberAnimation(i, seq, rate, resetTime)` — idle→walk etc. Sequence switches are instant (no
  crossfade — see *Scoped follow-up*). Time is preserved unless reset.
- `Set_CrowdMemberCustomData(i, a, b)` — shader per-instance custom-data floats **[2]/[3]** ([0]/[1] carry the
  frame indices). Material variety (tint/outfit masks) via the `PerInstanceCustomData` material node.
- `Set_CrowdMemberVisible(i, false)` + spawn a Plan-1 `IskmProxy` at the member's transform — the distance-LOD /
  gameplay flip (ragdoll, montage, sockets). Reverse to return to batched. Hidden members keep advancing time, so
  they rejoin in phase. See the Flip gym station for the reference orbit (hysteresis + promote cap).
- Mesh **variety**: one crowd actor per AnimCollection — use several collections (one per character mesh) for
  visual variety; they batch independently.

**Content requirements (ensure-guarded at `EnsureRenderResources`):**
- ≤ 8 bone influences per section (4- and 8-influence vertex factories; weights renormalized into an owned 8-bit
  buffer — >8 keeps the strongest 8).
- Cooked builds: crowd meshes need **`bNeedsCPUAccess`** (the bake reads skin weights on the CPU).
- Sequences are baked at `_SampleFrequency` (default 30Hz) into the shared SRV; memory =
  `frames × renderBones × 48B`.

**Culling.** Per-tile frustum culling (tight fixed bounds) + per-instance GPU frustum/HZB occlusion culling
(`AllowInstanceCullingOcclusionQueries` — instances behind occluders are culled individually). Culling bounds use
`Get_AnimatedMeshBounds()` (baked bone-union + ref-pose skin pad), not the raw mesh box.

**Tuning knobs:** tile size (`Initialize(_, TileSize)`; ~2000-2500cm — smaller = better culling granularity, more
draw batches), promote/demote distances + cap in the flip driver, `_SampleFrequency` on the AnimCollection.

**Scoped follow-up (documented, not implemented):** per-member sequence **crossfade** (shader 2-frame lerp).
Requires widening per-instance custom data (floats [0..3] are all taken: curFrame/prevFrame/userA/userB — a blend
alpha + held source frame need `NumCustomDataFloats=8` or repacking), a blend-state advance in the manager tick,
and a `Lerp(CalcBoneMatrix(a), CalcBoneMatrix(b))` path in `CkIskm_BatchedVertexFactory.ush`. Close-up transition
quality is currently covered by the SKMC flip (promoted members blend natively); distant 30Hz sequence pops are
the standard crowd tradeoff (Skelot's transitions are likewise opt-in).

---

## See also

- `CkIsmRenderer/Claude.md` — sibling module for instanced static meshes; same shared/per-entity split.
- `CkAnimation/Claude.md` — `FCk_Handle_AnimAsset` for per-entity anim asset records (orthogonal — IskmRenderer is the renderer; AnimAsset is per-entity anim metadata that may drive what gets played).
- `CkStateMachine/Claude.md` — likely caller for `Request_PlayAnimation`.
