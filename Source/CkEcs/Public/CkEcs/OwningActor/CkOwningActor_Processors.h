#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FProcessor_OwningActor_Destroy : public TProcessor<
        FProcessor_OwningActor_Destroy,
        FFragment_OwningActor_Current,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_DestructionPipeline;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActorComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
