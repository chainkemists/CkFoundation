#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/CkNet_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"
#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"
#include "CkPhysics/Integrator/CkIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_Integrator_Setup : public TProcessor<
            FCk_Processor_Integrator_Setup,
            TExclude<FCk_Tag_HasAuthority>,
            FCk_Tag_Integrator_Setup,
            FCk_Fragment_Integrator_Current,
            FCk_Fragment_Velocity_Current,
            FCk_Fragment_Acceleration_Current>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Integrator_Current& InIntegrator,
            FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Acceleration_Current& InAcceleration) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FCk_Processor_Integrator_Update : public TProcessor<
            FCk_Processor_Integrator_Update,
            TExclude<FCk_Tag_Integrator_Setup>,
            FCk_Tag_Integrator_Update,
            FCk_Fragment_Integrator_Current,
            FCk_Fragment_Velocity_Current,
            FCk_Fragment_Acceleration_Current>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Integrator_Current& InIntegrator,
            FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Acceleration_Current& InAcceleration) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
