#pragma once
#include "CkEntityLifetime_Fragment.h"

#include "CkEcs/OwningActor/CkOwningActor_Processors.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_EntityJustCreated : public TProcessorBase<FProcessor_EntityLifetime_EntityJustCreated>
    {
    public:
        using Group = FGroup_EntityLifecycle;

    public:
        using FTimeType = FCk_Time;
        using FRegistryType = FCk_Registry;

    public:
        using Super = TProcessorBase;

    public:
        using TProcessorBase::TProcessorBase;

    public:
        auto DoTick(FTimeType) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_DestructionPhase_Endplay
        : public TProcessor<FProcessor_EntityLifetime_DestructionPhase_Endplay,
            FTag_DestroyEntity_Initiate,
            ck::TExclude<FTag_DestroyEntity_EndPlay>>
    {
    public:
        using Group = FGroup_EntityLifecycle;
        using RunAfter = TDepList<FProcessor_EntityLifetime_DestroyEntity>;

    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_DestructionPhase_Teardown
        : public TProcessor<FProcessor_EntityLifetime_DestructionPhase_Teardown,
            FTag_DestroyEntity_EndPlay,
            TExclude<FTag_DestroyEntity_Teardown>>
    {
    public:
        using Group = FGroup_DestructionPipeline;
        using RunAfter = TDepList<FProcessor_OwningActor_Destroy>;

    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_DestructionPhase_Await
        : public TProcessor<FProcessor_EntityLifetime_DestructionPhase_Await,
            FTag_DestroyEntity_Teardown,
            TExclude<FTag_DestroyEntity_Await>>
    {
    public:
        using Group = FGroup_DestructionPipeline;
        using RunAfter = TDepList<FProcessor_EntityLifetime_DestructionPhase_Teardown>;

    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_DestructionPhase_Finalize
        : public TProcessor<FProcessor_EntityLifetime_DestructionPhase_Finalize,
            FTag_DestroyEntity_Await,
            TExclude<FTag_DestroyEntity_Finalize>>
    {
    public:
        using Group = FGroup_DestructionPipeline;
        using RunAfter = TDepList<FProcessor_EntityLifetime_DestructionPhase_Await>;

    public:
        using Super = TProcessor;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor_EntityLifetime_DestroyEntity
        : public TProcessor<FProcessor_EntityLifetime_DestroyEntity, FTag_DestroyEntity_Finalize>
    {
    public:
        using Group = FGroup_EntityLifecycle;
        using RunAfter = TDepList<FProcessor_EntityLifetime_EntityJustCreated>;

    public:
        using Super = TProcessor;
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;

    private:
        TArray<EntityType> _EntitiesToDestroy;
    };

    // --------------------------------------------------------------------------------------------------------------------
}