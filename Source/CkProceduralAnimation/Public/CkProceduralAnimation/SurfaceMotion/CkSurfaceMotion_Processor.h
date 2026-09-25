#pragma once

#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROCEDURALANIMATION_API FProcessor_SurfaceMotion_Setup : public ck_exp::TProcessor<
        FProcessor_SurfaceMotion_Setup,
        FCk_Handle_SurfaceMotion,
        TReadWrite<FFragment_SurfaceMotion>,
        TReadWrite<FFragment_SurfaceMotion_Support>,
        TReadOnly<FFragment_Transform>,
        FTag_SurfaceMotion_NeedsSetup,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FTag_SurfaceMotion_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SurfaceMotion& InMotionComp,
            FFragment_SurfaceMotion_Support& InSupportComp,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProcessor_SurfaceMotion_HandleRequests : public ck_exp::TProcessor<
        FProcessor_SurfaceMotion_HandleRequests,
        FCk_Handle_SurfaceMotion,
        TReadOnly<FFragment_SurfaceMotion_Params>,
        TReadWrite<FFragment_SurfaceMotion>,
        TReadWrite<FFragment_SurfaceMotion_Requests>,
        TExclude<FTag_SurfaceMotion_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_SurfaceMotion_Setup>;
        using MarkedDirtyBy = FFragment_SurfaceMotion_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SurfaceMotion_Params& InParams,
            FFragment_SurfaceMotion& InMotionComp,
            FFragment_SurfaceMotion_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The movement request lands in the ordinary transform pass; gait reads that settled pose
    // in Transform_Derived. This is the sole writer of the owning body's transform.
    class CKPROCEDURALANIMATION_API FProcessor_SurfaceMotion_Update : public ck_exp::TProcessor<
        FProcessor_SurfaceMotion_Update,
        FCk_Handle_SurfaceMotion,
        TReadOnly<FFragment_SurfaceMotion_Params>,
        TReadOnly<FFragment_SurfaceMotion>,
        TReadWrite<FFragment_SurfaceMotion_Support>,
        TReadOnly<FFragment_Transform>,
        TExclude<FTag_SurfaceMotion_NeedsSetup>,
        TExclude<FFragment_SurfaceMotion_Failure>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_SurfaceMotion_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SurfaceMotion_Params& InParams,
            const FFragment_SurfaceMotion& InMotionComp,
            FFragment_SurfaceMotion_Support& InSupportComp,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProcessor_SurfaceMotion_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_SurfaceMotion_CancelPendingRequests,
        FCk_Handle_SurfaceMotion,
        TReadOnly<FFragment_SurfaceMotion_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SurfaceMotion_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
