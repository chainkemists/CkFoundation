#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_Acceleration_Setup : public TProcessor<
            FProcessor_Acceleration_Setup,
            FFragment_Acceleration_Params,
            FFragment_Acceleration_Current,
            FTag_Acceleration_Setup>
    {
    public:
        using MarkedDirtyBy = FTag_Acceleration_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Params& InParams,
            FFragment_Acceleration_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_AccelerationModifier_SingleTarget_Setup : public TProcessor<
            FProcessor_AccelerationModifier_SingleTarget_Setup,
            FFragment_Acceleration_Current,
            FFragment_Acceleration_Target,
            FTag_AccelerationModifier_SingleTarget,
            FTag_AccelerationModifier_SingleTarget_Setup>
    {
    public:
        using MarkedDirtyBy = FTag_AccelerationModifier_SingleTarget_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_AccelerationModifier_SingleTarget_Teardown : public TProcessor<
            FProcessor_AccelerationModifier_SingleTarget_Teardown,
            FFragment_Acceleration_Current,
            FFragment_Acceleration_Target,
            FTag_AccelerationModifier_SingleTarget,
            FTag_PendingDestroyEntity>
    {
    public:
        using MarkedDirtyBy = FTag_PendingDestroyEntity;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_Acceleration_Replicate
        : public TProcessor<FProcessor_Acceleration_Replicate, FFragment_Acceleration_Current, TObjectPtr<UCk_Fragment_Acceleration_Rep>>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Acceleration_Rep>& InAccelRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
