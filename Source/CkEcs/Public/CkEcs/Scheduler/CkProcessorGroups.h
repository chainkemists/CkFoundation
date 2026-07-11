#pragma once

#include "CkProcessorTraits.inl.h"

// --------------------------------------------------------------------------------------------------------------------
// Group processors define the top-level ordering of the ECS pipeline.
// Individual processors declare `using Group = FGroup_X;` to join a group.
//
// Pipeline order (all TG_PrePhysics unless noted):
//
//   FGroup_DestructionPipeline  (OwningActor_Destroy, DestructionPhase_Finalize, DestructionPhase_Await — start-of-tick destruction completion for entities already past EndPlay)
//     → FGroup_Gameplay_TimeDelta  (Timer, Tween, Substep)
//       → FGroup_Gameplay  (core gameplay: Interaction, Inventory, Objectives, etc.)
//         → FGroup_Gameplay_AI  (StateMachine)
//           → FGroup_Gameplay_Audio  (AudioDirector, AudioTrack, Sfx, Vfx, VfxCue)
//             → FGroup_Gameplay_Rendering  (ISM, PMG, Shapes, RenderStatus, Camera)
//               → FGroup_Gameplay_Script  (EntityScript construction + BeginPlay)
//                 → FGroup_Gameplay_Chaos  (GeometryCollection destruction)
//                   → FGroup_Physics
//                     → FGroup_Transform_SyncFrom  (SyncFromActor, SyncFromMeshSocket, Interpolation)
//                       → FGroup_Transform  (HandleRequests, SceneNode, Tween)
//                         → FGroup_Transform_Finalize  (SyncToActor, FireSignals)
//                           → FGroup_Gameplay_Camera (Camera compose/POV/apply — reads anchors AFTER they are synced this frame)
//                             → FGroup_PostTransform (OverlapBody, RaySense, UI, etc.)
//                               → FGroup_Hydration (replicated-fragment + save-load hydration dispatch — applies payloads after composition, before replication-complete fires)
//                                 → FGroup_Replication
//                               → FGroup_EntityLifecycle (entity create + DestructionPhase_Endplay — adds the EndPlay tag)
//                                 → FGroup_EndPlay (all feature-level *_EndPlay processors + EntityScript_EndPlay — CK_IF_END_PLAY matches here because EndPlay tag is set but Teardown is not yet)
//                                   → FGroup_Teardown (DestructionPhase_Teardown — adds the Teardown tag, which ends the EndPlay window)
//
//   FGroup_Overlap (TG_PostPhysics)
//
// Destruction pipeline at the tick level:
//   Tick N where Destroy() is called:
//     (during tick) entity gets FTag_DestroyEntity_Initiate
//     FGroup_EntityLifecycle: DestructionPhase_Endplay adds FTag_DestroyEntity_EndPlay
//     FGroup_EndPlay: every feature *_EndPlay processor sees (EndPlay + !Teardown) and fires. EntityScript_EndPlay fires here too.
//     FGroup_Teardown: DestructionPhase_Teardown adds FTag_DestroyEntity_Teardown
//   Tick N+1:
//     FGroup_DestructionPipeline: DestructionPhase_Await adds FTag_DestroyEntity_Await
//   Tick N+2:
//     FGroup_DestructionPipeline: DestructionPhase_Finalize adds FTag_DestroyEntity_Finalize; DestroyEntity destroys the entity
//
// NOTE: Attribute pipeline ordering (Recompute → Compute → Clamp → FireSignals → Refill)
// is managed internally by the composite processor classes' Tick() methods.
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_DestructionPipeline;
    struct FGroup_Gameplay_TimeDelta;
    struct FGroup_Gameplay;
    struct FGroup_Gameplay_AI;
    struct FGroup_Gameplay_Audio;
    struct FGroup_Gameplay_Rendering;
    struct FGroup_Gameplay_Script;
    struct FGroup_Gameplay_Chaos;
    struct FGroup_Physics;
    struct FGroup_Transform_SyncFrom;
    struct FGroup_Transform;
    struct FGroup_Transform_Finalize;
    struct FGroup_Gameplay_Camera;
    struct FGroup_PostTransform;
    struct FGroup_Hydration;
    struct FGroup_Replication;
    struct FGroup_EntityLifecycle;
    struct FGroup_EndPlay;
    struct FGroup_Teardown;
    struct FGroup_Overlap;

    // ----------------------------------------------------------------------------------------------------------------

    struct FGroup_DestructionPipeline
    {
    };

    struct FGroup_Gameplay_TimeDelta
    {
        using RunAfter = TDepList<FGroup_DestructionPipeline>;
    };

    struct FGroup_Gameplay
    {
        using RunAfter = TDepList<FGroup_Gameplay_TimeDelta>;
    };

    struct FGroup_Gameplay_AI
    {
        using RunAfter = TDepList<FGroup_Gameplay>;
    };

    struct FGroup_Gameplay_Audio
    {
        using RunAfter = TDepList<FGroup_Gameplay_AI>;
    };

    struct FGroup_Gameplay_Rendering
    {
        using RunAfter = TDepList<FGroup_Gameplay_Audio>;
    };

    struct FGroup_Gameplay_Script
    {
        using RunAfter = TDepList<FGroup_Gameplay_Rendering>;
    };

    struct FGroup_Gameplay_Chaos
    {
        using RunAfter = TDepList<FGroup_Gameplay_Script>;
    };

    struct FGroup_Physics
    {
        using RunAfter = TDepList<FGroup_Gameplay_Chaos>;
    };

    struct FGroup_Transform_SyncFrom
    {
        using RunAfter = TDepList<FGroup_Physics>;
    };

    struct FGroup_Transform
    {
        using RunAfter = TDepList<FGroup_Transform_SyncFrom>;
    };

    struct FGroup_Transform_Finalize
    {
        using RunAfter = TDepList<FGroup_Transform>;
    };

    // Camera reads its anchor/look-at FCk_Handle_Transform AFTER the Transform groups have
    // synced this frame's pose (SyncFromActor/SyncFromMeshSocket → Transform → SyncToActor). Running
    // here (still TG_PrePhysics) guarantees the POV samples the current-frame anchor, not last frame's.
    struct FGroup_Gameplay_Camera
    {
        using RunAfter = TDepList<FGroup_Transform_Finalize>;
    };

    struct FGroup_PostTransform
    {
        using RunAfter = TDepList<FGroup_Gameplay_Camera>;
    };

    // Late hydration dispatch (spec §4.4): the replicated-fragment + local save-load dispatchers apply their
    // payloads here, AFTER FGroup_PostTransform (so every feature composed this frame exists) and BEFORE
    // FGroup_Replication (so applied values are visible before OnReplicationComplete broadcasts the same frame).
    struct FGroup_Hydration
    {
        using RunAfter = TDepList<FGroup_PostTransform>;
    };

    struct FGroup_Replication
    {
        using RunAfter = TDepList<FGroup_Hydration>;
    };

    // FGroup_EntityLifecycle hosts the tag-add processors that START the EndPlay window:
    //   - EntityJustCreated, DestroyEntity
    //   - DestructionPhase_Endplay (adds FTag_DestroyEntity_EndPlay)
    // The corresponding tag-CLOSE processor (DestructionPhase_Teardown) lives in FGroup_Teardown so that
    // FGroup_EndPlay can run between them while the (EndPlay && !Teardown) filter actually matches.
    struct FGroup_EntityLifecycle
    {
        using RunAfter = TDepList<FGroup_Replication>;
    };

    // FGroup_EndPlay is the window during which CK_IF_END_PLAY (= EndPlay + !Teardown) matches. Every
    // feature-level *_EndPlay processor and EntityScript_EndPlay live here. They fire in the same tick as
    // Destroy() is called, after DestructionPhase_Endplay has added the tag and before
    // DestructionPhase_Teardown closes the window.
    struct FGroup_EndPlay
    {
        using RunAfter = TDepList<FGroup_EntityLifecycle>;
    };

    // FGroup_Teardown closes the EndPlay window by adding FTag_DestroyEntity_Teardown (via
    // DestructionPhase_Teardown). It must run strictly after every processor in FGroup_EndPlay —
    // enforced by this group-level RunAfter so individual *_EndPlay processors don't need per-processor
    // ordering declarations.
    struct FGroup_Teardown
    {
        using RunAfter = TDepList<FGroup_EndPlay>;
    };

    struct FGroup_Overlap
    {
        static constexpr auto TickGroup = TG_PostPhysics;
    };
}

// --------------------------------------------------------------------------------------------------------------------
