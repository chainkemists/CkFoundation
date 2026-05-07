# CkIskmRenderer

**Purpose:** Skeletal-mesh rendering for ECS entities. Per-entity `USkeletalMeshComponent` (Plan-1; Plan-2 will add a batched cluster proxy with a GPU pose buffer). Supports anim sequences, montages, optional AnimBP, ragdoll, modular outfit submeshes, per-instance custom data, sockets, line traces, and notify events.

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

- `Add(InOwner, InAnimCollection)` calls `LoadSynchronous` on the AnimCollection's default AnimInstance class on first use — this can cause a brief hitch the first time an entity is added against a fresh AnimCollection. Pre-warm by issuing the first `Add` outside a hot path (e.g. during level setup).

---

## See also

- `CkIsmRenderer/Claude.md` — sibling module for instanced static meshes; same shared/per-entity split.
- `CkAnimation/Claude.md` — `FCk_Handle_AnimAsset` for per-entity anim asset records (orthogonal — IskmRenderer is the renderer; AnimAsset is per-entity anim metadata that may drive what gets played).
- `CkStateMachine/Claude.md` — likely caller for `Request_PlayAnimation`.
