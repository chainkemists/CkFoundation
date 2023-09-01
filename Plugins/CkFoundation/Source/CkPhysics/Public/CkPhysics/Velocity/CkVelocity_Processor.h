#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_Velocity_Setup : public TProcessor<
            FProcessor_Velocity_Setup,
            FFragment_Velocity_Params,
            FFragment_Velocity_Current,
            FTag_Velocity_Setup>
    {
    public:
        using MarkedDirtyBy = FTag_Velocity_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Params& InParams,
            FFragment_Velocity_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_VelocityModifier_SingleTarget_Setup : public TProcessor<
            FProcessor_VelocityModifier_SingleTarget_Setup,
            FFragment_Velocity_Current,
            FCk_Fragment_Velocity_Target,
            FTag_VelocityModifier_SingleTarget,
            FTag_VelocityModifier_SingleTarget_Setup>
    {
    public:
        using MarkedDirtyBy = FTag_VelocityModifier_SingleTarget_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Velocity_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_VelocityModifier_SingleTarget_Teardown : public TProcessor<
            FProcessor_VelocityModifier_SingleTarget_Teardown,
            FFragment_Velocity_Current,
            FCk_Fragment_Velocity_Target,
            FTag_VelocityModifier_SingleTarget,
            FCk_Tag_PendingDestroyEntity>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_PendingDestroyEntity;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Velocity_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_Velocity_Replicate
        : public TProcessor<FProcessor_Velocity_Replicate, FFragment_Velocity_Current, TObjectPtr<UCk_Fragment_Velocity_Rep>>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Velocity_Rep>& InVelRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
