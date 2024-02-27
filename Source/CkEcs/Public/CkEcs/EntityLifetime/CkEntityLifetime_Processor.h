#pragma once
#include "CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Registry/CkRegistry.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_EntityJustCreated : public FProcessor
    {
    public:
        using Super = FProcessor;
        using FTimeType = FCk_Time;
        using FRegistryType = FCk_Registry;

    public:
        explicit FProcessor_EntityLifetime_EntityJustCreated(const FRegistryType& InRegistry);

    public:
        auto Tick(FTimeType) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_RequestDestroyEntity
        : public TProcessor<FProcessor_EntityLifetime_RequestDestroyEntity, FTag_RequestTriggerDestroyEntity>
    {
    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_TriggerDestroyEntity
        : public TProcessor<FProcessor_EntityLifetime_TriggerDestroyEntity, FTag_TriggerDestroyEntity>
    {
    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_PendingDestroyEntity
        : public TProcessor<FProcessor_EntityLifetime_PendingDestroyEntity, FTag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}
