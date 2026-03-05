#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle_ReadOnly.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Processor/CkDeferredCommands.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkProfile/Stats/CkStats.h"

#include "Async/ParallelFor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    class TParallelProcessor : public TProcessorBase<T_DerivedProcessor>
    {
        CK_GENERATED_BODY(TParallelProcessor<T_DerivedProcessor COMMA T_HandleType COMMA T_Fragments...>);

        static_assert(
            ((detail::TIsExcludedPolicy<T_Fragments>::value ||
              detail::TIsEmptyPolicy<T_Fragments>::value ||
              detail::TIsAccessPolicyWrapped<T_Fragments>::value) && ...),
            "All non-excluded, non-empty fragments in TParallelProcessor must be wrapped "
            "in ck::TReadOnly<T> or ck::TReadWrite<T> for explicit access intent.");

    public:
        // STAT_ForEachEntity runs on worker threads — visible in Unreal Insights
        // but not in the flat stat overlay. Use STAT_ParallelDispatch for
        // game-thread wall-clock time of the parallel phase.
        CK_DEFINE_PHASE_STAT(STAT_ForEachEntity, T_DerivedProcessor, ck::FStatPhase_ForEachEntity, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);
        CK_DEFINE_PHASE_STAT(STAT_CollectEntities, T_DerivedProcessor, ck::FStatPhase_CollectEntities, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_PHASE_STAT(STAT_ParallelDispatch, T_DerivedProcessor, ck::FStatPhase_ParallelDispatch, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_PHASE_STAT(STAT_FlushCommands, T_DerivedProcessor, ck::FStatPhase_FlushCommands, FStatGroup_STATGROUP_CkProcessors_Details);

    public:
        using Super = TProcessorBase<T_DerivedProcessor>;
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = TCk_Handle_ReadOnly<T_HandleType>;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;

    public:
        explicit TParallelProcessor(
            const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_PoliciesOnly, typename... T_ComponentsOnly>
        auto DoTick(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>) -> void;

    private:
        // Reused across ticks to avoid per-tick allocation. Reset() preserves capacity.
        TArray<EntityType> _CachedEntities;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    TParallelProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        TParallelProcessor(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    auto
        TParallelProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        using ViewType = decltype(this->_TransientEntity.template View<detail::UnwrapAccessPolicy_T<T_Fragments>...>());
        using ComponentsOnly = typename ViewType::template FragmentsOnly<detail::UnwrapAccessPolicy_T<T_Fragments>...>;
        using PoliciesOnly = detail::PoliciesOnly<T_Fragments...>;

        DoTick(InDeltaT, PoliciesOnly{}, ComponentsOnly{});
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    template <typename... T_PoliciesOnly, typename... T_ComponentsOnly>
    auto
        TParallelProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        // Phase 1: Collect matching entities (single-threaded)
        // _CachedEntities is reused across ticks — Reset() preserves allocated capacity.
        _CachedEntities.Reset();
        {
            CK_STAT(STAT_CollectEntities);

            this->_TransientEntity.template View<detail::UnwrapAccessPolicy_T<T_Fragments>...>().ForEach(
                [&](EntityType InEntity, T_ComponentsOnly&... /*unused*/)
            {
                _CachedEntities.Emplace(InEntity);
            });
        }

        if (_CachedEntities.IsEmpty())
        { return; }

        // Phase 2: Parallel iteration with per-task command buffers
        auto& Registry = this->_TransientEntity.Get_Registry();
        auto TaskCommandBuffers = TArray<FDeferredCommandBuffer>{};

        {
            CK_STAT(STAT_ParallelDispatch);

#if !UE_BUILD_SHIPPING
            Registry.BeginParallelRegion();
#endif

            const auto ParallelBody = [&](FDeferredCommandBuffer& InTaskCommands, int32 Index)
            {
                CK_STAT(STAT_ForEachEntity);

                const auto Entity = _CachedEntities[Index];
                auto ReadOnlyHandle = HandleType{Entity, Registry, InTaskCommands};

                This()->ForEachEntity(InDeltaT, ReadOnlyHandle,
                    static_cast<typename detail::TResolveConstness<T_PoliciesOnly, T_ComponentsOnly>::Type>(
                        Registry.template Get<T_ComponentsOnly>(Entity))...);
            };

            if constexpr (detail::THasMinBatchSize<DerivedType>::value)
            {
                ParallelForWithTaskContext(
                    TEXT("TParallelProcessor"),
                    TaskCommandBuffers,
                    _CachedEntities.Num(),
                    DerivedType::MinBatchSize,
                    ParallelBody,
                    EParallelForFlags::None);
            }
            else
            {
                ParallelForWithTaskContext(
                    TaskCommandBuffers,
                    _CachedEntities.Num(),
                    ParallelBody);
            }

#if !UE_BUILD_SHIPPING
            Registry.EndParallelRegion();
#endif
        }

        // Phase 3: Flush all per-task command buffers (single-threaded)
        {
            CK_STAT(STAT_FlushCommands);

            for (auto& TaskBuffer : TaskCommandBuffers)
            {
                TaskBuffer.Flush(this->_TransientEntity);
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
