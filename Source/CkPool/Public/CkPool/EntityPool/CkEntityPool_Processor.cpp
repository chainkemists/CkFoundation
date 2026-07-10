#include "CkEntityPool_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkPool/CkPool_Log.h"
#include "CkPool/EntityPool/CkEntityPool_Utils.h"
#include "CkPool/Poolable/CkPoolableReceiver_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityPool_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityPool_Prewarm);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityPool_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityPool_HandleDestroyedPooledEntity);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityPool_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_pool_processor
{
    // Pooled entities are composed via EntityScript::Add — the script instance UObject is where
    // FCk_Pool_PoolableReceiver properties live (the AS-reachable poolable opt-in)
    auto
        TryGet_ScriptInstance(
            FCk_Handle InEntity)
        -> UCk_EntityScript_UE*
    {
        if (NOT InEntity.Has<ck::FFragment_EntityScript_Current>())
        { return {}; }

        return InEntity.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityPool_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent)
        -> void
    {
        InPool.Remove<MarkedDirtyBy>();

        CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_EntityScriptClass()),
            TEXT("EntityPool [{}] has an INVALID EntityScript class — the Pool will never vend anything"), InPool)
        { return; }

        auto PrewarmCount = InParams.Get_PrewarmCount();

        if (InParams.Get_CapacityPolicy() == ECk_Pool_CapacityPolicy::Bounded)
        { PrewarmCount = FMath::Min(PrewarmCount, InParams.Get_MaxSize()); }

        InCurrent._NumPrewarmRemaining = PrewarmCount;

        if (InCurrent._NumPrewarmRemaining > 0)
        {
            InPool.AddOrGet<FTag_EntityPool_PrewarmInProgress>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityPool_Prewarm::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent)
        -> void
    {
        const auto Budget = FMath::Max(1, InParams.Get_PrewarmBudgetPerTick());
        const auto NumToSpawn = FMath::Min(Budget, InCurrent._NumPrewarmRemaining);

        for (auto Index = 0; Index < NumToSpawn; ++Index)
        {
            DoInitiate_InstanceSpawn(InPool, InParams, InCurrent);
        }

        InCurrent._NumPrewarmRemaining -= NumToSpawn;

        if (InCurrent._NumPrewarmRemaining <= 0)
        {
            InPool.Remove<FTag_EntityPool_PrewarmInProgress>();
        }
    }

    auto
        FProcessor_EntityPool_Prewarm::
        DoInitiate_InstanceSpawn(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent)
        -> void
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InPool, [&](FCk_Handle InNewEntity)
        {
            InNewEntity.Add<FFragment_EntityPooled>(FFragment_EntityPooled{InPool});
        });

        auto PoolCopy = InPool;
        const auto PostConstruction = FCk_EntityScript_PostConstruction_Func{
        [PoolCopy](FCk_Handle InConstructedEntity) mutable
        {
            UCk_Utils_EntityPool_UE::DoRequest_HandleConstructed(PoolCopy, InConstructedEntity);
        }};

        // archetype pools construct from the pinned authored instance; class pools from the CDO
        const auto Pending = ck::IsValid(InCurrent._PinnedArchetype.Get())
            ? UCk_Utils_EntityScript_UE::Add(NewEntity, TWeakObjectPtr<UCk_EntityScript_UE>{InCurrent._PinnedArchetype.Get()},
                InParams.Get_ConstructionSpawnParams(), PostConstruction)
            : UCk_Utils_EntityScript_UE::Add(NewEntity, InParams.Get_EntityScriptClass(),
                InParams.Get_ConstructionSpawnParams(), PostConstruction);

        if (ck::Is_NOT_Valid(Pending))
        {
            // EntityScript::Add already fired the ensure naming the reason — recovery is to not leak the half-built instance
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewEntity);
            return;
        }

        ++InCurrent._NumLiveInstances;
        ++InCurrent._NumSpawnsInFlight;

        pool::VeryVerbose(TEXT("EntityPool [{}] initiated spawn of a pooled instance [{}] (live: [{}], in-flight: [{}])"),
            InPool, NewEntity, InCurrent._NumLiveInstances, InCurrent._NumSpawnsInFlight);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityPool_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            FFragment_EntityPool_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InPool, InParams, InCurrent, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InPool.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_Acquire& InRequest)
        -> void
    {
        // pop dormant instances, skipping any that were externally destroyed while dormant
        // (their HandleDestroyed reconciliation may still be queued behind this request)
        while (InCurrent._DormantEntities.Num() > 0)
        {
            auto DormantEntity = InCurrent._DormantEntities.Pop(EAllowShrinking::No);

            if (ck::Is_NOT_Valid(DormantEntity))
            { continue; }

            ++InCurrent._NumHits;
            DoDeliver(InPool, InCurrent, DormantEntity, InRequest.Get_PerUseParams(), InRequest.Get_TicketEntity());
            return;
        }

        ++InCurrent._NumMisses;

        switch (const auto& ExhaustionPolicy = InParams.Get_ExhaustionPolicy())
        {
            case ECk_Pool_ExhaustionPolicy::Grow:
            {
                const auto CanCreateMore = InParams.Get_CapacityPolicy() == ECk_Pool_CapacityPolicy::Unbounded ||
                    InCurrent._NumLiveInstances < InParams.Get_MaxSize();

                if (CanCreateMore)
                {
                    FProcessor_EntityPool_Prewarm::DoInitiate_InstanceSpawn(InPool, InParams, InCurrent);

                    // batch growth: TOP UP the queued extras to (GrowBatchCount - 1), amortized through the
                    // prewarm budget — never hitching the frame. Top-up (not add-per-miss) so a burst of N
                    // simultaneous misses provisions ONE batch of extras, not N of them; clamped to capacity
                    if (const auto ExtraGrow = InParams.Get_GrowBatchCount() - 1 - InCurrent._NumPrewarmRemaining;
                        ExtraGrow > 0)
                    {
                        auto AllowedExtra = ExtraGrow;

                        if (InParams.Get_CapacityPolicy() == ECk_Pool_CapacityPolicy::Bounded)
                        {
                            AllowedExtra = FMath::Min(AllowedExtra,
                                InParams.Get_MaxSize() - InCurrent._NumLiveInstances - InCurrent._NumPrewarmRemaining);
                        }

                        if (AllowedExtra > 0)
                        {
                            InCurrent._NumPrewarmRemaining += AllowedExtra;
                            InPool.AddOrGet<FTag_EntityPool_PrewarmInProgress>();
                        }
                    }
                }
                else
                {
                    pool::Verbose(TEXT("EntityPool [{}] is at capacity [{}] — parking the acquire until the next Release"),
                        InPool, InParams.Get_MaxSize());
                    UUtils_Signal_OnEntityPool_Exhausted::Broadcast(InPool, MakePayload(InPool));
                }

                InCurrent._PendingAcquires.Emplace(
                    FEntityPool_PendingAcquireEntry{InRequest.Get_TicketEntity(), InRequest.Get_PerUseParams()});
                break;
            }
            case ECk_Pool_ExhaustionPolicy::Fail:
            {
                UUtils_Signal_OnEntityPool_Exhausted::Broadcast(InPool, MakePayload(InPool));
                DoFulfill_Failed(InPool, InRequest.Get_TicketEntity());
                break;
            }
            default:
            {
                CK_INVALID_ENUM(ExhaustionPolicy);
                break;
            }
        }
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_Release& InRequest)
        -> void
    {
        auto EntityToRelease = InRequest.Get_EntityToRelease();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityToRelease),
            TEXT("Cannot Release Entity [{}] to Pool [{}] — the Entity is destroyed or being destroyed. "
                 "Destroying an in-use pooled Entity is legal (the Pool forgets it) but Releasing it afterwards is a caller bug"),
            EntityToRelease, InPool)
        { return; }

        CK_ENSURE_IF_NOT(EntityToRelease.Has<FFragment_EntityPooled>(),
            TEXT("Cannot Release Entity [{}] to Pool [{}] — the Entity is not pooled"), EntityToRelease, InPool)
        { return; }

        const auto& Pooled = EntityToRelease.Get<FFragment_EntityPooled>();

        CK_ENSURE_IF_NOT(Pooled.Get_OwningPool() == InPool,
            TEXT("Cannot Release Entity [{}] to Pool [{}] — it belongs to Pool [{}]"),
            EntityToRelease, InPool, Pooled.Get_OwningPool())
        { return; }

        CK_ENSURE_IF_NOT(NOT EntityToRelease.Has<FTag_EntityPool_Dormant>(),
            TEXT("Double-Release: Entity [{}] is already dormant in Pool [{}]"), EntityToRelease, InPool)
        { return; }

        // per-instance veto (FCk_Pool_PoolableReceiver::_CanBePooled == false): destroy instead of parking.
        // The InUse tag stays on — HandleDestroyed's steal-path reconciliation owns the counters
        if (auto* ScriptInstance = ck_entity_pool_processor::TryGet_ScriptInstance(EntityToRelease);
            ck::IsValid(ScriptInstance) && NOT UCk_Utils_PoolableReceiver_UE::Get_CanBePooled_OnObject(ScriptInstance))
        {
            pool::Verbose(TEXT("Pooled Entity [{}] vetoed pooling (CanBePooled == false) — destroying instead"), EntityToRelease);
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(EntityToRelease);
            return;
        }

        --InCurrent._NumInUse;
        EntityToRelease.Try_Remove<FTag_EntityPool_InUse>();

        UUtils_Signal_OnEntityPool_EntityReleased::Broadcast(EntityToRelease, MakePayload(EntityToRelease));

        UCk_Utils_PoolableReceiver_UE::Broadcast_ReleasedToPool_OnObject(
            ck_entity_pool_processor::TryGet_ScriptInstance(EntityToRelease));

        // hand the instance straight to a parked acquire if one is waiting — it never enters the dormant list
        if (DoTryDeliver_ToPendingAcquire(InPool, InCurrent, EntityToRelease))
        { return; }

        EntityToRelease.AddOrGet<FTag_EntityPool_Dormant>();
        InCurrent._DormantEntities.Emplace(EntityToRelease);
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_HandleConstructed& InRequest)
        -> void
    {
        --InCurrent._NumSpawnsInFlight;

        auto ConstructedEntity = InRequest.Get_ConstructedEntity();

        if (ck::Is_NOT_Valid(ConstructedEntity))
        {
            // destroyed while (or right after) constructing — the HandleDestroyed reconciliation owns the counters
            pool::Verbose(TEXT("A pooled instance was destroyed before Pool [{}] could take custody of it"), InPool);
            return;
        }

        if (DoTryDeliver_ToPendingAcquire(InPool, InCurrent, ConstructedEntity))
        { return; }

        ConstructedEntity.AddOrGet<FTag_EntityPool_Dormant>();
        InCurrent._DormantEntities.Emplace(ConstructedEntity);
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoHandleRequest(
            HandleType InPool,
            const FFragment_EntityPool_Params& InParams,
            FFragment_EntityPool_Current& InCurrent,
            const FRequest_EntityPool_HandleDestroyed& InRequest)
        -> void
    {
        --InCurrent._NumLiveInstances;

        if (InRequest.Get_WasDormant())
        {
            InCurrent._DormantEntities.Remove(InRequest.Get_DestroyedEntity());

            CK_TRIGGER_ENSURE(TEXT("Dormant pooled Entity [{}] owned by Pool [{}] was destroyed externally. "
                "The Pool has evicted it from its free list, but destroying a Pool-owned dormant Entity indicates a lifetime bug in the caller"),
                InRequest.Get_DestroyedEntity(), InPool);
        }
        else if (InRequest.Get_WasInUse())
        {
            --InCurrent._NumInUse;
            pool::Verbose(TEXT("In-use pooled Entity [{}] was destroyed instead of Released — Pool [{}] forgets it (steal semantics)"),
                InRequest.Get_DestroyedEntity(), InPool);
        }
        // else: died mid-construction; _NumSpawnsInFlight is reconciled by its HandleConstructed request
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoDeliver(
            HandleType InPool,
            FFragment_EntityPool_Current& InCurrent,
            FCk_Handle InEntity,
            const FInstancedStruct& InPerUseParams,
            FCk_Handle InTicket)
        -> void
    {
        InEntity.Try_Remove<FTag_EntityPool_Dormant>();
        InEntity.AddOrGet<FTag_EntityPool_InUse>();

        auto& Pooled = InEntity.AddOrGet<FFragment_EntityPooled>();
        ++Pooled._UseGeneration;

        ++InCurrent._NumInUse;
        InCurrent._HighWaterMark = FMath::Max(InCurrent._HighWaterMark, InCurrent._NumInUse);

        UUtils_Signal_OnEntityPool_EntityAcquired::Broadcast(InEntity, MakePayload(InEntity, InPerUseParams));

        UCk_Utils_PoolableReceiver_UE::Broadcast_AcquiredFromPool_OnObject(
            ck_entity_pool_processor::TryGet_ScriptInstance(InEntity), InPerUseParams);

        if (ck::IsValid(InTicket))
        {
            UUtils_Signal_OnEntityPool_AcquireFulfilled::Broadcast(InTicket,
                MakePayload(FCk_EntityPool_AcquireResult{InPool, ECk_SucceededFailed::Succeeded, InEntity}));

            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InTicket);
        }
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoTryDeliver_ToPendingAcquire(
            HandleType InPool,
            FFragment_EntityPool_Current& InCurrent,
            FCk_Handle InEntity)
        -> bool
    {
        while (InCurrent._PendingAcquires.Num() > 0)
        {
            const auto Pending = InCurrent._PendingAcquires[0];
            InCurrent._PendingAcquires.RemoveAt(0);

            // requester may have abandoned the acquire (ticket destroyed) — skip to the next one
            if (ck::Is_NOT_Valid(Pending.Get_TicketEntity()))
            { continue; }

            DoDeliver(InPool, InCurrent, InEntity, Pending.Get_PerUseParams(), Pending.Get_TicketEntity());
            return true;
        }

        return false;
    }

    auto
        FProcessor_EntityPool_HandleRequests::
        DoFulfill_Failed(
            HandleType InPool,
            FCk_Handle InTicket)
        -> void
    {
        if (ck::Is_NOT_Valid(InTicket))
        { return; }

        UUtils_Signal_OnEntityPool_AcquireFulfilled::Broadcast(InTicket,
            MakePayload(FCk_EntityPool_AcquireResult{InPool, ECk_SucceededFailed::Failed, FCk_Handle{}}));

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InTicket);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityPool_HandleDestroyedPooledEntity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEntity,
            const FFragment_EntityPooled& InPooled)
        -> void
    {
        auto OwningPool = InPooled.Get_OwningPool();

        // cascade teardown: the pool itself is being destroyed (or is already gone) — nothing to reconcile
        if (ck::Is_NOT_Valid(OwningPool))
        { return; }

        UCk_Utils_EntityPool_UE::DoRequest_HandleDestroyed(OwningPool, InEntity,
            InEntity.Has<FTag_EntityPool_Dormant>(), InEntity.Has<FTag_EntityPool_InUse>());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityPool_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPool,
            FFragment_EntityPool_Current& InCurrent)
        -> void
    {
        for (const auto& Pending : InCurrent._PendingAcquires)
        {
            auto Ticket = Pending.Get_TicketEntity();

            // tickets are lifetime children of the pool — they are pending-kill too by now, but their signal
            // fragments are still alive during the EndPlay window, so bound promises still get their Failed result
            if (ck::Is_NOT_Valid(Ticket, ck::IsValid_Policy_IncludePendingKill{}))
            { continue; }

            UUtils_Signal_OnEntityPool_AcquireFulfilled::Broadcast(Ticket,
                MakePayload(FCk_EntityPool_AcquireResult{InPool, ECk_SucceededFailed::Failed, FCk_Handle{}}));
        }

        InCurrent._PendingAcquires.Reset();
        InCurrent._DormantEntities.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
