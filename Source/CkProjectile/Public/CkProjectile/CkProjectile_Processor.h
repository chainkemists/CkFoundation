#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcsBasics/Transform/CkTransform_Fragment.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"
#include "CkProjectile/CkProjectile_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROJECTILE_API FProcessor_Projectile_Update : public TProcessor<
            FProcessor_Projectile_Update,
            FFragment_EulerIntegrator_Current,
            FTag_EulerIntegrator_Update>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegratorComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROJECTILE_API FProcessor_Projectile_AutoReorient : public TProcessor<
            FProcessor_Projectile_AutoReorient,
            FFragment_Velocity_Current,
            FFragment_Transform_Current,
            FTag_Projectile_AutoReorient>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocityCurrent,
            const FFragment_Transform_Current&) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
