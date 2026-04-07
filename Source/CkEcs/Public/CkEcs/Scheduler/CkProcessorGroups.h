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
//       → FGroup_Gameplay  (most processors live here; MarkedDirtyBy = automatic pump)
//         → FGroup_Physics
//           → FGroup_Transform
//             → FGroup_PostTransform (Misc, ChaosDestruction, etc.)
//               → FGroup_Replication
//                 → FGroup_EntityLifecycle (entity create + final destroy)
//
//   FGroup_Overlap (TG_PostPhysics)
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FGroup_PreDestruction;
    struct FGroup_DestructionPipeline;
    struct FGroup_Gameplay;
    struct FGroup_Physics;
    struct FGroup_Transform;
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

    struct FGroup_Gameplay
    {
        using RunAfter = TDepList<FGroup_DestructionPipeline>;
    };

    struct FGroup_Physics
    {
        using RunAfter = TDepList<FGroup_Gameplay>;
    };

    struct FGroup_Transform
    {
        using RunAfter = TDepList<FGroup_Physics>;
    };

    struct FGroup_PostTransform
    {
        using RunAfter = TDepList<FGroup_Transform>;
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
