#pragma once

#include "CkPredictedVelocity_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// Forward declaration for RunAfter
namespace ck { class FProcessor_EulerIntegrator_Update; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPHYSICS_API FProcessor_PredictedVelocity_Update : public TProcessor<
            FProcessor_PredictedVelocity_Update,
            ck::TReadWrite<FFragment_PredictedVelocity_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_EulerIntegrator_Update>;

    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PredictedVelocity_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------