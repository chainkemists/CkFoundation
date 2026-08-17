#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkDynamic/CkDynamic_Fragment.h"
#include "CkDynamic/CkDynamic_FragmentDisplaySchema.h"
#include "CkDynamic/CkDynamic_FragmentSchema.h"
#include "CkDynamic/CkDynamic_Sentinel.h"

#include <UObject/UnrealType.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBindString.h>
#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>
#include <AngelscriptAnyStructParameter.h>
#include "CkDynamic/CkDynamic_AngelScript.h"
#include <ClassGenerator/ASStruct.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_utils
{
    // Registry-ctx cache of the FFragment_DynamicFragment_Data pools, so per-entity sweeps
    // (the save capture calls Get_AllFragments once per entity) touch only the dynamic pools instead of
    // walking every pool in the registry. Pools are only ever appended, so an unchanged pool count proves
    // the list is current; living in the registry's ctx, the cache dies with its world.
    struct FCtx_DynamicFragmentPools
    {
        int64                                                      _PoolCountWhenBuilt = -1;
        TArray<entt::storage<ck::FFragment_DynamicFragment_Data>*> _Pools;
    };

    auto PassesDestroyFilter(const FCk_Handle& InHandle, ECk_DestroyFilter InFilter) -> bool
    {
        switch (InFilter)
        {
            case ECk_DestroyFilter::None:
            {
                return true;
            }
            case ECk_DestroyFilter::IgnorePendingKill:
            {
                return NOT (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
                            UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Teardown) ||
                            UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Destroyed));
            }
            case ECk_DestroyFilter::Teardown:
            {
                return UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Teardown);
            }
            default:
            {
                CK_INVALID_ENUM(InFilter);
                return false;
            }
        }
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Add_Fragment(
        FCk_Handle& InHandle,
        const FInstancedStruct& InFragmentData,
        ECk_Replication InReplication)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid, TEXT("Invalid Handle passed. Unable to add Fragment"))
    { return {}; }

    const auto FragmentDataIsValid = ck::IsValid(InFragmentData);
    CK_ENSURE_IF_NOT(FragmentDataIsValid, TEXT("Invalid struct data in FInstancedStruct"))
    { return {}; }

    const auto Schema = ck::dynamic::Validate_FragmentSchema(InFragmentData.GetScriptStruct());
    CK_ENSURE_IF_NOT(Schema.IsSafe,
        TEXT("Dynamic Fragment schema [{}] is unsafe at [{}]: {}. Strong UObject references in named EnTT storage "
             "are not GC-traced; use weak/soft references or a stable CK handle."),
        InFragmentData.GetScriptStruct(), Schema.FailurePath, Schema.FailureReason)
    {}
    if (NOT Schema.IsSafe)
    { return {}; }

    if (InReplication == ECk_Replication::Replicates && NOT CanSetupReplication(InHandle, InFragmentData))
    { return {}; }

    auto Fragment = ck::FFragment_DynamicFragment_Data{InFragmentData};
    const auto StorageId = Get_StorageId(InFragmentData.GetScriptStruct());
    auto&& Storage = InHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    const auto FragmentDoesNotExist = NOT Storage.contains(Entity);
    CK_ENSURE_IF_NOT(FragmentDoesNotExist,
        TEXT("Fragment [{}] already exists in Entity [{}]. ")
        TEXT("Call AddOrGet_Fragment if you want replace-on-duplicate semantics, ")
        TEXT("or compose this feature on a child entity instead."),
        InFragmentData.GetScriptStruct(), InHandle)
    { return {}; }

    Storage.emplace(Entity, MoveTemp(Fragment));
    InHandle.Get_RegistryView().BumpDirtyMarkerVersion(Get_DirtyMarkerHash(InFragmentData.GetScriptStruct()));

    // Presence marker only (Has<>/removal) — the real data lives in the named storage.
    if (NOT InHandle.Has<ck::FFragment_DynamicFragment_Data>())
    {
        InHandle.Add<ck::FFragment_DynamicFragment_Data>(ck::FFragment_DynamicFragment_Data{});
    }

    if (InReplication == ECk_Replication::Replicates)
    {
        DoSetupReplication(InHandle, InFragmentData);
    }

    return InHandle;
}

auto
    UCk_Utils_DynamicFragment_UE::
    AddOrGet_Fragment_TypeUnsafe(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        ECk_Replication InReplication)
    -> FInstancedStruct&
{
    if (auto* Fragment = TryAddOrGet_Fragment_TypeUnsafe(InHandle, InStructType, InReplication))
    { return *Fragment; }

    return ck::dynamic::Get_InvalidSentinel_FragmentData(InStructType);
}

auto
    UCk_Utils_DynamicFragment_UE::
    TryAddOrGet_Fragment_TypeUnsafe(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        ECk_Replication InReplication)
    -> FInstancedStruct*
{
    const auto TypeIsValid = ck::IsValid(InStructType);
    CK_ENSURE_IF_NOT(TypeIsValid, TEXT("Invalid Dynamic Fragment type passed to AddOrGet"))
    { return nullptr; }

    const auto Schema = ck::dynamic::Validate_FragmentSchema(InStructType);
    CK_ENSURE_IF_NOT(Schema.IsSafe,
        TEXT("Dynamic Fragment schema [{}] is unsafe at [{}]: {}. Strong UObject references in named EnTT storage "
             "are not GC-traced; use weak/soft references or a stable CK handle."),
        InStructType, Schema.FailurePath, Schema.FailureReason)
    {}
    if (NOT Schema.IsSafe)
    { return nullptr; }

    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid, TEXT("Invalid Handle passed to AddOrGet Dynamic Fragment [{}]"), InStructType)
    { return nullptr; }

    if (NOT Has_Fragment(InHandle, InStructType))
    {
        Add_Fragment(InHandle, FInstancedStruct(InStructType), InReplication);
        const auto AddSucceeded = Has_Fragment(InHandle, InStructType);
        CK_ENSURE_IF_NOT(AddSucceeded,
            TEXT("AddOrGet Dynamic Fragment [{}] failed to create storage on [{}]"), InStructType, InHandle)
        { return nullptr; }
    }
    else
    {
        // Callers mutate the returned struct in place, so mirror FCk_Registry::AddOrGet: bump anyway, or the
        // scheduler's pump short-circuit misses a change that never moved membership.
        InHandle.Get_RegistryView().BumpDirtyMarkerVersion(Get_DirtyMarkerHash(InStructType));
    }

    return TryGet_Fragment_TypeUnsafe(InHandle, InStructType);
}

auto
    UCk_Utils_DynamicFragment_UE::
    Request_Remove(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto Result = Request_TryRemove(InHandle, InStructType, InDelegate);

    CK_ENSURE(Result == ECk_SucceededFailed::Succeeded,
        TEXT("Could NOT remove Dynamic Fragment [{}] from Handle [{}]"), InStructType, InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Request_TryRemove(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> ECk_SucceededFailed
{
    const auto StructTypeIsValid = ck::IsValid(InStructType);
    CK_ENSURE_IF_NOT(StructTypeIsValid,
        TEXT("Invalid struct type passed. Unable to remove Dynamic Fragment from Handle [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return ECk_SucceededFailed::Failed;
    }

    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid Handle [{}] passed. Unable to remove Dynamic Fragment [{}]"), InHandle, InStructType)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return ECk_SucceededFailed::Failed;
    }

    const auto StorageId = Get_StorageId(InStructType);
    auto&& Storage = InHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed);
        return ECk_SucceededFailed::Failed;
    }

    Storage.remove(Entity);
    InHandle.Get_RegistryView().BumpDirtyMarkerVersion(Get_DirtyMarkerHash(InStructType));

    const auto HasOtherFragments = ck::algo::AnyOf(InHandle.Get_RegistryView().Storage(), [Entity](const auto& Pair)
    {
        const auto& Pool = Pair.second;
        return Pool.info() == entt::type_id<ck::FFragment_DynamicFragment_Data>() && Pool.contains(Entity);
    });

    if (NOT HasOtherFragments)
    {
        InHandle.Try_Remove<ck::FFragment_DynamicFragment_Data>();
    }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Get_Fragment_TypeUnsafe(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FInstancedStruct&
{
    if (auto* Fragment = TryGet_Fragment_TypeUnsafe(InHandle, InStructType))
    { return *Fragment; }

    return ck::dynamic::Get_InvalidSentinel_FragmentData(InStructType);
}

auto
    UCk_Utils_DynamicFragment_UE::
    TryGet_Fragment_TypeUnsafe(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FInstancedStruct*
{

    const auto TypeIsValid = ck::IsValid(InStructType);
    CK_ENSURE_IF_NOT(TypeIsValid,
        TEXT("Invalid Dynamic Fragment type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return nullptr; }

    const auto Schema = ck::dynamic::Validate_FragmentSchema(InStructType);
    CK_ENSURE_IF_NOT(Schema.IsSafe,
        TEXT("Refusing to expose unsafe Dynamic Fragment schema [{}] at [{}]: {}"),
        InStructType, Schema.FailurePath, Schema.FailureReason)
    {}
    if (NOT Schema.IsSafe)
    { return nullptr; }

    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return nullptr; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    const auto FragmentExists = Storage.contains(Entity);
    CK_ENSURE_IF_NOT(FragmentExists,
        TEXT("Entity [{}] does NOT have the Dynamic Fragment [{}]! Cannot retrieve it"), InHandle, InStructType)
    { return nullptr; }

    auto& Fragment = Storage.get(Entity);
    return &Fragment.Get_StructData();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Has_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment type passed. Unable to query Dynamic Fragment from [{}]"), InHandle)
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to query Dynamic Fragment [{}]"), InHandle, InStructType)
    { return false; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    return Storage.contains(Entity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithOneFragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructType},
        InFilter,
        [&](FCk_Handle& Handle, const auto& /*Storages*/, entt::entity /*Entity*/)
        {
            InDelegate.Execute(Handle);
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithTwoFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB},
        InFilter,
        [&](FCk_Handle& Handle, const auto& /*Storages*/, entt::entity /*Entity*/)
        {
            InDelegate.Execute(Handle);
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithThreeFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB, InStructTypeC},
        InFilter,
        [&](FCk_Handle& Handle, const auto& /*Storages*/, entt::entity /*Entity*/)
        {
            InDelegate.Execute(Handle);
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithFourFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const UScriptStruct* InStructTypeD,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB, InStructTypeC, InStructTypeD},
        InFilter,
        [&](FCk_Handle& Handle, const auto& /*Storages*/, entt::entity /*Entity*/)
        {
            InDelegate.Execute(Handle);
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithFiveFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const UScriptStruct* InStructTypeD,
        const UScriptStruct* InStructTypeE,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB, InStructTypeC, InStructTypeD, InStructTypeE},
        InFilter,
        [&](FCk_Handle& Handle, const auto& /*Storages*/, entt::entity /*Entity*/)
        {
            InDelegate.Execute(Handle);
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Get_IsSnapshotTransient(
        const UScriptStruct* InStructType)
    -> bool
{
    if (ck::Is_NOT_Valid(InStructType))
    { return false; }

    const auto DoCompute = [&]() -> bool
    {
        const auto* Marker = FCk_DynamicFragment_SnapshotTransient::StaticStruct();
        if (InStructType->IsChildOf(Marker))
        { return true; }

        for (TFieldIterator<FStructProperty> It{InStructType}; It; ++It)
        {
            if (It->Struct == Marker)
            { return true; }
        }

        return false;
    };

    // Weak key so an AS hot reload, which can replace a UScriptStruct at a recycled address, re-computes —
    // matching Validate_FragmentSchema / Get_StorageId. Off the game thread the cache is skipped, not raced.
    if (NOT IsInGameThread())
    { return DoCompute(); }

    static TMap<TWeakObjectPtr<const UScriptStruct>, bool> Cache;
    if (const auto* Found = Cache.Find(InStructType))
    { return *Found; }

    const auto Result = DoCompute();
    Cache.Add(InStructType, Result);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Get_StorageId(
        const UScriptStruct* InStructType)
    -> entt::id_type
{
    const auto TypeIsValid = ck::IsValid(InStructType);
    CK_ENSURE_IF_NOT(TypeIsValid, TEXT("Invalid struct type"))
    { return entt::id_type{}; }

    // Keyed by weak pointer so an AS live-reload — which replaces UScriptStruct objects, potentially at a
    // recycled address — re-computes instead of serving a stale id. Game-thread-only, matching every caller.
    const auto IsGameThreadLookup = IsInGameThread();
    CK_ENSURE_IF_NOT(IsGameThreadLookup, TEXT("Dynamic Fragment storage ids must be resolved on the game thread"))
    { return entt::id_type{}; }

    static TMap<TWeakObjectPtr<const UScriptStruct>, entt::id_type> Cache;

    if (const auto* Found = Cache.Find(InStructType))
    { return *Found; }

    // This is the first common observation point for every dynamic-fragment producer. The display registry keeps
    // only the stable path and owned labels; it never retains this UScriptStruct across an AngelScript hot reload.
    const auto DisplaySchemaObserved = ck::dynamic::Observe_FragmentDisplaySchema(InStructType);
    CK_ENSURE_IF_NOT(DisplaySchemaObserved,
        TEXT("Unable to observe Dynamic Fragment display schema for [{}]"), InStructType)
    {}

    const auto Id = entt::id_type{GetTypeHash(InStructType->GetPathName())};
    Cache.Add(InStructType, Id);
    return Id;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Get_DirtyMarkerHash(
        const UScriptStruct* InStructType)
    -> uint32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type"))
    { return uint32{}; }

    return GetTypeHash(FName{*InStructType->GetPathName()});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Has_AnyEntityWith_Fragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType)
    -> bool
{
    if (ck::Is_NOT_Valid(InAnyHandle) || ck::Is_NOT_Valid(InStructType))
    { return false; }

    const auto StorageId = Get_StorageId(InStructType);
    auto MutableHandle = InAnyHandle;
    auto& Storage = MutableHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    return NOT Storage.empty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Has_AnyLiveEntityWith_Fragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType)
    -> bool
{
    if (ck::Is_NOT_Valid(InAnyHandle) || ck::Is_NOT_Valid(InStructType))
    { return false; }

    const auto StorageId = Get_StorageId(InStructType);
    auto MutableHandle = InAnyHandle;
    auto& Storage = MutableHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    // A single-storage view skips in_place tombstones, so begin() == end() means zero LIVE entities.
    const auto View = entt::basic_view{Storage};
    return View.begin() != View.end();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Get_AllFragments(
        const FCk_Handle& InHandle)
    -> TArray<FInstancedStruct>
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_DynamicFragment_Data>())
    { return {}; }

    auto Result = TArray<FInstancedStruct>{};
    auto MutableHandle = InHandle;
    const auto Entity = InHandle.Get_Entity().Get_ID();
    auto RegistryView = MutableHandle.Get_RegistryView();

    auto PoolsIterable = RegistryView.Storage();
    const auto PoolCount = static_cast<int64>(PoolsIterable.end() - PoolsIterable.begin());

    auto& Cache = RegistryView.Ctx().emplace<ck_dynamic_utils::FCtx_DynamicFragmentPools>();
    if (Cache._PoolCountWhenBuilt != PoolCount)
    {
        Cache._PoolCountWhenBuilt = PoolCount;
        Cache._Pools.Reset();
        for (auto&& [StorageId, Pool] : PoolsIterable)
        {
            if (Pool.info() != entt::type_id<ck::FFragment_DynamicFragment_Data>())
            { continue; }

            Cache._Pools.Add(&RegistryView.Storage<ck::FFragment_DynamicFragment_Data>(StorageId));
        }
    }

    for (auto* Pool : Cache._Pools)
    {
        if (NOT Pool->contains(Entity))
        { continue; }

        const auto& Fragment = Pool->get(Entity);

        if (const auto& StructData = Fragment.Get_StructData();
            ck::IsValid(StructData))
        {
            Result.Add(StructData);
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    CanSetupReplication(
        const FCk_Handle& InHandle,
        const FInstancedStruct& InStructData)
    -> bool
{
    const auto* StructType = InStructData.GetScriptStruct();

    // Runs before Add_Fragment's first mutation: a rejected request must leave no storage behind.
    const auto HasReplicableData = static_cast<bool>(TFieldIterator<FProperty>{StructType});
    CK_ENSURE_IF_NOT(HasReplicableData,
        TEXT("Dynamic Fragment [{}] on Entity [{}] was requested to replicate, but the struct has no ")
        TEXT("properties (tag / size-0). Replicating it carries no data and is meaningless. Add it without ")
        TEXT("ECk_Replication::Replicates, or give the struct a replicated property."),
        StructType, InHandle)
    { return false; }

    return HasReplicableData;
}

auto
    UCk_Utils_DynamicFragment_UE::
    DoSetupReplication(
        FCk_Handle& InHandle,
        const FInstancedStruct& InStructData)
    -> void
{
    const auto* StructType = InStructData.GetScriptStruct();

    InHandle.AddOrGet<ck::FFragment_DynamicFragment_ReplicatedTypes>().Get_Types().Add(StructType);
    InHandle.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>();
}

auto
    UCk_Utils_DynamicFragment_UE::
    Request_MarkReplicationDirty(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid Handle passed to Request_MarkReplicationDirty"))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    const auto IsReplicated = InHandle.Has<ck::FFragment_DynamicFragment_ReplicatedTypes>() &&
        InHandle.Get<ck::FFragment_DynamicFragment_ReplicatedTypes>().Get_Types().Contains(InStructType);

    CK_ENSURE_IF_NOT(IsReplicated,
        TEXT("Request_MarkReplicationDirty: Dynamic Fragment [{}] on Entity [{}] is NOT set up to replicate. ")
        TEXT("Add it with ECk_Replication::Replicates before marking it dirty."),
        InStructType, InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    InHandle.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);
}

auto
    UCk_Utils_DynamicFragment_UE::
    BindTo_OnRepNotify(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_OnRepNotify& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment type passed to BindTo_OnRepNotify on Handle [{}]"), InHandle)
    { return; }

    CK_SIGNAL_BIND_WITH_CONDITION(ck::UUtils_Signal_DynamicFragment_OnRepNotify, InHandle, InDelegate,
        InBindingPolicy, InPostFireBehavior,
        [InStructType](FCk_Handle /*InHandle*/, FCk_DynamicFragment_RepNotifyInfo InInfo)
        {
            return InInfo.ChangedType.Get() == InStructType;
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    UnbindFrom_OnRepNotify(
        FCk_Handle& InHandle,
        const FCk_DynamicFragment_OnRepNotify& InDelegate)
    -> void
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_DynamicFragment_OnRepNotify, InHandle, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK
auto
    UCk_Utils_DynamicFragment_UE::
    Add_Fragment(
        FCk_Handle& InHandle,
        const FAngelscriptAnyStructParameter& InStructData,
        ECk_Replication InReplication)
    -> FCk_Handle
{
    return Add_Fragment(InHandle, InStructData.InstancedStruct, InReplication);
}

auto
    UCk_Utils_DynamicFragment_UE::
    AddOrGet_Fragment(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        ECk_Replication InReplication)
    -> FScriptStructWildcard&
{
    if (auto* Fragment = TryAddOrGet_Fragment_TypeUnsafe(InHandle, InStructType, InReplication))
    { return *(FScriptStructWildcard*)Fragment->GetMutableMemory(); }

    FAngelscriptManager::Throw("Dynamic Fragment AddOrGet rejected invalid input");
    static auto InvalidNoType = FScriptStructWildcard{};
    return ck::IsValid(InStructType)
        ? *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InStructType).GetMutableMemory()
        : InvalidNoType;
}

auto
    UCk_Utils_DynamicFragment_UE::
    Get_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FScriptStructWildcard&
{
    if (auto* Fragment = TryGet_Fragment_TypeUnsafe(InHandle, InStructType))
    { return *(FScriptStructWildcard*)Fragment->GetMutableMemory(); }

    FAngelscriptManager::Throw("Dynamic Fragment Get rejected invalid input");
    static auto InvalidNoType = FScriptStructWildcard{};
    return ck::IsValid(InStructType)
        ? *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InStructType).GetMutableMemory()
        : InvalidNoType;
}

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkDynamicFragment(static_cast<int32>(FAngelscriptBinds::EOrder::Late), []
{
    auto ExistingClass = FAngelscriptBinds::ExistingClass("FCk_Handle");

    ExistingClass.Method(
        "FCk_Handle Add_Fragment(const FInstancedStruct& InFragmentData, ECk_Replication InReplication = ECk_Replication::DoesNotReplicate)",
        [](FCk_Handle& Self, const FInstancedStruct& InFragmentData, ECk_Replication InReplication) -> FCk_Handle
        {
            const auto Result = UCk_Utils_DynamicFragment_UE::Add_Fragment(Self, InFragmentData, InReplication);
            if (ck::Is_NOT_Valid(Result))
            { FAngelscriptManager::Throw("Dynamic Fragment Add rejected invalid input"); }
            return Result;
        });

    ExistingClass.Method(
        "FCk_Handle Add_Fragment(const FAngelscriptAnyStructParameter& InFragmentData, ECk_Replication InReplication = ECk_Replication::DoesNotReplicate)",
        [](FCk_Handle& Self, const FAngelscriptAnyStructParameter& InFragmentData, ECk_Replication InReplication) -> FCk_Handle
        {
            const auto Result = UCk_Utils_DynamicFragment_UE::Add_Fragment(Self, InFragmentData, InReplication);
            if (ck::Is_NOT_Valid(Result))
            { FAngelscriptManager::Throw("Dynamic Fragment Add rejected invalid input"); }
            return Result;
        });

    ExistingClass.Method(
        "FScriptStructWildcard& AddOrGet_Fragment(const UScriptStruct InStructType, ECk_Replication InReplication = ECk_Replication::DoesNotReplicate)",
        [](FCk_Handle& Self, const UScriptStruct* InStructType, ECk_Replication InReplication) -> FScriptStructWildcard&
        {
            return UCk_Utils_DynamicFragment_UE::AddOrGet_Fragment(Self, InStructType, InReplication);
        });
    FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(0);

    ExistingClass.Method(
        "FScriptStructWildcard& Get_Fragment(const UScriptStruct InStructType) const",
        [](const FCk_Handle& Self, const UScriptStruct* InStructType) -> FScriptStructWildcard&
        {
            return UCk_Utils_DynamicFragment_UE::Get_Fragment(Self, InStructType);
        });
    FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(0);
});

#endif

// --------------------------------------------------------------------------------------------------------------------

template<size_t N, typename T_Callback>
auto 
    UCk_Utils_DynamicFragment_UE::
    ForEachEntity_WithDynamicFragments(
        const FCk_Handle& InAnyHandle,
        const std::array<const UScriptStruct*, N>& InStructTypes,
        ECk_DestroyFilter InFilter,
        T_Callback&& InCallback)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandle),
        TEXT("Invalid Handle passed. Unable to iterate over Entities with Dynamic Fragments"))
    { return; }

    const auto InvalidIt = std::find_if(InStructTypes.begin(), InStructTypes.end(), [](const UScriptStruct* Type)
    {
        return ck::Is_NOT_Valid(Type);
    });

    const auto InvalidIndex = (InvalidIt != InStructTypes.end())
        ? static_cast<int32>(std::distance(InStructTypes.begin(), InvalidIt))
        : INDEX_NONE;

    CK_ENSURE_IF_NOT(InvalidIndex == INDEX_NONE,
        TEXT("Invalid Dynamic Fragment type passed at index [{}]"), InvalidIndex)
    { return; }

    auto MutableAnyHandle = InAnyHandle;

    auto Storages = std::array<entt::storage<ck::FFragment_DynamicFragment_Data>*, N>{};
    std::transform(InStructTypes.begin(), InStructTypes.end(), Storages.begin(),
    [&MutableAnyHandle](const UScriptStruct* StructType)
    {
        const auto StorageId = Get_StorageId(StructType);
        return &MutableAnyHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    });

    for (const auto Entity : static_cast<const entt::sparse_set&>(*Storages[0]))
    {
        if (NOT InAnyHandle.Get_RegistryView().IsValid(Entity))
        { continue; }

        auto HandleWithFragments = InAnyHandle.Get_ValidHandle(Entity);

        if (NOT ck_dynamic_utils::PassesDestroyFilter(HandleWithFragments, InFilter))
        { continue; }

        const auto ExistsInAllStorages = ck::algo::AllOf(
            Storages.begin() + 1,
            Storages.end(),
            [Entity](const auto* Storage) { return Storage->contains(Entity); });

        if (NOT ExistsInAllStorages)
        { continue; }

        InCallback(HandleWithFragments, Storages, Entity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
