#pragma once
#include "CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Registry/CkRegistry.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FCk_Processor_EntityLifetime_EntityJustCreated
    {
    public:
        using FTimeType = FCk_Time;
        using FRegistryType = FCk_Registry;

    public:
        explicit FCk_Processor_EntityLifetime_EntityJustCreated(const FRegistryType& InRegistry);

    public:
        auto Tick(FTimeType) -> void;

    private:
        FCk_Registry _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FCk_Processor_EntityLifetime_TriggerDestroyEntity
        : public TProcessor<FCk_Processor_EntityLifetime_TriggerDestroyEntity, FCk_Tag_TriggerDestroyEntity>
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

    class CKECS_API FCk_Processor_EntityLifetime_PendingDestroyEntity
        : public TProcessor<FCk_Processor_EntityLifetime_PendingDestroyEntity, FCk_Tag_PendingDestroyEntity>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}
