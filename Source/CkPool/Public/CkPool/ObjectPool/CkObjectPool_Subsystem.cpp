#include "CkObjectPool_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkPool/CkPool_Log.h"
#include "CkPool/ObjectPool/CkObjectPool_Fragment.h"
#include "CkPool/ObjectPool/CkObjectPool_Poolable.h"
#include "CkPool/ObjectPool/CkObjectPool_Utils.h"
#include "CkPool/Poolable/CkPoolableReceiver_Utils.h"

#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <TimerManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_object_pool_subsystem
{
    auto
        Get_ImplementsPoolable(
            const UObject* InObject)
        -> bool
    {
        return InObject->GetClass()->ImplementsInterface(UCk_ObjectPool_Poolable::StaticClass());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPool_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    Super::Tick(InDeltaTime);

    for (auto& Kvp : _Pools)
    {
        auto& Pool = Kvp.Value;

        if (Pool._NumPrewarmRemaining <= 0)
        { continue; }

        const auto Budget = FMath::Max(1, Pool._Params.Get_PrewarmBudgetPerTick());
        const auto NumToSpawn = FMath::Min(Budget, Pool._NumPrewarmRemaining);

        for (auto Index = 0; Index < NumToSpawn; ++Index)
        {
            auto NewInstance = DoSpawn_Instance(Pool);

            if (ck::Is_NOT_Valid(NewInstance, ck::IsValid_Policy_NullptrOnly{}))
            { break; }

            Pool._FreeObjects.Emplace(NewInstance);
        }

        Pool._NumPrewarmRemaining -= NumToSpawn;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPool_Subsystem_UE::
    DoGetOrCreate_Pool(
        const FCk_ObjectPool_ParamsData& InParams)
    -> FCk_ObjectPool_PoolState*
{
    const auto& ObjectClass = InParams.Get_ObjectClass();

    CK_ENSURE_IF_NOT(ck::IsValid(ObjectClass), TEXT("Cannot create an ObjectPool with an INVALID class"))
    { return {}; }

    if (auto* Existing = _Pools.Find(ObjectClass))
    { return Existing; }

    if (const auto ActorCDO = Cast<AActor>(ObjectClass->GetDefaultObject());
        ck::IsValid(ActorCDO, ck::IsValid_Policy_NullptrOnly{}))
    {
        CK_ENSURE_IF_NOT(NOT ActorCDO->GetIsReplicated(),
            TEXT("ObjectPool only supports non-replicated Actor classes (v1). [{}] replicates — "
                 "pooled actors bypass the spawn/channel lifecycle replication relies on"), ObjectClass)
        { return {}; }
    }

    auto& NewPool = _Pools.Add(ObjectClass);
    NewPool._Params = InParams;

    auto PrewarmCount = InParams.Get_PrewarmCount();

    if (InParams.Get_CapacityPolicy() == ECk_Pool_CapacityPolicy::Bounded)
    { PrewarmCount = FMath::Min(PrewarmCount, InParams.Get_MaxSize()); }

    NewPool._NumPrewarmRemaining = PrewarmCount;

    // registry entity + record membership (synchronous) — mirrors this pool into ECS for
    // enumeration/tooling; the acquire/release hot path never touches it
    auto PoolEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(this);
    PoolEntity.Add<ck::FFragment_ObjectPool_PoolInfo>(ck::FFragment_ObjectPool_PoolInfo{ObjectClass});

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(PoolEntity, *ck::Format_UE(TEXT("ObjectPool [{}]"), ObjectClass));
#endif

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());
    ck::RecordOfObjectPools_Utils::AddIfMissing(TransientEntity);
    ck::RecordOfObjectPools_Utils::Request_Connect(TransientEntity, PoolEntity, ECk_Record_LabelRequirementPolicy::Optional);

    NewPool._PoolEntity = PoolEntity;

    ck::pool::Verbose(TEXT("Created ObjectPool for class [{}] (prewarm: [{}], registry entity: [{}])"), ObjectClass, PrewarmCount, PoolEntity);

    return &NewPool;
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoAcquire(
        const TSubclassOf<UObject>& InObjectClass,
        const FTransform& InTransform,
        bool InHasTransform,
        const FInstancedStruct& InPerUseParams)
    -> UObject*
{
    auto* Pool = _Pools.Find(InObjectClass);

    CK_ENSURE_IF_NOT(ck::IsValid(Pool, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("No ObjectPool exists for class [{}] — the caller (Utils) should have auto-created it"), InObjectClass)
    { return {}; }

    DoSweep_NullSlots(*Pool);

    auto AcquiredObject = static_cast<UObject*>(nullptr);

    while (Pool->_FreeObjects.Num() > 0)
    {
        auto Candidate = Pool->_FreeObjects.Pop(EAllowShrinking::No).Get();

        if (ck::Is_NOT_Valid(Candidate))
        { continue; }

        AcquiredObject = Candidate;
        ++Pool->_NumHits;
        break;
    }

    if (ck::Is_NOT_Valid(AcquiredObject, ck::IsValid_Policy_NullptrOnly{}))
    {
        ++Pool->_NumMisses;

        if (Pool->_Params.Get_ExhaustionPolicy() == ECk_Pool_ExhaustionPolicy::Fail)
        {
            ck::pool::Verbose(TEXT("ObjectPool [{}] is empty and Fail policy is set — returning null"), InObjectClass);
            return {};
        }

        const auto CanCreateMore = Pool->_Params.Get_CapacityPolicy() == ECk_Pool_CapacityPolicy::Unbounded ||
            Pool->_NumLiveInstances < Pool->_Params.Get_MaxSize();

        if (NOT CanCreateMore)
        {
            ck::pool::Verbose(TEXT("ObjectPool [{}] is at capacity [{}] — returning null (synchronous pool has no parking)"),
                InObjectClass, Pool->_Params.Get_MaxSize());
            return {};
        }

        AcquiredObject = DoSpawn_Instance(*Pool);

        if (ck::Is_NOT_Valid(AcquiredObject, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        // batch growth: TOP UP the queued extras to (GrowBatchCount - 1) through the amortized
        // prewarm tick — never synchronously in the acquire call, and a burst of misses provisions
        // ONE batch (top-up, not add-per-miss); clamped to capacity
        if (const auto ExtraGrow = Pool->_Params.Get_GrowBatchCount() - 1 - Pool->_NumPrewarmRemaining;
            ExtraGrow > 0)
        {
            auto AllowedExtra = ExtraGrow;

            if (Pool->_Params.Get_CapacityPolicy() == ECk_Pool_CapacityPolicy::Bounded)
            {
                AllowedExtra = FMath::Min(AllowedExtra,
                    Pool->_Params.Get_MaxSize() - Pool->_NumLiveInstances - Pool->_NumPrewarmRemaining);
            }

            if (AllowedExtra > 0)
            { Pool->_NumPrewarmRemaining += AllowedExtra; }
        }
    }

    Pool->_InUseObjects.Emplace(AcquiredObject);
    Pool->_HighWaterMark = FMath::Max(Pool->_HighWaterMark, Pool->_InUseObjects.Num());

    DoThaw(AcquiredObject, InTransform, InHasTransform);

    if (ck_object_pool_subsystem::Get_ImplementsPoolable(AcquiredObject))
    {
        ICk_ObjectPool_Poolable::Execute_PrepareForUse(AcquiredObject);
    }

    UCk_Utils_PoolableReceiver_UE::Broadcast_AcquiredFromPool_OnObject(AcquiredObject, InPerUseParams);

    return AcquiredObject;
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoRelease(
        UObject* InObject)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Cannot Release an INVALID object to its ObjectPool"))
    { return; }

    auto* Pool = _Pools.Find(InObject->GetClass());

    CK_ENSURE_IF_NOT(ck::IsValid(Pool, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Object [{}] does not belong to any ObjectPool (no pool exists for class [{}])"),
        InObject, InObject->GetClass())
    { return; }

    const auto NumRemoved = Pool->_InUseObjects.Remove(InObject);

    CK_ENSURE_IF_NOT(NumRemoved > 0,
        TEXT("Object [{}] is not in-use in its ObjectPool — double-Release or an object the pool never vended"),
        InObject)
    { return; }

    const auto InterfaceVeto = ck_object_pool_subsystem::Get_ImplementsPoolable(InObject) &&
        NOT ICk_ObjectPool_Poolable::Execute_Get_CanBePooled(InObject);
    const auto ReceiverVeto = NOT UCk_Utils_PoolableReceiver_UE::Get_CanBePooled_OnObject(InObject);

    if (InterfaceVeto || ReceiverVeto)
    {
        ck::pool::Verbose(TEXT("Object [{}] vetoed pooling (CanBePooled == false) — destroying instead"), InObject);
        --Pool->_NumLiveInstances;
        DoDestroy_Instance(InObject);
        return;
    }

    if (ck_object_pool_subsystem::Get_ImplementsPoolable(InObject))
    {
        ICk_ObjectPool_Poolable::Execute_PrepareForPool(InObject);
    }

    UCk_Utils_PoolableReceiver_UE::Broadcast_ReleasedToPool_OnObject(InObject);

    DoFreeze(InObject);
    Pool->_FreeObjects.Emplace(InObject);
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoDestroyPool(
        const TSubclassOf<UObject>& InObjectClass)
    -> void
{
    auto* Pool = _Pools.Find(InObjectClass);

    if (ck::Is_NOT_Valid(Pool, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    for (const auto& FreeObject : Pool->_FreeObjects)
    {
        if (auto* Object = FreeObject.Get();
            ck::IsValid(Object))
        {
            DoDestroy_Instance(Object);
        }
    }

    if (ck::IsValid(Pool->_PoolEntity))
    { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Pool->_PoolEntity); }

    // in-use instances are the consumers' to finish with — they simply stop being pooled
    ck::pool::Verbose(TEXT("Destroyed ObjectPool for class [{}] ([{}] in-use instances released from tracking)"),
        InObjectClass, Pool->_InUseObjects.Num());

    _Pools.Remove(InObjectClass);
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoGet_Stats(
        const TSubclassOf<UObject>& InObjectClass)
    -> FCk_ObjectPool_Stats
{
    auto* Pool = _Pools.Find(InObjectClass);

    if (ck::Is_NOT_Valid(Pool, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    DoSweep_NullSlots(*Pool);

    return FCk_ObjectPool_Stats{}
        .Set_NumFree(Pool->_FreeObjects.Num())
        .Set_NumInUse(Pool->_InUseObjects.Num())
        .Set_NumLiveInstances(Pool->_NumLiveInstances)
        .Set_NumPrewarmRemaining(Pool->_NumPrewarmRemaining)
        .Set_HighWaterMark(Pool->_HighWaterMark)
        .Set_NumHits(Pool->_NumHits)
        .Set_NumMisses(Pool->_NumMisses);
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoGet_IsPooledObject(
        const UObject* InObject)
    -> bool
{
    if (ck::Is_NOT_Valid(InObject, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    const auto* Pool = _Pools.Find(InObject->GetClass());

    if (ck::Is_NOT_Valid(Pool, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return Pool->_FreeObjects.Contains(InObject) || Pool->_InUseObjects.Contains(InObject);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPool_Subsystem_UE::
    DoSpawn_Instance(
        FCk_ObjectPool_PoolState& InPool)
    -> UObject*
{
    const auto& ObjectClass = InPool._Params.Get_ObjectClass();

    auto CreatedInstance = static_cast<UObject*>(nullptr);

    if (ObjectClass->IsChildOf<AActor>())
    {
        auto World = GetWorld();

        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("ObjectPool has no World to spawn Actor [{}] into"), ObjectClass)
        { return {}; }

        auto SpawnParams = FActorSpawnParameters{};
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        const auto SpawnTransform = FTransform::Identity;
        CreatedInstance = World->SpawnActor(ObjectClass, &SpawnTransform, SpawnParams);
    }
    else
    {
        CreatedInstance = NewObject<UObject>(this, ObjectClass);
    }

    CK_ENSURE_IF_NOT(ck::IsValid(CreatedInstance), TEXT("ObjectPool failed to create an instance of [{}]"), ObjectClass)
    { return {}; }

    DoFreeze(CreatedInstance);
    ++InPool._NumLiveInstances;

    ck::pool::VeryVerbose(TEXT("ObjectPool created instance [{}] of [{}] (live: [{}])"),
        CreatedInstance, ObjectClass, InPool._NumLiveInstances);

    return CreatedInstance;
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoDestroy_Instance(
        UObject* InObject)
    -> void
{
    if (auto* Actor = Cast<AActor>(InObject);
        ck::IsValid(Actor, ck::IsValid_Policy_NullptrOnly{}))
    {
        Actor->Destroy();
        return;
    }

    InObject->MarkAsGarbage();
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoSweep_NullSlots(
        FCk_ObjectPool_PoolState& InPool)
    -> void
{
    const auto NumSweptFree = InPool._FreeObjects.RemoveAll([](const TObjectPtr<UObject>& InSlot)
    {
        return ck::Is_NOT_Valid(InSlot.Get());
    });

    const auto NumSweptInUse = InPool._InUseObjects.RemoveAll([](const TObjectPtr<UObject>& InSlot)
    {
        return ck::Is_NOT_Valid(InSlot.Get());
    });

    if (NumSweptFree > 0)
    {
        // externally destroying a pool-owned FREE instance is a caller lifetime bug (mirror of the
        // EntityPool dormant-destroy ensure); in-use external destroys are legal "steal" semantics
        CK_TRIGGER_ENSURE(TEXT("[{}] FREE pooled instance(s) of [{}] were destroyed externally — "
            "the ObjectPool owns its free instances; destroying them indicates a lifetime bug in the caller"),
            NumSweptFree, InPool._Params.Get_ObjectClass());
    }

    InPool._NumLiveInstances -= (NumSweptFree + NumSweptInUse);

    if (NumSweptInUse > 0)
    {
        ck::pool::Verbose(TEXT("ObjectPool [{}]: [{}] in-use instance(s) destroyed externally instead of Released — forgotten (steal semantics)"),
            InPool._Params.Get_ObjectClass(), NumSweptInUse);
    }
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoFreeze(
        UObject* InObject)
    -> void
{
    auto* Actor = Cast<AActor>(InObject);

    if (ck::Is_NOT_Valid(Actor, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    Actor->SetActorHiddenInGame(true);
    Actor->SetActorEnableCollision(false);
    Actor->SetActorTickEnabled(false);
    Actor->GetWorldTimerManager().ClearAllTimersForObject(Actor);
}

auto
    UCk_ObjectPool_Subsystem_UE::
    DoThaw(
        UObject* InObject,
        const FTransform& InTransform,
        bool InHasTransform)
    -> void
{
    auto* Actor = Cast<AActor>(InObject);

    if (ck::Is_NOT_Valid(Actor, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    if (InHasTransform)
    {
        constexpr auto SweepDuringTeleport = false;
        Actor->SetActorTransform(InTransform, SweepDuringTeleport, nullptr, ETeleportType::TeleportPhysics);
    }

    // restore to the class's authored defaults — the pool froze these on release
    const auto ActorCDO = Actor->GetClass()->GetDefaultObject<AActor>();
    Actor->SetActorHiddenInGame(ActorCDO->IsHidden());
    Actor->SetActorEnableCollision(ActorCDO->GetActorEnableCollision());
    Actor->SetActorTickEnabled(ActorCDO->PrimaryActorTick.bStartWithTickEnabled);
}

// --------------------------------------------------------------------------------------------------------------------
