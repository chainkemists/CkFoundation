#include "CkObjectPooling_Subsystem.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/ObjectPooling/CkObjectPoolingParticipant.h"
#include "CkCore/ObjectPooling/CkObjectPoolingParticipant_Utils.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Settings.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/LatentActionManager.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <TimerManager.h>
#include <UObject/UObjectGlobals.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <ClassGenerator/ASClass.h>
#include <angelscript.h>
#include <as_context.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddWeakLambda(this,
    [this]
    {
        _StaleSweepPending = true;
    });
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    Deinitialize()
    -> void
{
    FCoreUObjectDelegates::GetPostGarbageCollect().Remove(_PostGarbageCollectHandle);

    Super::Deinitialize();
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoesSupportWorldType(
        const EWorldType::Type InWorldType) const
    -> bool
{
    // the editor ECS world spawns EntityScripts/components through the pooled path too — without
    // this subsystem there, those instances would be caller-owned with only weak holders (no GC
    // root) and editor GC would collect them mid-preview
    return Super::DoesSupportWorldType(InWorldType) || InWorldType == EWorldType::Editor;
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    Super::Tick(InDeltaTime);

    if (_StaleSweepPending)
    {
        _StaleSweepPending = false;
        DoSweep_StaleAfterGC();
    }

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

            if (ck::Is_NOT_Valid(NewInstance))
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
    Get_NumPinnedUnique() const
    -> int32
{
    return _PinnedUnique.Num();
}

auto
    UCk_ObjectPooling_Subsystem_UE::
    Get_IsTrackedObject(
        const UObject* InObject) const
    -> bool
{
    if (ck::Is_NOT_Valid(InObject))
    { return false; }

    if (_PinnedUnique.Contains(const_cast<UObject*>(InObject)))
    { return true; }

    return _InstanceToPool.Contains(FObjectKey{InObject});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    AcquireFromPool(
        const TSubclassOf<UObject>& InClass,
        UObject* InArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams,
        UObject* InOuter)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass), TEXT("Cannot Acquire a pooled object with an INVALID class"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InClass->IsChildOf<AActor>(),
        TEXT("ObjectPooling hands out plain UObjects only — [{}] is an Actor class. "
             "Actor creation/pooling goes through SpawnActor, not Request_CreateNewObject"), InClass)
    { return {}; }

    if (ck::IsValid(InArchetype))
    {
        CK_ENSURE_IF_NOT(InArchetype->GetClass() == InClass.Get(),
            TEXT("Archetype [{}] is not an instance of class [{}] — pools are keyed by (class, archetype) "
                 "and recycled instances reset to the archetype, so the classes must match exactly"),
            InArchetype, InClass)
        { return {}; }
    }

    auto* Archetype = ck::IsValid(InArchetype)
        ? InArchetype
        : InClass->GetDefaultObject();

    if (InPoolParams.Get_RecyclePolicy() == ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease)
    {
        auto* EffectiveOuter = ck::IsValid(InOuter) ? InOuter : static_cast<UObject*>(GetWorld());
        auto* NewInstance = NewObject<UObject>(EffectiveOuter, InClass, NAME_None, RF_NoFlags, Archetype);

        _PinnedUnique.Emplace(NewInstance);

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

    if (ck::Is_NOT_Valid(AcquiredObject))
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

        if (ck::Is_NOT_Valid(AcquiredObject))
        { return {}; }

        // top up queued extras to (GrowBatchCount-1) via the prewarm tick (never synchronous)
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

    // a fresh instance already carries the archetype (NewObject template) — only a recycled one resets
    if (WasRecycled)
    {
        Request_ResetToArchetype(AcquiredObject, Pool->Get_Archetype());
    }

    UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_AcquiredFromPool_OnObject(AcquiredObject);

    return AcquiredObject;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    TryReleaseToPool(
        UObject* InObject)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(InObject != nullptr, TEXT("Cannot Release a NULL object to its ObjectPool"))
    { return ECk_SucceededFailed::Failed; }

    // already destroyed externally (destroy-then-release ordering) — reconcile tracking now while
    // the pointer still hashes to its entries, and stay benign: release is fire-and-forget
    if (ck::Is_NOT_Valid(InObject))
    {
        _PinnedUnique.Remove(InObject);

        if (const auto* PoolKey = _InstanceToPool.Find(FObjectKey{InObject}))
        {
            if (auto* Pool = _Pools.Find(*PoolKey))
            {
                if (Pool->_InUseObjects.Remove(InObject) > 0)
                { --Pool->_NumLiveInstances; }
            }

            _InstanceToPool.Remove(FObjectKey{InObject});
        }

        ck::core::Verbose(TEXT("TryReleaseToPool: [{}] was already destroyed — tracking reconciled, nothing to release"), InObject);
        return ECk_SucceededFailed::Failed;
    }

    if (_PinnedUnique.Remove(InObject) > 0)
    {
        DoQuiesce_ReleasedObject(InObject);
        UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_ReleasedToPool_OnObject(InObject);
        return ECk_SucceededFailed::Succeeded;
    }

    const auto* PoolKey = _InstanceToPool.Find(FObjectKey{InObject});

    // not tracked (CDO / snapshot-minted / non-pooled create) — benign no-op so teardown can call ungated
    if (PoolKey == nullptr)
    {
        ck::core::Verbose(TEXT("TryReleaseToPool: [{}] is not managed by the pooling subsystem — nothing to release"), InObject);
        return ECk_SucceededFailed::Failed;
    }

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

    DoQuiesce_ReleasedObject(InObject);
    UCk_Utils_ObjectPoolingParticipant_UE::Broadcast_ReleasedToPool_OnObject(InObject);

    Pool->_FreeObjects.Emplace(InObject);

    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoQuiesce_ReleasedObject(
        UObject* InObject)
    -> void
{
    // pre-pooling, an object whose entity died was GC'd and its pending world timers / latent
    // actions silently never fired. Pooling keeps the instance alive (pinned or parked), so a
    // lingering timer WOULD fire post-release against dead associations — loudly, in whatever
    // test/gameplay runs next. Release therefore quiesces exactly like death did (the same pair
    // UActorComponent::EndPlay clears). Only TRACKED objects get this — release on an untracked
    // object is a no-op and must not side-effect timers we don't own

    if (auto* Hooks = _ReleaseQuiesceHooks.Find(FObjectKey{InObject}))
    {
        // move out first — a hook that re-enters (an unbind can drop a delegate fragment and
        // cascade) must not mutate the array mid-iteration
        const auto LocalHooks = MoveTemp(*Hooks);
        _ReleaseQuiesceHooks.Remove(FObjectKey{InObject});

        for (const auto& Hook : LocalHooks)
        { Hook(); }
    }

    auto* World = GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return; }

    World->GetTimerManager().ClearAllTimersForObject(InObject);
    World->GetLatentActionManager().RemoveActionsForObject(InObject);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_Subsystem_UE::
    TryRegisterReleaseQuiesceHook(
        const UObject* InBoundObject,
        TFunction<void()> InHook)
    -> bool
{
    if (NOT Get_IsTrackedObject(InBoundObject))
    { return false; }

    _ReleaseQuiesceHooks.FindOrAdd(FObjectKey{InBoundObject}).Emplace(MoveTemp(InHook));
    return true;
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

    // a project-settings entry for the class overrides the acquire-site params (read once, at creation)
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
    Request_ResetToArchetype(
        UObject* InObject,
        const UObject* InArchetype)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Cannot reset an INVALID object to its archetype"))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InArchetype),
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

#if WITH_ANGELSCRIPT_CK
    // AngelScript members declared WITHOUT UPROPERTY() exist only as script-object properties
    // (byte offsets on the fused UObject) — the class generator creates FProperties solely for
    // UPROPERTY-exported members, so the reflected sweep above cannot see them. A fresh instance
    // gets their values from the class defaults function at construction; a recycled one must
    // copy them back from the archetype or it resumes the previous life's state (e.g. a consumed
    // one-shot cursor like UBb_DamageApplicator's NextResolution). The engine's script-registered
    // UObject::CopyScriptPropertiesFrom does exactly this copy; it must be invoked through an
    // execution context because the fork's asIScriptObject methods are non-virtual and not
    // DLL-exported (a direct operator= call fails to link outside AngelscriptCode)
    if (Cast<UASClass>(InObject->GetClass()) != nullptr)
    {
        static asIScriptFunction* CopyScriptPropertiesFunc = nullptr;

        if (CopyScriptPropertiesFunc == nullptr)
        {
            if (const auto* TypeInfo = FAngelscriptManager::Get().GetScriptEngine()->GetTypeInfoByName("UObject");
                TypeInfo != nullptr)
            { CopyScriptPropertiesFunc = TypeInfo->GetMethodByName("CopyScriptPropertiesFrom"); }
        }

        CK_ENSURE_IF_NOT(CopyScriptPropertiesFunc != nullptr,
            TEXT("Could not resolve UObject::CopyScriptPropertiesFrom from the script engine — "
                 "recycled [{}] keeps its previous life's script-only members"), InObject)
        { return; }

        auto Context = FAngelscriptContext{InObject};
        Context.PrepareExternal(CopyScriptPropertiesFunc);

        // dispatch through the asIScriptContext interface — its methods are virtual, so the call
        // resolves via vtable instead of importing asCContext symbols the module does not export
        asIScriptContext* ScriptContext = static_cast<asCContext*>(Context);
        ScriptContext->SetObject(InObject);
        ScriptContext->SetArgObject(0, const_cast<UObject*>(InArchetype));

        Context.ExecuteExternal();
    }
#endif

    // the sweep above copied instanced-subobject POINTERS from the archetype — re-instance them so
    // the recycled object owns fresh copies (a write through an aliased subobject would corrupt the
    // archetype/CDO). Mirrors FObjectInitializer::InstanceSubobjects: the graph must map
    // InArchetype -> InObject, so archetype-outered subobjects duplicate into the recycled object
    // (parameterless UObject::InstanceSubobjectTemplates roots the graph at GetArchetype()/self and
    // leaves the aliases untouched)
    if (InObject->GetClass()->HasAnyClassFlags(CLASS_HasInstancedReference))
    {
        auto InstancingGraph = FObjectInstancingGraph{};
        InstancingGraph.AddNewObject(InObject, const_cast<UObject*>(InArchetype));

        InObject->GetClass()->InstanceSubobjectTemplates(
            InObject, InArchetype, InArchetype->GetClass(), InObject, &InstancingGraph);

        // the instancing graph REUSES an existing same-named per-instance subobject instead of
        // re-creating it — its properties still hold the previous incarnation's values. Recurse the
        // reset into each (instance, template) pair so the whole reflected tree matches a fresh
        // create. (Instanced subobjects inside Set/Map containers are not walked — none exist in
        // the framework; add the container walk if one ever does)
        const auto ResetMatchedSubobject = [InObject](UObject* InSubobject, UObject* InTemplateSubobject) -> void
        {
            if (ck::Is_NOT_Valid(InSubobject) || ck::Is_NOT_Valid(InTemplateSubobject))
            { return; }

            if (InSubobject == InTemplateSubobject || InSubobject->GetClass() != InTemplateSubobject->GetClass())
            { return; }

            if (InSubobject->GetOuter() != InObject)
            { return; }

            Request_ResetToArchetype(InSubobject, InTemplateSubobject);
        };

        for (auto* Property = InObject->GetClass()->PropertyLink;
             ck::IsValid(Property);
             Property = Property->PropertyLinkNext)
        {
            if (const auto* ObjectProp = CastField<FObjectPropertyBase>(Property);
                ObjectProp != nullptr && ObjectProp->HasAnyPropertyFlags(CPF_PersistentInstance))
            {
                for (auto ArrayIndex = 0; ArrayIndex < ObjectProp->ArrayDim; ++ArrayIndex)
                {
                    ResetMatchedSubobject(
                        ObjectProp->GetObjectPropertyValue_InContainer(InObject, ArrayIndex),
                        ObjectProp->GetObjectPropertyValue_InContainer(InArchetype, ArrayIndex));
                }
                continue;
            }

            if (const auto* ArrayProp = CastField<FArrayProperty>(Property))
            {
                const auto* InnerObjectProp = CastField<FObjectPropertyBase>(ArrayProp->Inner);

                if (InnerObjectProp == nullptr || NOT InnerObjectProp->HasAnyPropertyFlags(CPF_PersistentInstance))
                { continue; }

                auto InstanceArray = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InObject)};
                auto TemplateArray = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(const_cast<UObject*>(InArchetype))};

                const auto NumSharedElements = FMath::Min(InstanceArray.Num(), TemplateArray.Num());

                for (auto Index = 0; Index < NumSharedElements; ++Index)
                {
                    ResetMatchedSubobject(
                        InnerObjectProp->GetObjectPropertyValue(InstanceArray.GetRawPtr(Index)),
                        InnerObjectProp->GetObjectPropertyValue(TemplateArray.GetRawPtr(Index)));
                }
            }
        }
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
            auto* Slot = InArray[Index].Get();

            if (ck::IsValid(Slot))
            { continue; }

            // once GC purges the slot to null the original key is unrecoverable here — the post-GC
            // sweep (DoSweep_StaleAfterGC) prunes those reverse-map entries by dead-key resolve
            if (InRemoveFromReverseMap && Slot != nullptr)
            { _InstanceToPool.Remove(FObjectKey{Slot}); }

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

auto
    UCk_ObjectPooling_Subsystem_UE::
    DoSweep_StaleAfterGC()
    -> void
{
    for (auto PoolIt = _Pools.CreateIterator(); PoolIt; ++PoolIt)
    {
        auto& Pool = PoolIt->Value;

        DoSweep_NullSlots(Pool);

        // zombie pool: its class or archetype died (BP recompile, editor-preview churn) — no future
        // acquire can ever match its key again. Only drop it once nothing is in use: dropping the
        // state would unpin in-use instances that holders still observe weakly
        if ((ck::Is_NOT_Valid(Pool._Class.Get()) || ck::Is_NOT_Valid(Pool._Archetype.Get()))
            && Pool._InUseObjects.IsEmpty())
        {
            ck::core::Verbose(TEXT("Dropping zombie ObjectPool (class [{}], archetype [{}] — key died)"),
                Pool._Class.Get(), Pool._Archetype.Get());
            PoolIt.RemoveCurrent();
        }
    }

    for (auto It = _InstanceToPool.CreateIterator(); It; ++It)
    {
        if (It->Key.ResolveObjectPtr() == nullptr)
        { It.RemoveCurrent(); }
    }

    for (auto It = _PinnedUnique.CreateIterator(); It; ++It)
    {
        if (ck::Is_NOT_Valid(It->Get()))
        { It.RemoveCurrent(); }
    }

    // hooks of a stolen instance never ran (no release) — the dead object's bindings compare
    // unequal to any live re-bind, which is exactly pre-pooling GC semantics, so just drop them
    for (auto It = _ReleaseQuiesceHooks.CreateIterator(); It; ++It)
    {
        if (It->Key.ResolveObjectPtr() == nullptr)
        { It.RemoveCurrent(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
