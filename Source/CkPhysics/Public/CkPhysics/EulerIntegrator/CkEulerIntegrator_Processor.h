#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

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
            FFragment_Acceleration_Current,
            TObjectPtr<UCk_Fragment_Velocity_Rep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_Velocity_Current& InVelocity,
            const FFragment_Acceleration_Current& InAcceleration,
            const TObjectPtr<UCk_Fragment_Velocity_Rep>& InVelocityRO) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPHYSICS_API FProcessor_EulerIntegrator_Update : public TProcessor<
            FProcessor_EulerIntegrator_Update,
            TExclude<FTag_EulerIntegrator_DoOnePredictiveUpdate>,
            FTag_EulerIntegrator_NeedsUpdate,
            FFragment_EulerIntegrator_Current,
            FFragment_Velocity_Current,
            FFragment_Acceleration_Current,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_Velocity_Current& InVelocity,
            const FFragment_Acceleration_Current& InAcceleration) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
