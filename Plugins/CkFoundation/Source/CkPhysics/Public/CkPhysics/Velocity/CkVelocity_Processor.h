#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FCk_Processor_Velocity_Setup : public TProcessor<
            FCk_Processor_Velocity_Setup,
            FCk_Fragment_Velocity_Params,
            FCk_Fragment_Velocity_Current,
            FCk_Tag_Velocity_Setup>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_Velocity_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Params& InParams,
            FCk_Fragment_Velocity_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_VelocityModifier_SingleTarget_Setup : public TProcessor<
            FCk_Processor_VelocityModifier_SingleTarget_Setup,
            FCk_Fragment_Velocity_Current,
            FCk_Fragment_Velocity_Target,
            FCk_Tag_VelocityModifier_SingleTarget,
            FCk_Tag_VelocityModifier_SingleTarget_Setup>
    {
    public:
        using MarkedDirtyBy = FCk_Tag_VelocityModifier_SingleTarget_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Velocity_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_VelocityModifier_SingleTarget_Teardown : public TProcessor<
            FCk_Processor_VelocityModifier_SingleTarget_Teardown,
            FCk_Fragment_Velocity_Current,
            FCk_Fragment_Velocity_Target,
            FCk_Tag_VelocityModifier_SingleTarget,
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
            const FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Velocity_Target& InTarget) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_Velocity_Replicate
        : public TProcessor<FCk_Processor_Velocity_Replicate, FCk_Fragment_Velocity_Current, TObjectPtr<UCk_Fragment_Velocity_Rep>>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Velocity_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
