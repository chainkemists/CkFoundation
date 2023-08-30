#pragma once

#include "CkActorModifier_Fragment.h"
#include "CkActorModifier_Fragment_Params.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_SpawnActor_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_SpawnActor_HandleRequests, FCk_Fragment_ActorModifier_SpawnActorRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_SpawnActorRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const TimeType& InDeltaT,
            HandleType InHandle,
            FCk_Fragment_ActorModifier_SpawnActorRequests& InRequests) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_AddActorComponent_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_AddActorComponent_HandleRequests, FCk_Fragment_OwningActor_Current, FCk_Fragment_ActorModifier_AddActorComponentRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_AddActorComponentRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_OwningActor_Current& InOwningActorComp,
            FCk_Fragment_ActorModifier_AddActorComponentRequests& InRequests) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
