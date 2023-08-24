#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkPhysics/Integrator/CkIntegrator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROJECTILE_API FCk_Processor_Projectile_Update : public TProcessor<
            FCk_Processor_Projectile_Update,
            FCk_Fragment_Integrator_Current,
            FCk_Tag_Integrator_Update>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Integrator_Current& InIntegratorComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
