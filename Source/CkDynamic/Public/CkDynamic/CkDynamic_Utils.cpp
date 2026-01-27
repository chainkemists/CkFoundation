#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkDynamic/CkDynamic_Fragment.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBindString.h>
#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>
#include "CkDynamic/CkDynamic_AngelScript.h"
#include <ClassGenerator/ASStruct.h>
#endif

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
    auto StorageId = Get_StorageId(InFragmentData.GetScriptStruct());
    auto&& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    auto Entity = InHandle.Get_Entity().Get_ID();

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
    const auto& Result = Request_TryRemove(InHandle, InStructType);

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
    auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    {
        return ECk_SucceededFailed::Failed;
    }

    Storage.remove(Entity);

    auto HasOtherFragments = false;

    for (auto Pair = InHandle->Storage().begin(); Pair != InHandle->Storage().end(); ++Pair)
    {
        if (auto& Pool = Pair->second;
            Pool.type() == entt::type_id<ck::FFragment_DynamicFragment_Data>() && Pool.contains(Entity))
        {
            HasOtherFragments = true;
            break;
        }
    }

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
    static FInstancedStruct Invalid;

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment [{}] type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return Invalid; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return Invalid; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    auto Entity = InHandle.Get_Entity().Get_ID();

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
        TEXT("Invalid Dynamic Fragment [{}] type passed. Unable to query Dynamic Fragment from [{}]"), InHandle)
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to query Dynamic Fragment [{}]"), InHandle, InStructType)
    { return false; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    auto Entity = InHandle.Get_Entity().Get_ID();

    return Storage.contains(Entity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithFragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandle),
        TEXT("Invalid Handle [{}] passed. Unable to iterate over Entities with Dynamic Fragment [{}]"), InAnyHandle, InStructType)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment [{}] type passed. Unable to iterate over Entities with Dynamic Fragment"), InStructType)
    { return; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InAnyHandle;
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto View = entt::basic_view{Storage};
    View.each([&](entt::entity InEntity, FCk_Fragment_DynamicFragment_Data& InFragment)
    {
        if (NOT InAnyHandle->IsValid(InEntity))
        {
            return;
        }

        const auto _Handle = InAnyHandle.Get_ValidHandle(InEntity);

        switch (InFilter)
        {
            case ECk_DestroyFilter::None:
            {
                break;
            }
            case ECk_DestroyFilter::IgnorePendingKill:
            {
                if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_Handle, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
                    UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_Handle, ECk_EntityLifetime_DestructionPhase::Teardown) ||
                    UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_Handle, ECk_EntityLifetime_DestructionPhase::Destroyed))
                {
                    return;
                }

                break;
            }
            case ECk_DestroyFilter::Teardown:
            {
                if (NOT UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(_Handle, ECk_EntityLifetime_DestructionPhase::Teardown))
                {
                    return;
                }
                break;
            }
        }

        InDelegate.Execute(_Handle, InFragment.Get_StructData());
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
    static FScriptStructWildcard Invalid;

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType),
        TEXT("Invalid Dynamic Fragment [{}] type passed. Unable to get Dynamic Fragment from [{}]"), InHandle)
    { return Invalid; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid Handle [{}] passed. Unable to get Dynamic Fragment [{}]"), InHandle, InStructType)
    { return Invalid; }

    const auto StorageId = Get_StorageId(InStructType);
    auto Handle = InHandle;
    auto& Storage = Handle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);
    auto Entity = InHandle.Get_Entity().Get_ID();

    CK_ENSURE_IF_NOT(Storage.contains(Entity),
        TEXT("Entity [{}] does NOT have the Dynamic Fragment [{}]! Cannot retrieve it"), InHandle, InStructType)
    { return Invalid; }

    auto& Fragment = Storage.get(Entity);
    return *(FScriptStructWildcard*)Fragment.Get_StructData().GetMemory();
}

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkDynamicFragment(static_cast<int32>(FAngelscriptBinds::EOrder::Late), []
{
    auto Namespace = FAngelscriptBinds::FNamespace{"utils_dynamic_fragment"};
    FAngelscriptBinds::BindGlobalFunction(
        "FScriptStructWildcard& AddOrGet_Fragment(const FCk_Handle& InHandle, const UScriptStruct InStructType)",
        FUNC(UCk_Utils_DynamicFragment_UE::AddOrGet_Fragment));
    FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(1);
    FAngelscriptBinds::BindGlobalFunction(
        "FScriptStructWildcard& Get_Fragment(const FCk_Handle& InHandle, const UScriptStruct InStructType)",
        FUNC(UCk_Utils_DynamicFragment_UE::Get_Fragment));
    FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(1);
});

#endif

// --------------------------------------------------------------------------------------------------------------------