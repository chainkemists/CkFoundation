#include "CkObjectPooling_Subsystem.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/ObjectPooling/CkObjectPoolingParticipant.h"
#include "CkCore/ObjectPooling/CkObjectPoolingParticipant_Utils.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Settings.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>
#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
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
    UCk_ObjectPooling_Subsystem_UE::
    Get_PoolStats(
        const FCk_ObjectPooling_PoolKey& InKey) const
    -> FCk_ObjectPooling_PoolStats
{
    const auto* Pool = _Pools.Find(InKey);

    if (Pool == nullptr)
    { return {}; }

    return FCk_ObjectPooling_PoolStats{}
        .Set_NumFree(Pool->_FreeObjects.Num())
        .Set_NumInUse(Pool->_InUseObjects.Num())
        .Set_NumLiveInstances(Pool->_NumLiveInstances)
        .Set_NumPrewarmRemaining(Pool->_NumPrewarmRemaining)
        .Set_HighWaterMark(Pool->_HighWaterMark)
        .Set_NumHits(Pool->_NumHits)
        .Set_NumMisses(Pool->_NumMisses);
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    ForEach_Pool(
        const TFunction<void(const FCk_ObjectPooling_PoolKey&, const FCk_ObjectPooling_PoolStats&)>& InFunc) const
    -> void
{
    for (const auto& Kvp : _Pools)
    {
        InFunc(Kvp.Key, Get_PoolStats(Kvp.Key));
    }
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    Get_NumVendedUnique() const
    -> int32
{
    return _VendedUnique.Num();
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    Get_IsVendedObject(
        const UObject* InObject) const
    -> bool
{
    if (ck::Is_NOT_Valid(InObject))
    { return false; }

    if (_VendedUnique.Contains(const_cast<UObject*>(InObject)))
    { return true; }

    return _InstanceToPool.Contains(FObjectKey{InObject});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoRequest_Acquire(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams,
        UObject* InOuter)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass), TEXT("Cannot Acquire a pooled object with an INVALID class"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InClass->IsChildOf<AActor>(),
        TEXT("ObjectPooling vends plain UObjects only — [{}] is an Actor class. "
             "Actor creation/pooling goes through SpawnActor, not Request_CreateNewObject"), InClass)
    { return {}; }

    if (ck::IsValid(InArchetype, ck::IsValid_Policy_NullptrOnly{}))
    {
        CK_ENSURE_IF_NOT(InArchetype->GetClass() == InClass.Get(),
            TEXT("Archetype [{}] is not an instance of class [{}] — pools are keyed by (class, archetype) "
                 "and recycled instances reset to the archetype, so the classes must match exactly"),
            InArchetype, InClass)
        { return {}; }
    }

    auto* Archetype = ck::IsValid(InArchetype, ck::IsValid_Policy_NullptrOnly{})
        ? InArchetype
        : InClass->GetDefaultObject();

    if (InPoolParams.Get_RecyclePolicy() == ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease)
    {
        auto* EffectiveOuter = ck::IsValid(InOuter) ? InOuter : static_cast<UObject*>(GetWorld());
        auto* NewInstance = NewObject<UObject>(EffectiveOuter, InClass, NAME_None, RF_NoFlags, Archetype);

        _VendedUnique.Emplace(NewInstance);

        UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_AcquiredFromPool_OnObject(NewInstance);

        return NewInstance;
    }

    auto* Pool = DoGetOrCreate_Pool(InClass, Archetype, InPoolParams);

    CK_ENSURE_IF_NOT(ck::IsValid(Pool, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Failed to get-or-create an ObjectPool for class [{}]"), InClass)
    { return {}; }

    DoSweep_NullSlots(*Pool);

    auto AcquiredObject = static_cast<UObject*>(nullptr);
    auto WasRecycled = false;

    while (Pool->_FreeObjects.Num() > 0)
    {
        auto Candidate = Pool->_FreeObjects.Pop(EAllowShrinking::No).Get();

        if (ck::Is_NOT_Valid(Candidate))
        { continue; }

        AcquiredObject = Candidate;
        WasRecycled = true;
        ++Pool->_NumHits;
        break;
    }

    if (ck::Is_NOT_Valid(AcquiredObject, ck::IsValid_Policy_NullptrOnly{}))
    {
        ++Pool->_NumMisses;

        if (Pool->_Params.Get_ExhaustionPolicy() == ECk_ObjectPooling_ExhaustionPolicy::Fail)
        {
            ck::core::Verbose(TEXT("ObjectPool [{}] is empty and Fail policy is set — returning null"), Pool->Get_Class().Get());
            return {};
        }

        const auto CanCreateMore = Pool->_Params.Get_CapacityPolicy() == ECk_ObjectPooling_CapacityPolicy::Unbounded ||
            Pool->_NumLiveInstances < Pool->_Params.Get_MaxSize();

        if (NOT CanCreateMore)
        {
            ck::core::Verbose(TEXT("ObjectPool [{}] is at capacity [{}] — returning null"),
                Pool->Get_Class().Get(), Pool->_Params.Get_MaxSize());
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

            if (Pool->_Params.Get_CapacityPolicy() == ECk_ObjectPooling_CapacityPolicy::Bounded)
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
    _InstanceToPool.Add(FObjectKey{AcquiredObject}, FCk_ObjectPooling_PoolKey{Pool->Get_Class(), Pool->Get_Archetype()});

    // fresh instances already carry the archetype's values (NewObject template) — only a RECYCLED
    // instance needs the reset sweep (user contract: reset ONLY after it was recycled)
    if (WasRecycled)
    {
        DoReset_ToArchetype(AcquiredObject, Pool->Get_Archetype());
    }

    UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_AcquiredFromPool_OnObject(AcquiredObject);

    return AcquiredObject;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoTryReleaseToPool(
        UObject* InObject)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Cannot Release an INVALID object to its ObjectPool"))
    { return ECk_SucceededFailed::Failed; }

    if (_VendedUnique.Remove(InObject) > 0)
    {
        UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_ReleasedToPool_OnObject(InObject);
        return ECk_SucceededFailed::Succeeded;
    }

    const auto* PoolKey = _InstanceToPool.Find(FObjectKey{InObject});

    CK_ENSURE_IF_NOT(PoolKey != nullptr,
        TEXT("Object [{}] was never vended by the ObjectPooling subsystem (or was already released) — "
             "double-Release or an object created outside the pooling-aware Request_CreateNewObject"),
        InObject)
    { return ECk_SucceededFailed::Failed; }

    auto* Pool = _Pools.Find(*PoolKey);

    CK_ENSURE_IF_NOT(Pool != nullptr,
        TEXT("Object [{}] maps to an ObjectPool that no longer exists"), InObject)
    {
        _InstanceToPool.Remove(FObjectKey{InObject});
        return ECk_SucceededFailed::Failed;
    }

    const auto NumRemoved = Pool->_InUseObjects.Remove(InObject);
    _InstanceToPool.Remove(FObjectKey{InObject});

    CK_ENSURE_IF_NOT(NumRemoved > 0,
        TEXT("Object [{}] is not in-use in its ObjectPool — double-Release or a stolen instance"),
        InObject)
    { return ECk_SucceededFailed::Failed; }

    if (NOT UCk_Utils_ObjectPoolingParticipant_UE::Get_CanBePooled_OnObject(InObject))
    {
        ck::core::Verbose(TEXT("Object [{}] vetoed pooling (CanBePooled == false) — destroying instead"), InObject);
        --Pool->_NumLiveInstances;
        InObject->MarkAsGarbage();
        return ECk_SucceededFailed::Succeeded;
    }

    UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_ReleasedToPool_OnObject(InObject);

    Pool->_FreeObjects.Emplace(InObject);

    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoGetOrCreate_Pool(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams)
    -> FCk_ObjectPooling_PoolState*
{
    const auto PoolKey = FCk_ObjectPooling_PoolKey{InClass.Get(), InArchetype};

    if (auto* Existing = _Pools.Find(PoolKey))
    { return Existing; }

    // a project-settings entry for the class overrides the acquire-site params — it exists precisely
    // to let a project tune pools without touching call sites or assets. Read ONCE at pool creation
    const auto EffectiveParams = [&]
    {
        auto Params = InPoolParams;

        if (const auto SettingsEntry = UCk_ObjectPooling_ProjectSettings_UE::TryGet_PoolEntry(InClass))
        {
            Params = SettingsEntry->Get_PoolParams();
            Params.Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::Recycle);
        }

        return Params;
    }();

    auto& NewPool = _Pools.Add(PoolKey);
    NewPool._Params = EffectiveParams;
    NewPool._Class = InClass.Get();
    NewPool._Archetype = InArchetype;

    auto PrewarmCount = EffectiveParams.Get_PrewarmCount();

    if (EffectiveParams.Get_CapacityPolicy() == ECk_ObjectPooling_CapacityPolicy::Bounded)
    { PrewarmCount = FMath::Min(PrewarmCount, EffectiveParams.Get_MaxSize()); }

    NewPool._NumPrewarmRemaining = PrewarmCount;

    ck::core::Verbose(TEXT("Created ObjectPool for class [{}] archetype [{}] (prewarm: [{}])"),
        InClass, InArchetype, PrewarmCount);

    return &NewPool;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoSpawn_Instance(
        FCk_ObjectPooling_PoolState& InPool)
    -> UObject*
{
    const auto& Class = InPool.Get_Class();

    CK_ENSURE_IF_NOT(ck::IsValid(Class), TEXT("ObjectPool has an INVALID class — cannot spawn an instance"))
    { return {}; }

    auto* NewInstance = NewObject<UObject>(GetWorld(), Class, NAME_None, RF_NoFlags, InPool.Get_Archetype());

    ++InPool._NumLiveInstances;

    return NewInstance;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoReset_ToArchetype(
        UObject* InObject,
        const UObject* InArchetype)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Cannot reset an INVALID object to its archetype"))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InArchetype, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Cannot reset [{}] — its creation archetype is INVALID (pool pin should have prevented this)"),
        InObject)
    { return; }

    for (auto* Property = InObject->GetClass()->PropertyLink;
         ck::IsValid(Property);
         Property = Property->PropertyLinkNext)
    {
        if (const auto* StructProp = CastField<FStructProperty>(Property);
            StructProp != nullptr && StructProp->Struct == FCk_Handle_ObjectPoolingParticipant::StaticStruct())
        { continue; }

        Property->CopyCompleteValue_InContainer(InObject, InArchetype);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoSweep_NullSlots(
        FCk_ObjectPooling_PoolState& InPool)
    -> void
{
    const auto SweepArray = [&](TArray<TObjectPtr<UObject>>& InArray, const bool InRemoveFromReverseMap)
    {
        for (auto Index = InArray.Num() - 1; Index >= 0; --Index)
        {
            if (ck::IsValid(InArray[Index].Get()))
            { continue; }

            if (InRemoveFromReverseMap)
            { _InstanceToPool.Remove(FObjectKey{InArray[Index].Get()}); }

            InArray.RemoveAtSwap(Index, EAllowShrinking::No);
            --InPool._NumLiveInstances;
        }
    };

    constexpr auto FreeSlotsHaveNoReverseEntry = false;
    constexpr auto InUseSlotsHaveReverseEntry = true;
    SweepArray(InPool._FreeObjects, FreeSlotsHaveNoReverseEntry);
    SweepArray(InPool._InUseObjects, InUseSlotsHaveReverseEntry);
}

// --------------------------------------------------------------------------------------------------------------------
