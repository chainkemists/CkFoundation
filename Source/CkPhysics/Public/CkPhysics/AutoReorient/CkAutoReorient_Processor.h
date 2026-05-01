#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_AutoReorient_OrientTowardsVelocity : public ck_exp::TProcessor<
            FProcessor_AutoReorient_OrientTowardsVelocity,
            FCk_Handle_Transform,
            ck::TReadOnly<FFragment_Velocity_Current>,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_AutoReorient_Params>,
            FTag_AutoReorient_OrientTowardsVelocity,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_AutoReorient_OrientTowardsVelocity;
        // Enqueues Request_AddRotationOffset(RotationOffset) computed from cached velocity — pump would re-enqueue and over-rotate.
        // The orient-towards-velocity tag is sticky (not removed by this body).
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocityCurrent,
            const FFragment_Transform& InTransformCurrent,
            const FFragment_AutoReorient_Params& InAutoReorientParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
