#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/CkNet_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"
#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_EulerIntegrator_DoOnePredictiveUpdate : public TProcessor<
            FProcessor_EulerIntegrator_DoOnePredictiveUpdate,
            TExclude<FTag_HasAuthority>,
            FTag_EulerIntegrator_DoOnePredictiveUpdate,
            FFragment_EulerIntegrator_Current,
            FFragment_Velocity_Current,
            FFragment_Acceleration_Current>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_Velocity_Current& InVelocity,
            const FFragment_Acceleration_Current& InAcceleration) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_EulerIntegrator_Update : public TProcessor<
            FProcessor_EulerIntegrator_Update,
            TExclude<FTag_EulerIntegrator_DoOnePredictiveUpdate>,
            FTag_EulerIntegrator_Update,
            FFragment_EulerIntegrator_Current,
            FFragment_Velocity_Current,
            FFragment_Acceleration_Current>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_Velocity_Current& InVelocity,
            const FFragment_Acceleration_Current& InAcceleration) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
