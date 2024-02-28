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

    class CKECS_API FProcessor_EntityLifetime_DestructionPhase_Await
        : public TProcessor<FProcessor_EntityLifetime_DestructionPhase_Await, FTag_DestroyEntity_Initiate>
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

    class CKECS_API FProcessor_EntityLifetime_DestructionPhase_Finalize
        : public TProcessor<FProcessor_EntityLifetime_DestructionPhase_Finalize, FTag_DestroyEntity_Await>
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

    class CKECS_API FProcessor_EntityLifetime_DestroyEntity
        : public TProcessor<FProcessor_EntityLifetime_DestroyEntity, FTag_DestroyEntity_Finalize>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(TimeType InDeltaT, HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}
