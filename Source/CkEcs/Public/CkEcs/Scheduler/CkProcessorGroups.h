#pragma once

#include "CkProcessorTraits.inl.h"

// --------------------------------------------------------------------------------------------------------------------
// Group processors define the top-level ordering of the ECS pipeline.
// Individual processors declare `using Group = FGroup_X;` to join a group.
//
// Pipeline order (all TG_PrePhysics unless noted):
//
//   FGroup_PreDestruction
//     → FGroup_DestructionPipeline
//       → FGroup_Gameplay_TimeDelta  (Timer, Tween, Substep)
//         → FGroup_Gameplay  (core gameplay: Interaction, Inventory, Objectives, etc.)
//           → FGroup_Gameplay_AI  (StateMachine, StateTree)
//             → FGroup_Gameplay_Audio  (AudioDirector, AudioTrack, Sfx, Vfx, VfxCue)
//               → FGroup_Gameplay_Rendering  (ISM, PMG, Shapes, RenderStatus, Camera)
//                 → FGroup_Gameplay_Script  (EntityScript construction + BeginPlay)
//                   → FGroup_Gameplay_Chaos  (GeometryCollection destruction)
//                     → FGroup_Physics
//                       → FGroup_Transform_SyncFrom  (SyncFromActor, SyncFromMeshSocket, Interpolation)
//                         → FGroup_Transform  (HandleRequests, SceneNode, Tween)
//                           → FGroup_Transform_Finalize  (SyncToActor, FireSignals)
//                             → FGroup_PostTransform (OverlapBody, RaySense, UI, etc.)
//                               → FGroup_Replication
//                                 → FGroup_EntityLifecycle (entity create + final destroy)
//
//   FGroup_Overlap (TG_PostPhysics)
//
// NOTE: Attribute pipeline ordering (Recompute → Compute → Clamp → FireSignals → Refill)
// is managed internally by the composite processor classes' Tick() methods.
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_PreDestruction;
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
    struct FGroup_PostTransform;
    struct FGroup_Replication;
    struct FGroup_EntityLifecycle;
    struct FGroup_Overlap;

    // ----------------------------------------------------------------------------------------------------------------

    struct FGroup_PreDestruction
    {
    };

    struct FGroup_DestructionPipeline
    {
        using RunAfter = TDepList<FGroup_PreDestruction>;
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

    struct FGroup_PostTransform
    {
        using RunAfter = TDepList<FGroup_Transform_Finalize>;
    };

    struct FGroup_Replication
    {
        using RunAfter = TDepList<FGroup_PostTransform>;
    };

    struct FGroup_EntityLifecycle
    {
        using RunAfter = TDepList<FGroup_Replication>;
    };

    struct FGroup_Overlap
    {
        static constexpr auto TickGroup = TG_PostPhysics;
    };
}

// --------------------------------------------------------------------------------------------------------------------
