#pragma once

#include "CkActorModifier_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKACTOR_API FProcessor_ActorModifier_SpawnActor_HandleRequests : public TProcessor<
            FProcessor_ActorModifier_SpawnActor_HandleRequests,
            FFragment_ActorModifier_SpawnActorRequests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ActorModifier_SpawnActorRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_ActorModifier_SpawnActorRequests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ActorModifier_SpawnActor& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FProcessor_ActorModifier_AddActorComponent_HandleRequests : public TProcessor<
            FProcessor_ActorModifier_AddActorComponent_HandleRequests,
            FFragment_ActorModifier_AddActorComponentRequests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ActorModifier_AddActorComponentRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_ActorModifier_AddActorComponentRequests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ActorModifier_AddActorComponent& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
