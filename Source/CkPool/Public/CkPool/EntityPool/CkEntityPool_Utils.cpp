#include "CkEntityPool_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkPool/CkPool_Log.h"
#include "CkPool/EntityPool/CkEntityPool_Subsystem.h"
#include "CkPool/Settings/CkPool_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    Request_CreatePool(
        const UObject* InWorldContextObject,
        const FCk_Fragment_EntityPool_ParamsData& InParams)
    -> FCk_Handle_EntityPool
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("EntityPool subsystem is invalid when creating a Pool"))
    { return {}; }

    return PoolSubsystem->DoGetOrCreate_Pool(InParams);
}

auto
    UCk_Utils_EntityPool_UE::
    Request_DestroyPool(
        const UObject* InWorldContextObject,
        FCk_Handle_EntityPool& InPool)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPool), TEXT("Cannot Destroy an INVALID EntityPool"))
    { return; }

    if (auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);
        ck::IsValid(PoolSubsystem))
    {
        PoolSubsystem->DoForget_Pool(InPool);
    }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InPool);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    Request_Acquire(
        const UObject* InWorldContextObject,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        const FInstancedStruct& InPerUseParams)
    -> FCk_Handle_PendingEntityPoolAcquire
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("EntityPool subsystem is invalid when acquiring from the Pool of [{}]"),
        InEntityScriptClass)
    { return {}; }

    // auto-create with the class's Pool project-settings entry if one exists, else built-in defaults.
    // An explicit Request_CreatePool earlier wins outright (GetOrCreate returns the existing pool)
    auto AutoCreateParams = FCk_Fragment_EntityPool_ParamsData{InEntityScriptClass};

    if (const auto SettingsEntry = UCk_Pool_ProjectSettings_UE::TryGet_EntityPoolEntry(InEntityScriptClass);
        SettingsEntry.IsSet())
    {
        AutoCreateParams.Set_PrewarmCount(SettingsEntry->Get_PrewarmCount())
                        .Set_PrewarmBudgetPerTick(SettingsEntry->Get_PrewarmBudgetPerTick())
                        .Set_CapacityPolicy(SettingsEntry->Get_CapacityPolicy())
                        .Set_MaxSize(SettingsEntry->Get_MaxSize())
                        .Set_ExhaustionPolicy(SettingsEntry->Get_ExhaustionPolicy())
                        .Set_GrowBatchCount(SettingsEntry->Get_GrowBatchCount());
    }

    auto Pool = PoolSubsystem->DoGetOrCreate_Pool(AutoCreateParams);

    if (ck::Is_NOT_Valid(Pool))
    { return {}; }

    return Request_Acquire_OnPool(Pool, InPerUseParams);
}

auto
    UCk_Utils_EntityPool_UE::
    Request_Acquire_WithPoolParams(
        const UObject* InWorldContextObject,
        const FCk_Fragment_EntityPool_ParamsData& InPoolParams,
        const FInstancedStruct& InPerUseParams)
    -> FCk_Handle_PendingEntityPoolAcquire
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PoolSubsystem), TEXT("EntityPool subsystem is invalid when acquiring from the Pool of [{}]"),
        InPoolParams.Get_EntityScriptClass())
    { return {}; }

    auto Pool = PoolSubsystem->DoGetOrCreate_Pool(InPoolParams);

    if (ck::Is_NOT_Valid(Pool))
    { return {}; }

    return Request_Acquire_OnPool(Pool, InPerUseParams);
}

auto
    UCk_Utils_EntityPool_UE::
    Request_Acquire_OnPool(
        FCk_Handle_EntityPool& InPool,
        const FInstancedStruct& InPerUseParams)
    -> FCk_Handle_PendingEntityPoolAcquire
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPool), TEXT("Cannot Acquire from an INVALID EntityPool"))
    { return {}; }

    auto Ticket = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InPool);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(Ticket, *ck::Format_UE(TEXT("EntityPool AcquireTicket of [{}]"), InPool));
#endif

    InPool.AddOrGet<ck::FFragment_EntityPool_Requests>()._Requests.Emplace(
        ck::FRequest_EntityPool_Acquire{Ticket}.Set_PerUseParams(InPerUseParams));

    return FCk_Handle_PendingEntityPoolAcquire{Ticket};
}

auto
    UCk_Utils_EntityPool_UE::
    Request_ReleaseToPool(
        FCk_Handle& InPooledEntity)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPooledEntity), TEXT("Cannot Release an INVALID Entity to its Pool"))
    { return InPooledEntity; }

    CK_ENSURE_IF_NOT(Get_IsPooledEntity(InPooledEntity), TEXT("Cannot Release Entity [{}] — it is not a pooled Entity"),
        InPooledEntity)
    { return InPooledEntity; }

    auto OwningPool = InPooledEntity.Get<ck::FFragment_EntityPooled>().Get_OwningPool();

    CK_ENSURE_IF_NOT(ck::IsValid(OwningPool), TEXT("Cannot Release Entity [{}] — its owning Pool is no longer valid"),
        InPooledEntity)
    { return InPooledEntity; }

    OwningPool.AddOrGet<ck::FFragment_EntityPool_Requests>()._Requests.Emplace(
        ck::FRequest_EntityPool_Release{InPooledEntity});

    return InPooledEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    TryGet_Pool_ByClass(
        const UObject* InWorldContextObject,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass)
    -> FCk_Handle_EntityPool
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(PoolSubsystem))
    { return {}; }

    return PoolSubsystem->DoTryGet_Pool_ByClass(InEntityScriptClass);
}

auto
    UCk_Utils_EntityPool_UE::
    TryGet_Pool_ByName(
        const UObject* InWorldContextObject,
        FGameplayTag InPoolName)
    -> FCk_Handle_EntityPool
{
    auto PoolSubsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(PoolSubsystem))
    { return {}; }

    return PoolSubsystem->DoTryGet_Pool_ByName(InPoolName);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityPool_UE, FCk_Handle_EntityPool,
    ck::FFragment_EntityPool_Params, ck::FFragment_EntityPool_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    Get_Stats(
        const FCk_Handle_EntityPool& InPool)
    -> FCk_EntityPool_Stats
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPool), TEXT("Cannot get Stats of an INVALID EntityPool"))
    { return {}; }

    const auto& Current = InPool.Get<ck::FFragment_EntityPool_Current>();

    return FCk_EntityPool_Stats{}
        .Set_NumDormant(Current.Get_DormantEntities().Num())
        .Set_NumInUse(Current.Get_NumInUse())
        .Set_NumLiveInstances(Current.Get_NumLiveInstances())
        .Set_NumSpawnsInFlight(Current.Get_NumSpawnsInFlight())
        .Set_NumPendingAcquires(Current.Get_PendingAcquires().Num())
        .Set_NumPrewarmRemaining(Current.Get_NumPrewarmRemaining())
        .Set_HighWaterMark(Current.Get_HighWaterMark())
        .Set_NumHits(Current.Get_NumHits())
        .Set_NumMisses(Current.Get_NumMisses());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    Get_IsPooledEntity(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_EntityPooled>();
}

auto
    UCk_Utils_EntityPool_UE::
    Get_IsDormant(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FTag_EntityPool_Dormant>();
}

auto
    UCk_Utils_EntityPool_UE::
    TryGet_OwningPool(
        const FCk_Handle& InHandle)
    -> FCk_Handle_EntityPool
{
    if (NOT Get_IsPooledEntity(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_EntityPooled>().Get_OwningPool();
}

auto
    UCk_Utils_EntityPool_UE::
    Get_UseGeneration(
        const FCk_Handle& InHandle)
    -> int32
{
    CK_ENSURE_IF_NOT(Get_IsPooledEntity(InHandle), TEXT("Entity [{}] is not pooled — it has no Use Generation"), InHandle)
    { return INDEX_NONE; }

    return InHandle.Get<ck::FFragment_EntityPooled>().Get_UseGeneration();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    BindTo_OnAcquiredFromPool(
        FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityAcquired& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnEntityPool_EntityAcquired, InPooledEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InPooledEntity;
}

auto
    UCk_Utils_EntityPool_UE::
    UnbindFrom_OnAcquiredFromPool(
        FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityAcquired& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnEntityPool_EntityAcquired, InPooledEntity, InDelegate);
    return InPooledEntity;
}

auto
    UCk_Utils_EntityPool_UE::
    BindTo_OnReleasedToPool(
        FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityReleased& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnEntityPool_EntityReleased, InPooledEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InPooledEntity;
}

auto
    UCk_Utils_EntityPool_UE::
    UnbindFrom_OnReleasedToPool(
        FCk_Handle& InPooledEntity,
        const FCk_Delegate_EntityPool_EntityReleased& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnEntityPool_EntityReleased, InPooledEntity, InDelegate);
    return InPooledEntity;
}

auto
    UCk_Utils_EntityPool_UE::
    BindTo_OnExhausted(
        FCk_Handle_EntityPool& InPool,
        const FCk_Delegate_EntityPool_Exhausted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_EntityPool
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnEntityPool_Exhausted, InPool, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InPool;
}

auto
    UCk_Utils_EntityPool_UE::
    UnbindFrom_OnExhausted(
        FCk_Handle_EntityPool& InPool,
        const FCk_Delegate_EntityPool_Exhausted& InDelegate)
    -> FCk_Handle_EntityPool
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnEntityPool_Exhausted, InPool, InDelegate);
    return InPool;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityPool_UE::
    DoGet_Subsystem(
        const UObject* InWorldContextObject)
    -> UCk_EntityPool_Subsystem_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("WorldContextObject is invalid when resolving the EntityPool subsystem"))
    { return {}; }

    auto World = InWorldContextObject->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("World is invalid when resolving the EntityPool subsystem"))
    { return {}; }

    return World->GetSubsystem<UCk_EntityPool_Subsystem_UE>();
}

auto
    UCk_Utils_EntityPool_UE::
    DoCreate_PoolEntity(
        UCk_EntityPool_Subsystem_UE* InSubsystem,
        const FCk_Fragment_EntityPool_ParamsData& InParams)
    -> FCk_Handle_EntityPool
{
    auto EffectiveParams = InParams;
    const auto& Archetype = InParams.Get_EntityScriptArchetype();

    if (ck::IsValid(Archetype))
    {
        CK_ENSURE_IF_NOT(InParams.Get_PoolName().IsValid(),
            TEXT("EntityPool with archetype [{}] requires a _PoolName — the class-keyed default pool "
                 "cannot distinguish two archetypes of the same class"), Archetype)
        { return {}; }

        CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(InParams.Get_EntityScriptClass()) || InParams.Get_EntityScriptClass() == Archetype->GetClass(),
            TEXT("EntityPool params declare class [{}] but the archetype [{}] is of class [{}]"),
            InParams.Get_EntityScriptClass(), Archetype, Archetype->GetClass())
        { return {}; }

        // essentials ride the ctor — rebuild the params with the class derived from the archetype
        EffectiveParams = FCk_Fragment_EntityPool_ParamsData{Archetype->GetClass()}
            .Set_EntityScriptArchetype(InParams.Get_EntityScriptArchetype())
            .Set_PoolName(InParams.Get_PoolName())
            .Set_ConstructionSpawnParams(InParams.Get_ConstructionSpawnParams())
            .Set_PrewarmCount(InParams.Get_PrewarmCount())
            .Set_PrewarmBudgetPerTick(InParams.Get_PrewarmBudgetPerTick())
            .Set_CapacityPolicy(InParams.Get_CapacityPolicy())
            .Set_MaxSize(InParams.Get_MaxSize())
            .Set_ExhaustionPolicy(InParams.Get_ExhaustionPolicy())
            .Set_GrowBatchCount(InParams.Get_GrowBatchCount());
    }

    CK_ENSURE_IF_NOT(ck::IsValid(EffectiveParams.Get_EntityScriptClass()),
        TEXT("Cannot create an EntityPool with an INVALID EntityScript class"))
    { return {}; }

    const auto CDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_EntityScript_UE>(EffectiveParams.Get_EntityScriptClass());

    CK_ENSURE_IF_NOT(ck::IsValid(CDO), TEXT("Could not get the CDO of EntityScript class [{}]"), EffectiveParams.Get_EntityScriptClass())
    { return {}; }

    // for archetype pools the replication verdict must come from the authored instance, not the CDO
    const auto* ReplicationSource = ck::IsValid(Archetype) ? Archetype.Get() : CDO;

    CK_ENSURE_IF_NOT(ReplicationSource->Get_EffectiveReplication() == ECk_Replication::DoesNotReplicate,
        TEXT("EntityPool only supports DoesNotReplicate EntityScripts (v1). [{}] declares Replicates — "
             "pooled entities bypass the spawn path replication relies on"), EffectiveParams.Get_EntityScriptClass())
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InSubsystem);

    if (InParams.Get_PoolName().IsValid())
    {
        UCk_Utils_GameplayLabel_UE::Add(NewEntity, InParams.Get_PoolName());
    }
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    else
    {
        UCk_Utils_Handle_UE::Set_DebugName(NewEntity, *ck::Format_UE(TEXT("EntityPool [{}]"), EffectiveParams.Get_EntityScriptClass()));
    }
#endif

    NewEntity.Add<ck::FFragment_EntityPool_Params>(EffectiveParams);
    auto& Current = NewEntity.Add<ck::FFragment_EntityPool_Current>();
    // pin synchronously — the pool may grow before its Setup processor runs, and fragments are not GC-traced
    Current._PinnedArchetype = TStrongObjectPtr<UCk_EntityScript_UE>{EffectiveParams.Get_EntityScriptArchetype().Get()};
    NewEntity.Add<ck::FTag_EntityPool_NeedsSetup>();

    auto NewPool = ck::StaticCast<FCk_Handle_EntityPool>(NewEntity);

    // record membership (synchronous) — the world's transient entity carries the registry of all pools
    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InSubsystem->GetWorld());
    ck::RecordOfEntityPools_Utils::AddIfMissing(TransientEntity);
    ck::RecordOfEntityPools_Utils::Request_Connect(TransientEntity, NewPool, ECk_Record_LabelRequirementPolicy::Optional);

    return NewPool;
}

auto
    UCk_Utils_EntityPool_UE::
    Get_AllPools(
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_EntityPool>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(InWorldContextObject->GetWorld()),
        TEXT("Invalid WorldContextObject when enumerating EntityPools"))
    { return {}; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorldContextObject->GetWorld());

    if (NOT ck::RecordOfEntityPools_Utils::Has(TransientEntity))
    { return {}; }

    return ck::RecordOfEntityPools_Utils::Get_ValidEntries(TransientEntity);
}

auto
    UCk_Utils_EntityPool_UE::
    DoRequest_HandleConstructed(
        FCk_Handle_EntityPool& InPool,
        FCk_Handle InConstructedEntity)
    -> void
{
    // the pool may have been torn down while the spawn was in flight — the instance is a lifetime child of
    // the pool, so the destruction cascade already owns it; nothing to reconcile
    if (ck::Is_NOT_Valid(InPool))
    { return; }

    InPool.AddOrGet<ck::FFragment_EntityPool_Requests>()._Requests.Emplace(
        ck::FRequest_EntityPool_HandleConstructed{InConstructedEntity});
}

auto
    UCk_Utils_EntityPool_UE::
    DoRequest_HandleDestroyed(
        FCk_Handle_EntityPool InPool,
        FCk_Handle InDestroyedEntity,
        bool InWasDormant,
        bool InWasInUse)
    -> void
{
    InPool.AddOrGet<ck::FFragment_EntityPool_Requests>()._Requests.Emplace(
        ck::FRequest_EntityPool_HandleDestroyed{InDestroyedEntity, InWasDormant, InWasInUse});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PendingEntityPoolAcquire_UE::
    Promise_OnAcquired(
        FCk_Handle_PendingEntityPoolAcquire& InPendingAcquire,
        const FCk_Delegate_EntityPool_Acquired& InDelegate)
    -> void
{
    // Request_Acquire returns an INVALID pending handle when the acquire was rejected up-front (it already
    // ensured naming the reason) — there is no ticket, so binding would AddOrGet on a tombstone handle.
    // A DESTROYED ticket lands here too: the acquire was already fulfilled and this bind arrived too late
    // (the contract is bind-same-frame; see the UFUNCTION comment). No-op either way — the delegate never fires
    if (ck::Is_NOT_Valid(InPendingAcquire.Get_TicketEntity()))
    { return; }

    auto Ticket = InPendingAcquire.Get_TicketEntity();

    ck::UUtils_Signal_OnEntityPool_AcquireFulfilled_PostFireUnbind::Bind(
        Ticket, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_PendingEntityPoolAcquire_UE::
    Get_IsValid(
        const FCk_Handle_PendingEntityPoolAcquire& InPendingAcquire)
    -> bool
{
    return ck::IsValid(InPendingAcquire);
}

// --------------------------------------------------------------------------------------------------------------------
