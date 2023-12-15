#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROJECTILE_API FProcessor_Projectile_Update : public TProcessor<
            FProcessor_Projectile_Update,
            FFragment_EulerIntegrator_Current,
            FTag_EulerIntegrator_NeedsUpdate>
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
}

// --------------------------------------------------------------------------------------------------------------------
