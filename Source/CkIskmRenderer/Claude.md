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
`CkIskm_BatchedRenderResources.*` is a port of Skelot v6 `SkelotRenderResources.h`, **GPUScene desktop path only** —
HP float32, no manual-vertex-fetch, no legacy non-GPUScene path, no curves. Shader side:
`Shaders/CkIskmRenderer/CkIskm_BatchedVertexFactory.ush`. The bone-weight stream is OWNED rather than borrowed from
the source mesh specifically to drop every assumption about the source skin-weight layout (variable vs constant
influence layout, 16-bit weights, influence counts that aren't 4/8) — those were the failure modes of the earlier
borrowed-stream approach. `CkIskmRendererVF` ships no `Claude.md` of its own — `Source/CLAUDE.md` names this doc as
its coverage.

**Depends on:** `Core,CoreUObject,Engine,GameplayTags,AnimGraphRuntime,CkCore,CkEcs,CkEcsExt,CkLabel,CkLog,CkGraphics,CkProvider,CkRecord,CkSettings,CkAnimation,CkPhysics,CkUsf`.

**Used by:** Any feature that needs animated skeletal entities without per-actor overhead — NPCs, crowds, pawns spawned via ECS rather than `AActor`.

---

## Key API

- `UCk_Utils_IskmRenderer_UE::Add(InOwner, InRendererData)` — register a shared renderer for a `UCk_IskmRenderer_Data` asset (one renderer actor per *Renderer PDA* per world; multiple Renderer PDAs may share one AnimCollection).
- `UCk_Utils_IskmProxy_UE::Add(InOwner, InParams)` — add a per-entity proxy. Allocates a `USkeletalMeshComponent` ("BaseSKMC") on the manager actor.
- `UCk_Utils_IskmProxy_UE::Request_*` — animation playback, montage, ragdoll, outfit, custom data.
- `UCk_Utils_IskmProxy_UE::TryGet_SocketTransform_CurrentEntityWorld` — current-frame world socket
  composed from the live component-space pose, current entity transform, and proxy render offset;
  unlike the ordinary World-space socket getter, it does not sample the SKMC's stale placement.
- `UCk_Utils_IskmProxy_UE::Request_SetCustomDataFloat_Late` — opt-in custom-data lane consumed in
  `FGroup_DeferredApply`, after `FGroup_PostTransform`; it mirrors the value to the CPU cache, base
  SKMC, and attached submesh SKMCs without changing the normal `Request_SetCustomDataFloat` lane.
- `UCk_Utils_IskmProxy_UE::Get_IsRagdollSettled` — true once the proxy is ragdolling **and** no rigid
  body on its BaseSKMC is still awake, i.e. the body has physically come to rest. Chaos auto-sleeps
  bodies at rest, so this is exact and takes no velocity threshold or tuned timer; a caller gating a
  death/respawn beat on it waits exactly as long as the fall takes. False while not ragdolling — a
  body that never fell has not settled — so it cannot be used to mean "is at rest" in general.
  Note the settle is only reachable if the body has something to land on: in free fall the velocity
  never drops below the sleep thresholds. Companion query: `Get_IsRagdolling`.
- `UCk_Utils_IskmProxy_UE::BindTo_OnAnimationFinished/OnAnimationNotify/OnMontageFinished` — ECS signals fired by the bridging `UCk_IskmNotify_AnimInstance`.
- **Entity-level outline (Plan-1):** driven by `CkUsf`'s `UCk_Utils_Usf_Outline_UE::Request_ApplyOutline(Handle, Preset, Scope)`.
  Sets Custom Depth/Stencil on the proxy's BaseSKMC **and** every outfit submesh, re-asserted per frame so submeshes
  attached later inherit it. Test getter: `UCk_Utils_IskmProxy_UE::Get_IsOutlineApplied`.
- **Entity-level cel pattern (Plan-1):** driven by `CkUsf`'s
  `UCk_Utils_Usf_CelPattern_UE::Request_SetCelPattern(Handle, Pattern, Scope)`. The outline processors' twin —
  Custom Depth/Stencil on the BaseSKMC and every outfit submesh, re-asserted per frame — minus the stencil
  allocation, because the cel contract is a direct value. Mutually exclusive with the outline per entity (the
  two write the same byte); an outline arriving over a pattern drops the cel applied-state WITHOUT clearing
  the flags, since the outline's own Sync overwrites the byte in the same group. Test getters:
  `UCk_Utils_IskmProxy_UE::Get_IsCelPatternApplied` / `Get_CelPatternStencilValue`.
- **Entity-level stylize effect mask (Plan-1):** driven by `CkUsf`'s
  `UCk_Utils_Usf_StylizeMask_UE::Request_AddToStylizeMask(Handle, Scope)`. The cel-pattern processors'
  twin, minus the per-entity payload — the project reserves ONE stencil value, so `_Sync` reads
  `UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue()` rather than consulting a subsystem. Precedence
  is outline > cel pattern > mask, so its Sync excludes both and the drop-applied step is TWO processors
  (a view is a conjunction; "a higher claim arrived" is a disjunction). Flags are NOT cleared on a drop,
  for the reason the cel twin carries.
- **Member-level outline (Plan-2, batched):** `UCk_Utils_IskmBatched_UE::Set_CrowdMemberOutline/Clear_CrowdMemberOutline/
  Get_CrowdMemberOutlinePreset/Get_CrowdOutlinedMemberCount/Get_CrowdOutlineRenderedInstanceCount` — index-based (batched
  members aren't entities). See *Plan-2 production guide → Outline (highlight cluster)* below.
- **Member-level cel pattern (Plan-2, batched):** `UCk_Utils_IskmBatched_UE::Set_CrowdMemberCelPattern/
  Clear_CrowdMemberCelPattern/Get_CrowdMemberCelPatternOr/Get_CrowdMemberCelPatternStencilValue/
  Get_CrowdCelPatternedMemberCount/Get_CrowdCelPatternRenderedInstanceCount` — the outline's twin on the same
  highlight-cluster machinery, keyed on the stencil VALUE. See *Plan-2 production guide → Outline (highlight
  cluster)* below.
- **Member-level stylize effect mask (Plan-2, batched):** `UCk_Utils_IskmBatched_UE::Set_CrowdMemberStylizeMask/
  Clear_CrowdMemberStylizeMask/Get_IsCrowdMemberStylizeMasked/Get_CrowdStylizeMaskedMemberCount/
  Get_CrowdStylizeMaskRenderedInstanceCount` — the same machinery a precedence level further down. A masked
  member records NO stencil of its own (the project reserves one value), so its group is found by membership
  scan rather than by key — re-resolving the project value at clear time would strand the cluster the member
  is actually in if the setting moved mid-session. Both `Set_MemberOutline` and `Set_MemberCelPattern` clear
  the mask on their success paths, the same silent downward claim the outline already makes over the pattern.

---

## Pattern

Mirror of `CkIsmRenderer` — Renderer (shared per-AnimCollection) + Proxy (per-entity). The Proxy's `BaseSKMC` is owned by the per-world `ACk_IskmRenderer_Actor_UE` manager. Outfit submeshes are child SKMCs with `LeaderPoseComponent` set to the BaseSKMC.

---

## Anti-patterns

1. Don't drive animations on the SKMC directly — route through `Request_*` so processors manage state and emit signals.
2. Don't create your own SKMC — always use `Add(...)` to allocate one from the manager pool.
3. Don't replicate animation state from this module — the caller (StateMachine, gameplay processor) replicates *its* state and re-issues `Request_*` on remotes. Recommended pattern: state machine replicates its current state enum, and on `OnRep_State` re-issues `Request_PlayAnimation` / `Request_PlayMontage`.
4. AnimBP authors **must** derive their AnimInstance class from `UCk_IskmNotify_AnimInstance` for `OnAnimationNotify` / `OnMontageFinished` signals to fire on those entities. The Setup processor logs a warning when it detects a non-derived class.
5. Don't skip the unconditional Custom Depth/Stencil clear in `Release_BaseSKMC` — a pooled SKMC must never carry
   outline state to its next borrower, regardless of outline-processor bookkeeping order (see *Notes* below).
6. Never call `SKMC->Stop()` in `Release_BaseSKMC`. On a component running an AnimBP or the notify bridge, `Stop()`
   logs the engine's *"Currently in Animation Blueprint mode"* Warning, which the AutoTest harness escalates to a
   failure for any proxy destroyed mid-test. `SetAnimInstanceClass(nullptr)` destroys the live instance outright
   (single-node or AnimBP) and halts playback either way.
7. Don't call `SetAnimInstanceClass(nullptr)` unguarded in the `PlayAnimation` handler — it is behind
   `if (SKMC->AnimClass != nullptr)` on purpose. With `AnimClass` already null it tears down the existing
   SingleNodeInstance that `PlayAnimation` is about to recreate; the momentary teardown leaves the render proxy
   half-initialized and the SKMC visibly snaps to ref pose (reproduced by re-issuing `Request_PlayAnimation` from a
   timer in the TransitionCycle gym station).

---

## Notes

- `Add(InOwner, InRendererData)` calls `LoadSynchronous` on the Renderer PDA's `_DefaultAnimInstanceClass` (a soft class ref) on first use — this can cause a brief hitch the first time an entity is added against a fresh Renderer PDA. Pre-warm by issuing the first `Add` outside a hot path (e.g. during level setup).
- `Release_BaseSKMC` unconditionally clears Custom Depth + stencil value before returning a SKMC to the pool. This is
  pool hygiene, not outline bookkeeping — even if the outline EndPlay processor ran out of order or never ran, the
  next borrower must never inherit a stale silhouette.

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

**Proxy request assets are soft refs** (`FCk_Request_IskmProxy_PlayAnimation::_Sequence`, `_PlayMontage::_Montage`, `_SetMaterialOverride::_Material`, `_SetSkeletalMesh::_Mesh`): a fragment-held `TObjectPtr` roots nothing (UE GC never walks the EnTT registry), so each request carries a `TSoftObjectPtr` plus a non-reflected CkResourceLoader `RootedAssetBatch` kicked at the Utils enqueue boundary (consumer id `"IskmProxy.Requests"` — flip it to Synchronous per-project in the ResourceLoader settings for debugging). Resident assets complete inline, so the warm path drains same-tick as before; a cold load stalls the WHOLE per-proxy queue (order preserved — nothing overtakes a loading request) and marks the entity `FTag_IskmProxy_PendingAssetLoad` (observability only) until the batch lands. A failed load completes the request `Failed`. Post-apply rooting is owned by the receivers (SKMC / AnimInstance / the `TStrongObjectPtr` override map), so the batch releases with the consumed request.

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

**Render profiles and stable buckets.** A crowd may receive an ordered `UCk_IskmRenderer_Data` set exactly once,
before its first member, through `Set_CrowdRenderProfiles`. Every member starts in profile 0 and may move through
`Set_CrowdMemberRenderProfile`; the return value is the acceptance boundary. The renderer keys components by
`(tile X, tile Y, profile index)`, while `_Members` remains the only identity/state owner. Migration prepares the
destination bucket before mutation, rebuilds old and new buckets, and preserves transform, animation phase,
custom data, visibility, cosmetics, and highlight membership. Empty buckets are intentionally retained to avoid
component churn when members cross a distance boundary repeatedly.

Profiles overwrite component state rather than patching it. SKMC bases and children, plus batched cluster
components/proxies, consume shadow/contact-shadow, main/depth, decals, occluder/custom-depth,
dynamic-indirect/distance-field lighting, ray-tracing visibility, lighting channels, bounds, min/max draw distance,
minimum LOD, material overrides, and velocity participation. `_FarAnimationUpdateInterval` throttles far pose
evaluation/uploads without losing elapsed animation time; `_FreezeFarAnimation` intentionally holds the far pose.
The batched proxy forwards both `ShouldRenderInMainPass()` and `ShouldRenderInDepthPass()` into
`FPrimitiveViewRelevance`; setting a profile's depth-pass flag on the component alone is not enough.
Profile animation collections must exactly match the crowd collection. Invalid profile sets or member/profile
indices are diagnosed and rejected by ordinary fail-closed control flow even when ensures compile out.

Material precedence is: whole-crowd override, profile base-slot overrides, crowd slot overrides, mesh defaults.
Turning off dynamic-indirect/distance-field participation alone does not remove direct-light material work; use a
skeletal-compatible cheap/unlit profile material when the goal is to remove that shader cost.

**Outline (highlight cluster).** `Set_CrowdMemberOutline(i, preset)` stands up one custom-depth-only
`UCk_Iskm_BatchedClusterComponent` ("highlight cluster") per (crowd, preset) — same flags as a tile, but it holds only
the outlined members' mirrored `FInstance` data and is pushed every manager tick alongside the tile clusters, so the
silhouette tracks the live skinned pose. Membership/visibility changes rebuild it (`Rebuild_HighlightGroup`); fixed bounds
= union of outlined members' world positions padded by the animated mesh box + half a tile. Hidden (Plan-1-flipped)
members are excluded — the flip driver outlines their Plan-1 SKMC via `CkUsf`'s entity API instead. **Gotcha:** unlike
tile clusters (whose local bounds box is centered on the component, so component rotation is a no-op), the highlight
cluster's bounds box is authored in *world* space from the members' actual positions. It **must** use absolute
location/rotation/scale pinned to identity (`SetUsingAbsoluteLocation/Rotation/Scale(true)` +
`SetWorldTransform(Identity)`) — if it inherits the crowd actor's transform (e.g. a spawn-time yaw), `CalcBounds`
rotates the box away from where the instances actually render and the cluster frustum-culls out at some view angles
even though the members are on screen.

**Cel pattern (the same highlight cluster).** `Set_CrowdMemberCelPattern(i, pattern)` is the outline's twin and
shares every line of that machinery — `DoCreate_HighlightCluster` is the ONE construction path, and
`Rebuild_HighlightGroup` / `Push_HighlightGroup` / the bounds rule / the hidden-member exclusion are common to
both. Deltas, all forced by the cel contract being a DIRECT stencil value rather than a refcounted preset
allocation: the group key is the resolved Custom-Stencil value (`Get_StencilValueFor(pattern)`, so two patterns
are two clusters), nothing is allocated or released, and `Clear_CrowdMemberCelPattern` is the only clear (there
is no null-pattern sentinel the way a null preset clears an outline). A member carries at most ONE of the two —
they write the same stencil byte: **the outline wins**, clearing an existing pattern silently on its way in,
while a pattern applied to an already-outlined member is refused loudly with zero mutation (the entity-level
`Request_SetCelPattern` refusal, member-indexed). The member records the group key it joined beside the pattern
it asked for, so a `StencilBase` change between Set and Clear still resolves the group it is actually in.

**Scoped follow-up (documented, not implemented):** per-member sequence **crossfade** (shader 2-frame lerp).
Requires widening per-instance custom data (floats [0..3] are all taken: curFrame/prevFrame/userA/userB — a blend
alpha + held source frame need `NumCustomDataFloats=8` or repacking), a blend-state advance in the manager tick,
and a `Lerp(CalcBoneMatrix(a), CalcBoneMatrix(b))` path in `CkIskm_BatchedVertexFactory.ush`. Close-up transition
quality is currently covered by the SKMC flip (promoted members blend natively); distant 30Hz sequence pops are
the standard crowd tradeoff (Skelot's transitions are likewise opt-in).

---

## Implementation notes

**Bake provenance and gating.** The CPU bone-matrix bake is a direct port of Skelot's `FSkelotAnimationBuffer`
(`SkelotAnimCollection.cpp` `CalcRenderMatrices`), and `FCk_Iskm_BakedSequence` mirrors `FSkelotSequenceDef`'s
render-relevant fields. Sampling/compaction/layout/bounds were since extracted into the shared `ck::anim_bake` core
(`CkAnimation/AnimBake`, also consumed by `CkVat`); this module keeps only the output encoding — transposed 3x4
matrices in a flat `Buffer<float4>`-ready array. Deltas from Skelot: always float32 (Skelot defaults to float16), and
`TotalFrameCount == FrameCountSequences` because there is no transition / dynamic-pose region yet. The output is
asset-intrinsic, transient (rebuilt, never serialized) and CPU-only, so it runs headlessly under `-nullrhi`; the SRV
upload is a separate step. **`Build_BakedPoseData` therefore gates on `IsRunningDedicatedServer()`, deliberately NOT
`FApp::CanEverRender()`** — only a real dedicated server legitimately lacks the mesh render data the bake reads
(cook-stripped, and unneeded since `AdvanceAnimation` already skips `NM_DedicatedServer`). A `-nullrhi`/headless run is
not a dedicated server and the bake succeeds there, but `CanEverRender()` is false in both cases: gating on it silently
starved listen-server / standalone / client-under-nullrhi callers of a bake `AdvanceAnimation` needs every tick, so its
"no baked pose" ensure fired continuously for the rest of the run.

**Processor scheduling (Plan-1).** `FProcessor_IskmRenderer_Setup` sits in `FGroup_Gameplay_Audio`, one phase EARLIER
than `FProcessor_IskmProxy_Setup`: sharing `FGroup_Gameplay_Rendering` would leave registration order deciding, and the
proxy registers first — it would poll `_RendererActor` before Setup assigned it. `FProcessor_IskmProxy_Setup::DoTick`
caches the world pointer once per tick (`mutable _World`) so `ForEachEntity` doesn't re-resolve it per proxy per frame.

**SocketFollower sampling.** The follower's world transform is `Offset x Socket(component-space) x
LeaderEntityTransform`, recomputed after the Transform request pass. Sampling the SKMC's WORLD-space socket instead (as
a `SyncFrom`-group processor would) reads the leader's previous-frame position — the SKMC only moves at PostTransform —
so the follower trails by a frame of velocity; component-space sampling leaves only the animation pose a frame stale
(sub-cm) while the root-motion term stays current. The ragdoll case inverts this: physics owns the SKMC,
`UpdateTransform` is excluded, and re-anchoring the live component-space socket onto the frozen death-pose root
produced the hair-detach bug.

`TryGet_SocketTransform_CurrentEntityWorld` exposes the same non-ragdoll composition without the follower offset and reports invalid handles/sockets explicitly:
`Socket(component-space) x (LeaderEntityTransform + rotated LocalLocationOffset)`. It validates the handle,
Transform feature, socket/bone, and every composed numeric input before returning. BaseSKMC absence during deferred
renderer setup or EndPlay teardown is ordinary `TryGet` unavailability and returns false with identity output; malformed
state ensures and returns identity without consulting or mutating the rejected object further. Physics-owned ragdoll
callers that need the live simulated pose should continue to use `Get_SocketTransform(..., World)`.

**Late custom-data lane.** `Request_SetCustomDataFloat_Late` writes a distinct transient request fragment consumed in
`FGroup_DeferredApply`, whose group dependency places it after the complete `FGroup_PostTransform`. The general
`Request_SetCustomDataFloat` lane remains in `FGroup_Gameplay_Rendering`. Both retain independent FIFO order and
completion guards; EndPlay cancels either lane with `Failed_Cancelled`. A callback that enqueues into the lane being
drained survives for the next frame. If both lanes target the same slot in one frame, the late lane wins by group order.

**SocketFollower group placement (incident).** `FProcessor_IskmProxy_SocketFollower_SyncTransform` runs in
`FGroup_Transform_Finalize`, after the ENTIRE `FGroup_Transform` — not merely `RunAfter
FProcessor_Transform_HandleRequests`, which was the original lag-free fix. The follower reads the leader transform
through a runtime lookup the scheduler cannot see, so it carries no view-dependency ordering on either mover; the plain
`RunAfter` left it racing `TProcessor_SceneNode_Update`, and a scene-node-driven leader (e.g. a promoted NPC's proxy
inheriting the agent's per-frame movement) was read one frame stale, so the cosmetic trailed the moving body. Finalize
still precedes `FGroup_PostTransform`, so the renderer flush picks up `FTag_Transform_Updated` the same frame.
`..._SyncDescendants` is the companion: because SyncTransform writes AFTER `TProcessor_SceneNode_Update` has run,
scene-node CHILDREN parented under a follower's output (a held item under a hand attach-point, plus that item's own
probe children) would see the follower's `FTag_Transform_Updated` one group too late — `Transform_Cleanup` wipes the tag
before the next frame's gate check, so after their construct-time one-shot they froze at the follower's equip-time pose.
SyncDescendants recomputes the follower's scene-node descendant subtree in place (same composition + anchor-skip
contract as `TProcessor_SceneNode_Update`) and runs before `FProcessor_Transform_SyncToActor` so the recomputed poses
land on their actors the same frame.

**Plan-1 → Plan-2 migration shape.** Two fragments look over-decomposed because of where they're going:
`FFragment_IskmProxy_Current`'s `TWeakObjectPtr<USkeletalMeshComponent>` is slated to become
`int32 _InstanceIndex + uint32 _InstanceVersion` — an SOA index into the renderer's instance arrays, with no per-entity
SKMC for sequence-mode entities — hence the rule that `_BaseSKMC` access never leaves `UCk_Utils_IskmProxy_UE` and the
proxy processors. `FFragment_IskmProxy_PoseSource` stays a separate fragment (never merged into AnimState) because it is
slated to become a tag-per-source (`FTag_IskmProxy_PoseSource_Sequence/_AnimBP/_Ragdoll`) so the cluster-update processor
can `TInclude`/`TExclude` by pose source without reading every entity's fragment. Unrelated: the `::` qualification on
`::UCk_IskmNotify_AnimInstance` / `::UCk_Utils_IskmRenderer_UE` inside `ck_iskmproxy_processor` is a holdover from when
those helpers lived in `namespace ck`, where `CkIskmProxy_Fragment.h`'s friend-class declarations inject a shadowing
forward decl; in the file-local namespace unqualified lookup resolves fine and the qualifier is retained as harmless.

**Editor-only renderer/crowd splitting.** Per-owner renderer actors exist because selection redirect
(`IsSelectionChild`/`GetSelectionParent`) is actor-level, so one renderer per (data asset, selection owner) IS the
instance-to-spawner mapping; the same applies to preview crowds, since a shared crowd cannot attribute tile instances to
owners at actor level. SKMC release stays owner-derived (`SKMC->GetOwner()`), so proxies need no extra teardown
bookkeeping, and runtime worlds always use the shared renderer. Editor worlds also freeze SKMC poses behind
`bUpdateAnimationInEditor` (the `RefreshBoneTransforms` gate) — `VisibilityBasedAnimTickOption` does nothing there — so
`EditorOnly_EnableAnimationTicking` must be applied to every acquired base SKMC and every leader-pose submesh child.

**Plan-1 per-proxy override scope (V1).** Material overrides apply to the BASE SKMC only — submeshes keep their def-time
override materials (`FCk_IskmRenderer_MeshDesc::_OverrideMaterials`) and static cosmetics are CkIsm-side. Morph targets
likewise apply to the BASE body mesh only: `LeaderPoseComponent` copies bone transforms, not morph curves, so outfit
submeshes do NOT inherit them. Both reset to mesh defaults when the proxy returns its SKMC to the pool, so nothing leaks
across borrowers.

**Plan-2 material validation (incident).** `UCk_Iskm_BatchedClusterComponent::Set_OverrideMaterial` /
`Set_SlotOverrideMaterials` route every pointer through a validated cast, against two observed failure modes: (1) a
wrong-typed object arriving through the AS boundary (`LoadAsset_Blocking` does no runtime class check) — a non-material
stored here access-violates the base `FPrimitiveSceneProxy` ctor's material scan on the next proxy recreate; (2) a
dangling/GC-collected pointer — non-null but with a null class pointer, so `Cast<>` AVs while dereferencing the class to
test the type (seen via the crowd LOD-flip passing a GC'd override material). Hence liveness (`ck::IsValid`, which
inspects only the flags/registry slot and is safe on pooled memory) is checked BEFORE `Cast<>`; either failure ensures
loudly and stores null, letting mesh defaults take over.

**Plan-2 bounds (incident).** In `SendRenderDynamicData_Concurrent` / `BuildDynData`, `LocalBounds` is the PER-INSTANCE
bound (one animated-pose box the engine applies per instance transform; the shared 1-entry array is valid because
`GetInstanceLocalBounds` clamps 1-or-N), while `LocalBoundsSphere` is the PRIMITIVE bound and must cover the WHOLE
instance spread. Using the single mesh box for the primitive bound collapsed the primitive's scene bounds to one mesh at
the component origin every animated frame — `FUpdateTransformCommand` then replaced the registration bounds, so
everything away from the tile centre was wrongly frustum/occlusion culled ("flickers unless looking at the spawn point").

**Plan-2 tile rebuild.** `RebuildTile` reads live `FMember` state, so a rebuild never snaps animation back — that is what
fixed the old "whole tile resets to spawn pose on any flip" bug. Crowd `Initialize` bakes the AnimCollection up front
(CPU-only, headless-safe, idempotent, and it rebuilds a stale bake) because tile fixed bounds derive from the baked
ANIMATED pose extent: a tile created pre-bake freezes the smaller static mesh box.

**Plan-2 ECS clock (incident).** The crowd actor no longer self-ticks (`bCanEverTick = false`). It historically advanced
members in its own `AActor::Tick` — a second clock, unordered against the ECS world-actor tick — which left far cosmetics
(ECS entities synced by the transform pipeline) up to a frame behind the member they follow. Now one controller entity
per crowd carries `ck::FFragment_IskmCrowd_Controller` and `FProcessor_IskmCrowd_Advance` ticks it in
`FGroup_Transform_SyncFrom`: AFTER the flip driver's `Gameplay_Script` member-world writes (member is current, not
agent-lagged) and BEFORE `FGroup_Transform`'s HandleRequests (so a cosmetic's `Request_SetTransform` — the deferred
cross-entity write, safe against scheduler write-ordering — lands the SAME tick). Member `PushTile` and cosmetic both
reach the `FGroup_PostTransform` render flush the same frame.

**Plan-2 game-side custom-data layout.** Shader per-instance floats [0]/[1] are the animation frame bits (rejected for
game writes); [2..15] map to `FInstance::UserData[0..13]` and are owned by the game. BusterBlock's layout (see BB
`DESIGN_IskmCosmeticParity.md`): [2..9] slot indices, [10..12] skin RGB, [13..15] spare.

**Plan-2 Skelot provenance.** `UCk_Iskm_BatchedClusterComponent` is the analogue of Skelot's `USkelotClusterComponent`;
`FCk_Iskm_BatchedClusterProxy` is a port of Skelot v6 `FSkelotClusterProxy`, single-mesh, GPUScene desktop only.
Per-instance GPU occlusion (`AllowInstanceCullingOcclusionQueries`) is Skelot parity and feeds `FPrimitiveSceneProxy`
scene-data flags.

---

## See also

- `CkIsmRenderer/Claude.md` — sibling module for instanced static meshes; same shared/per-entity split.
- `CkAnimation/Claude.md` — `FCk_Handle_AnimAsset` for per-entity anim asset records (orthogonal — IskmRenderer is the renderer; AnimAsset is per-entity anim metadata that may drive what gets played).
- `CkStateMachine/Claude.md` — likely caller for `Request_PlayAnimation`.
- `CkUsf/Claude.md` — entity-level outline request API, plus its *Entity outlines* section (the outline
  architecture across ISM/ISKM Plan-1/ISKM Plan-2).
