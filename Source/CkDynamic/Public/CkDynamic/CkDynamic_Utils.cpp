#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkDynamic/CkDynamic_Fragment.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBindString.h>
#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>
#include <AngelscriptAnyStructParameter.h>
#include "CkDynamic/CkDynamic_AngelScript.h"
#include <ClassGenerator/ASStruct.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace
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
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Add_Fragment(
        FCk_Handle& InHandle,
        const FInstancedStruct& InFragmentData)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to add Fragment"))
    { return InHandle; }

    CK_ENSURE_IF_NOT(ck::IsValid(InFragmentData), TEXT("Invalid struct data in FInstancedStruct"))
    { return InHandle; }

    auto Fragment = ck::FFragment_DynamicFragment_Data{InFragmentData};
    const auto StorageId = Get_StorageId(InFragmentData.GetScriptStruct());
    auto&& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    if (Storage.contains(Entity))
    {
        Storage.remove(Entity);
    }

    Storage.emplace(Entity, std::move(Fragment));

    if (NOT InHandle.Has<ck::FFragment_DynamicFragment_Data>())
    {
        InHandle.Add<ck::FFragment_DynamicFragment_Data>(Fragment);
    }

    return InHandle;
}

auto
    UCk_Utils_DynamicFragment_UE::
    AddOrGet_Fragment_TypeUnsafe(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FInstancedStruct&
{
    if (NOT Has_Fragment(InHandle, InStructType))
    {
        Add_Fragment(InHandle, FInstancedStruct(InStructType));
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
    auto&& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    {
        return ECk_SucceededFailed::Failed;
    }

    Storage.remove(Entity);

    const auto HasOtherFragments = ck::algo::AnyOf(InHandle->Storage(), [Entity](const auto& Pair)
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
    static auto Invalid = FInstancedStruct{};

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return Invalid; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return Invalid; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    CK_ENSURE_IF_NOT(Storage.contains(Entity),
        TEXT("Entity [{}] does NOT have the Dynamic Fragment [{}]! Cannot retrieve it"), InHandle, InStructType)
    { return Invalid; }

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
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    return Storage.contains(Entity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithOneFragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity_OneFragment& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructType},
        InFilter,
        [&](const FCk_Handle& Handle, const auto& Storages, entt::entity Entity)
        {
            InDelegate.Execute(Handle, Storages[0]->get(Entity).Get_StructData());
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithTwoFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const FCk_DynamicFragment_ForEachEntity_TwoFragments& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB},
        InFilter,
        [&](const FCk_Handle& Handle, const auto& Storages, entt::entity Entity)
        {
            InDelegate.Execute(
                Handle,
                Storages[0]->get(Entity).Get_StructData(),
                Storages[1]->get(Entity).Get_StructData());
        });
}

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithThreeFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const FCk_DynamicFragment_ForEachEntity_ThreeFragments& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB, InStructTypeC},
        InFilter,
        [&](const FCk_Handle& Handle, const auto& Storages, entt::entity Entity)
        {
            InDelegate.Execute(
                Handle,
                Storages[0]->get(Entity).Get_StructData(),
                Storages[1]->get(Entity).Get_StructData(),
                Storages[2]->get(Entity).Get_StructData());
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
        const FCk_DynamicFragment_ForEachEntity_FourFragments& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB, InStructTypeC, InStructTypeD},
        InFilter,
        [&](const FCk_Handle& Handle, const auto& Storages, entt::entity Entity)
        {
            InDelegate.Execute(
                Handle,
                Storages[0]->get(Entity).Get_StructData(),
                Storages[1]->get(Entity).Get_StructData(),
                Storages[2]->get(Entity).Get_StructData(),
                Storages[3]->get(Entity).Get_StructData());
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
        const FCk_DynamicFragment_ForEachEntity_FiveFragments& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    ForEachEntity_WithDynamicFragments(
        InAnyHandle,
        std::array{InStructTypeA, InStructTypeB, InStructTypeC, InStructTypeD, InStructTypeE},
        InFilter,
        [&](const FCk_Handle& Handle, const auto& Storages, entt::entity Entity)
        {
            InDelegate.Execute(
                Handle,
                Storages[0]->get(Entity).Get_StructData(),
                Storages[1]->get(Entity).Get_StructData(),
                Storages[2]->get(Entity).Get_StructData(),
                Storages[3]->get(Entity).Get_StructData(),
                Storages[4]->get(Entity).Get_StructData());
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

    return entt::id_type{GetTypeHash(InStructType->GetPathName())};
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK
auto
    UCk_Utils_DynamicFragment_UE::
    Add_Fragment(
        FCk_Handle& InHandle,
        const FAngelscriptAnyStructParameter& InStructData)
    -> FCk_Handle
{
    return Add_Fragment(InHandle, InStructData.InstancedStruct);
}

auto
    UCk_Utils_DynamicFragment_UE::
    AddOrGet_Fragment(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FScriptStructWildcard&
{
    if (NOT Has_Fragment(InHandle, InStructType))
    {
        Add_Fragment(InHandle, FInstancedStruct{InStructType});
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
    static auto Invalid = FScriptStructWildcard{};

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return Invalid; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return Invalid; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    const auto Entity = InHandle.Get_Entity().Get_ID();

    CK_ENSURE_IF_NOT(Storage.contains(Entity),
        TEXT("Entity [{}] does NOT have the Dynamic Fragment [{}]! Cannot retrieve it"), InHandle, InStructType)
    { return Invalid; }

    auto& Fragment = Storage.get(Entity);
    return *(FScriptStructWildcard*)Fragment.Get_StructData().GetMemory();
}

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkDynamicFragment(static_cast<int32>(FAngelscriptBinds::EOrder::Late), []
{
    auto ExistingClass = FAngelscriptBinds::ExistingClass("FCk_Handle");

    ExistingClass.Method(
        "FCk_Handle Add_Fragment(const FInstancedStruct& InFragmentData)",
        [](FCk_Handle& Self, const FInstancedStruct& InFragmentData) -> FCk_Handle
        {
            return UCk_Utils_DynamicFragment_UE::Add_Fragment(Self, InFragmentData);
        });

    ExistingClass.Method(
        "FCk_Handle Add_Fragment(const FAngelscriptAnyStructParameter& InFragmentData)",
        [](FCk_Handle& Self, const FAngelscriptAnyStructParameter& InFragmentData) -> FCk_Handle
        {
            return UCk_Utils_DynamicFragment_UE::Add_Fragment(Self, InFragmentData);
        });

    ExistingClass.Method(
        "FScriptStructWildcard& AddOrGet_Fragment(const UScriptStruct InStructType)",
        [](FCk_Handle& Self, const UScriptStruct* InStructType) -> FScriptStructWildcard&
        {
            return UCk_Utils_DynamicFragment_UE::AddOrGet_Fragment(Self, InStructType);
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
        return &MutableAnyHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    });

    for (const auto Entity : static_cast<const entt::sparse_set&>(*Storages[0]))
    {
        if (NOT InAnyHandle->IsValid(Entity))
        { continue; }

        const auto HandleWithFragments = InAnyHandle.Get_ValidHandle(Entity);

        if (NOT PassesDestroyFilter(HandleWithFragments, InFilter))
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
