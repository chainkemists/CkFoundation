#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FCk_Processor_OwningActor_Destroy
        : public TProcessor<FCk_Processor_OwningActor_Destroy, FCk_Fragment_OwningActor_Current, FCk_Tag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_OwningActor_Current& InOwningActorComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
