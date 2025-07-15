#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcs/Transform/CkTransform_Fragment.h"

#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_AutoReorient_OrientTowardsVelocity : public ck_exp::TProcessor<
            FProcessor_AutoReorient_OrientTowardsVelocity,
            FCk_Handle_Transform,
            FFragment_Velocity_Current,
            FFragment_Transform,
            FFragment_AutoReorient_Params,
            FTag_AutoReorient_OrientTowardsVelocity,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AutoReorient_OrientTowardsVelocity;

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
