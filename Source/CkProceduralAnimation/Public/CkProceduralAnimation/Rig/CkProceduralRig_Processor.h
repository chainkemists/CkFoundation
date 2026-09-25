#pragma once

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Processor.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Processor.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralRig_Setup : public ck_exp::TProcessor<
        FProcessor_ProceduralRig_Setup,
        FCk_Handle_ProceduralRig,
        TReadOnly<FFragment_ProceduralRig_Params>,
        TReadWrite<FFragment_ProceduralRig>,
        FTag_ProceduralRig_NeedsSetup,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralGait_Update>;
        using MarkedDirtyBy = FTag_ProceduralRig_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralRig_Params& InParams,
            FFragment_ProceduralRig& InRigComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Parallel-ready: reads its own leg's fragments and the body transform, writes only through deferred transform
    // requests. Kept single-threaded until a benchmark justifies TParallelProcessor.
    class CKPROCEDURALANIMATION_API FProcessor_ProceduralRig_Update : public ck_exp::TProcessor<
        FProcessor_ProceduralRig_Update,
        FCk_Handle_ProceduralRig,
        TReadOnly<FFragment_ProceduralRig_Params>,
        TReadWrite<FFragment_ProceduralRig>,
        TReadOnly<FFragment_ProceduralLeg_Params>,
        TReadOnly<FFragment_ProceduralLeg>,
        TExclude<FTag_ProceduralRig_NeedsSetup>,
        TExclude<FFragment_ProceduralRig_Failure>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_ProceduralGait_Update, FProcessor_ProceduralRig_Setup, FProcessor_ProceduralBodyPose_Update>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralRig_Params& InParams,
            FFragment_ProceduralRig& InRigComp,
            const FFragment_ProceduralLeg_Params& InLegParams,
            const FFragment_ProceduralLeg& InLegComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
