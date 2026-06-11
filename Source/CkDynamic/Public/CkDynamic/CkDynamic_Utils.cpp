#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkDynamic/CkDynamic_Fragment.h"

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

    auto Get_InvalidSentinel_FragmentData(const UScriptStruct* InStructType) -> FInstancedStruct&
    {
        static TMap<const UScriptStruct*, FInstancedStruct> Sentinels;
        auto& Instance = Sentinels.FindOrAdd(InStructType);
        if (NOT Instance.IsValid() || Instance.GetScriptStruct() != InStructType)
        {
            Instance.InitializeAs(InStructType);
        }
        return Instance;
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
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to add Fragment"))
    { return InHandle; }

    CK_ENSURE_IF_NOT(ck::IsValid(InFragmentData), TEXT("Invalid struct data in FInstancedStruct"))
    { return InHandle; }

    auto Fragment = ck::FFragment_DynamicFragment_Data{InFragmentData};
    const auto StorageId = Get_StorageId(InFragmentData.GetScriptStruct());
    auto&& Storage = InHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    CK_ENSURE_IF_NOT(NOT Storage.contains(Entity),
        TEXT("Fragment [{}] already exists in Entity [{}]. ")
        TEXT("Call AddOrGet_Fragment if you want replace-on-duplicate semantics, ")
        TEXT("or compose this feature on a child entity instead."),
        InFragmentData.GetScriptStruct(), InHandle)
    { return InHandle; }

    Storage.emplace(Entity, MoveTemp(Fragment));
    InHandle.Get_RegistryView().BumpDirtyMarkerVersion(Get_DirtyMarkerHash(InFragmentData.GetScriptStruct()));

    // The default-storage fragment is only a presence marker (Has<>/removal) —
    // the real data lives in the named storage. Keep it default-constructed.
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
    if (NOT Has_Fragment(InHandle, InStructType))
    {
        Add_Fragment(InHandle, FInstancedStruct(InStructType), InReplication);
    }
    else
    {
        // Callers of AddOrGet intend to mutate the returned struct in place. Mirror
        // FCk_Registry::AddOrGet's contract: bump so the scheduler's pump short-circuit sees the
        // change even though membership didn't move.
        InHandle.Get_RegistryView().BumpDirtyMarkerVersion(Get_DirtyMarkerHash(InStructType));
    }

    return Get_Fragment_TypeUnsafe(InHandle, InStructType);
}

auto
    UCk_Utils_DynamicFragment_UE::
    Request_Remove(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> void
{
    const auto Result = Request_TryRemove(InHandle, InStructType);

    CK_ENSURE(Result == ECk_SucceededFailed::Succeeded,
        TEXT("Could NOT remove Dynamic Fragment [{}] from Handle [{}]"), InStructType, InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Request_TryRemove(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid struct type passed. Unable to remove Dynamic Fragment from Handle [{}]"), InHandle)
    { return ECk_SucceededFailed::Failed; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to remove Dynamic Fragment [{}]"), InHandle, InStructType)
    { return ECk_SucceededFailed::Failed; }

    const auto StorageId = Get_StorageId(InStructType);
    auto&& Storage = InHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    {
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
    static auto InvalidNoType = FInstancedStruct{};

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return InvalidNoType; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return ck_dynamic_utils::Get_InvalidSentinel_FragmentData(InStructType); }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    CK_ENSURE_IF_NOT(Storage.contains(Entity),
        TEXT("Entity [{}] does NOT have the Dynamic Fragment [{}]! Cannot retrieve it"), InHandle, InStructType)
    { return ck_dynamic_utils::Get_InvalidSentinel_FragmentData(InStructType); }

    auto& Fragment = Storage.get(Entity);
    return Fragment.Get_StructData();
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
    Get_StorageId(
        const UScriptStruct* InStructType)
    -> entt::id_type
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type"))
    { return entt::id_type{}; }

    // Hot path: called per-fragment-access from script and once per declared storage per tick from the
    // script-processor join. The legacy computation allocated an FString (GetPathName) on every call.
    // Keyed by weak pointer so an AS live-reload (which replaces UScriptStruct objects, potentially at a
    // recycled address) re-computes instead of serving a stale id. Game-thread-only, matching every caller.
    check(IsInGameThread());

    static TMap<TWeakObjectPtr<const UScriptStruct>, entt::id_type> Cache;

    if (const auto* Found = Cache.Find(InStructType))
    { return *Found; }

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
    Get_AllFragments(
        const FCk_Handle& InHandle)
    -> TArray<FInstancedStruct>
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_DynamicFragment_Data>())
    { return {}; }

    auto Result = TArray<FInstancedStruct>{};
    auto MutableHandle = InHandle;
    const auto Entity = InHandle.Get_Entity().Get_ID();

    for (auto&& [StorageId, Pool] : MutableHandle.Get_RegistryView().Storage())
    {
        if (Pool.info() != entt::type_id<ck::FFragment_DynamicFragment_Data>())
        { continue; }

        if (NOT Pool.contains(Entity))
        { continue; }

        auto& TypedStorage = MutableHandle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
        const auto& Fragment = TypedStorage.get(Entity);

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
    DoSetupReplication(
        FCk_Handle& InHandle,
        const FInstancedStruct& InStructData)
    -> void
{
    const auto* StructType = InStructData.GetScriptStruct();

    // A struct with no reflected properties (tag / size-0) carries no replicable data. The iterator's
    // operator bool is true iff there is at least one FProperty — no loop body, so no C4702.
    const auto HasReplicableData = static_cast<bool>(TFieldIterator<FProperty>{StructType});

    CK_ENSURE_IF_NOT(HasReplicableData,
        TEXT("Dynamic Fragment [{}] on Entity [{}] was requested to replicate, but the struct has no ")
        TEXT("properties (tag / size-0). Replicating it carries no data and is meaningless. Add it without ")
        TEXT("ECk_Replication::Replicates, or give the struct a replicated property."),
        StructType, InHandle)
    { return; }

    InHandle.AddOrGet<ck::FFragment_DynamicFragment_ReplicatedTypes>().Get_Types().Add(StructType);
    InHandle.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>();
}

auto
    UCk_Utils_DynamicFragment_UE::
    Request_MarkReplicationDirty(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle passed to Request_MarkReplicationDirty"))
    { return; }

    const auto IsReplicated = InHandle.Has<ck::FFragment_DynamicFragment_ReplicatedTypes>() &&
        InHandle.Get<ck::FFragment_DynamicFragment_ReplicatedTypes>().Get_Types().Contains(InStructType);

    CK_ENSURE_IF_NOT(IsReplicated,
        TEXT("Request_MarkReplicationDirty: Dynamic Fragment [{}] on Entity [{}] is NOT set up to replicate. ")
        TEXT("Add it with ECk_Replication::Replicates before marking it dirty."),
        InStructType, InHandle)
    { return; }

    InHandle.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>();
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
    if (NOT Has_Fragment(InHandle, InStructType))
    {
        Add_Fragment(InHandle, FInstancedStruct{InStructType}, InReplication);
    }

    return Get_Fragment(InHandle, InStructType);
}

auto
    UCk_Utils_DynamicFragment_UE::
    Get_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FScriptStructWildcard&
{
    static auto InvalidNoType = FScriptStructWildcard{};

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return InvalidNoType; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return *(FScriptStructWildcard*)ck_dynamic_utils::Get_InvalidSentinel_FragmentData(InStructType).GetMutableMemory(); }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle.Get_RegistryView().Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    CK_ENSURE_IF_NOT(Storage.contains(Entity),
        TEXT("Entity [{}] does NOT have the Dynamic Fragment [{}]! Cannot retrieve it"), InHandle, InStructType)
    { return *(FScriptStructWildcard*)ck_dynamic_utils::Get_InvalidSentinel_FragmentData(InStructType).GetMutableMemory(); }

    auto& Fragment = Storage.get(Entity);
    return *(FScriptStructWildcard*)Fragment.Get_StructData().GetMemory();
}

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkDynamicFragment(static_cast<int32>(FAngelscriptBinds::EOrder::Late), []
{
    auto ExistingClass = FAngelscriptBinds::ExistingClass("FCk_Handle");

    ExistingClass.Method(
        "FCk_Handle Add_Fragment(const FInstancedStruct& InFragmentData, ECk_Replication InReplication = ECk_Replication::DoesNotReplicate)",
        [](FCk_Handle& Self, const FInstancedStruct& InFragmentData, ECk_Replication InReplication) -> FCk_Handle
        {
            return UCk_Utils_DynamicFragment_UE::Add_Fragment(Self, InFragmentData, InReplication);
        });

    ExistingClass.Method(
        "FCk_Handle Add_Fragment(const FAngelscriptAnyStructParameter& InFragmentData, ECk_Replication InReplication = ECk_Replication::DoesNotReplicate)",
        [](FCk_Handle& Self, const FAngelscriptAnyStructParameter& InFragmentData, ECk_Replication InReplication) -> FCk_Handle
        {
            return UCk_Utils_DynamicFragment_UE::Add_Fragment(Self, InFragmentData, InReplication);
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
