# CkIskmRenderer

**Purpose:** Skeletal-mesh rendering for ECS entities. Two paths, selected by `ECk_IskmProxy_PoseSource`:
- **Plan-1 (per-entity `USkeletalMeshComponent`)** — anim sequences, montages, optional AnimBP, ragdoll, modular outfit
  submeshes, per-instance custom data, sockets, line traces, notify events. The fallback for anything the baked path can't express.
- **Plan-2 (batched GPU-skinned instancing)** — N sequence-mode instances share one baked bone-matrix `Buffer<float4>` SRV,
  are skinned in a custom `FVertexFactory`, and draw through cluster `FPrimitiveSceneProxy`(es) via GPUScene instance data.
  A Skelot port. Status: **Phases 0-2 landed** (baker + full render pipeline + N-instance static rendering; compiles/loads/
  renders on GPU, shader verified under --no-nullrhi, 24 autotests green). Per-instance independent animation, ECS `Add`
  routing, LOD/culling, and the SKMC fallback are Phases 3-6 — see `CONTINUATION_PROMPT_Plan2.md`.

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

## See also

- `CkIsmRenderer/Claude.md` — sibling module for instanced static meshes; same shared/per-entity split.
- `CkAnimation/Claude.md` — `FCk_Handle_AnimAsset` for per-entity anim asset records (orthogonal — IskmRenderer is the renderer; AnimAsset is per-entity anim metadata that may drive what gets played).
- `CkStateMachine/Claude.md` — likely caller for `Request_PlayAnimation`.
