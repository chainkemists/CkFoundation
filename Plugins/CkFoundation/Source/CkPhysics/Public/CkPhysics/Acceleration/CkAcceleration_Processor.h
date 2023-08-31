#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FCk_Processor_Acceleration_Setup : public TProcessor<
            FCk_Processor_Acceleration_Setup,
            FCk_Fragment_Acceleration_Params,
            FCk_Fragment_Acceleration_Current,
            FCk_Tag_Acceleration_Setup>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_Acceleration_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Acceleration_Params& InParams,
            FCk_Fragment_Acceleration_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_AccelerationModifier_SingleTarget_Setup : public TProcessor<
            FCk_Processor_AccelerationModifier_SingleTarget_Setup,
            FCk_Fragment_Acceleration_Current,
            FCk_Fragment_Acceleration_Target,
            FCk_Tag_AccelerationModifier_SingleTarget,
            FCk_Tag_AccelerationModifier_SingleTarget_Setup>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_AccelerationModifier_SingleTarget_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Acceleration_Current& InAcceleration,
            const FCk_Fragment_Acceleration_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_AccelerationModifier_SingleTarget_Teardown : public TProcessor<
            FCk_Processor_AccelerationModifier_SingleTarget_Teardown,
            FCk_Fragment_Acceleration_Current,
            FCk_Fragment_Acceleration_Target,
            FCk_Tag_AccelerationModifier_SingleTarget,
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
            const FCk_Fragment_Acceleration_Current& InAcceleration,
            const FCk_Fragment_Acceleration_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_Acceleration_Replicate
        : public TProcessor<FCk_Processor_Acceleration_Replicate, FCk_Fragment_Acceleration_Current, TObjectPtr<UCk_Fragment_Acceleration_Rep>>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Acceleration_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Acceleration_Rep>& InAccelRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
