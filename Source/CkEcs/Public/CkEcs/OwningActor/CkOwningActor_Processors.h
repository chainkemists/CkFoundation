#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // NOTE: CK_IF_TEARING_DOWN on purpose since we only destroy the actor AFTER EndPlay finishes
    class CKECS_API FProcessor_OwningActor_Destroy : public TProcessor<
        FProcessor_OwningActor_Destroy,
        ck::TReadOnly<FFragment_OwningActor_Current>,
        CK_IF_TEARING_DOWN>
    {
    public:
        using Group = FGroup_DestructionPipeline;

    public:
        using TProcessor::TProcessor;

    public:
        static auto 
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActorComp) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
