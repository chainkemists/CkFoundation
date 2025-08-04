#include "CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkDynamic/CkDynamic_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle
    UCk_Utils_DynamicFragment_UE::
    Add_Fragment(
        FCk_Handle& InHandle,
        const FAngelscriptAnyStructParameter& InStructData)
{
    auto StructType = const_cast<UScriptStruct*>(InStructData.InstancedStruct.GetScriptStruct());
    auto StructData = InStructData.InstancedStruct.GetMemory();

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to add Fragment"))
    { return InHandle; }

    CK_ENSURE_IF_NOT(ck::IsValid(StructType) && StructData != nullptr, TEXT("Invalid struct data in FAngelscriptAnyStructParameter"))
    { return InHandle; }

    // Create fragment with the struct data
    auto Fragment = ck::FFragment_DynamicFragment_Data{InStructData};

    // Get the storage ID for this struct type
    auto StorageId = Get_StorageId(StructType);

    // Get or create the storage for this struct type
    auto&& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    // Remove existing fragment if present
    auto Entity = InHandle.Get_Entity().Get_ID();
    if (Storage.contains(Entity))
    {
        Storage.remove(Entity);
    }

    // Add to storage
    Storage.emplace(Entity, std::move(Fragment));

    // Also add to the main registry so we can query for entities with fragments
    if (NOT InHandle.Has<ck::FFragment_DynamicFragment_Data>())
    {
        InHandle.Add<ck::FFragment_DynamicFragment_Data>(Fragment);
    }

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Request_TryRemove(
        FCk_Handle& InHandle,
        UScriptStruct* InStructType)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to remove Fragment"))
    { return ECk_SucceededFailed::Failed; }

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type passed. Unable to remove Fragment"))
    { return ECk_SucceededFailed::Failed; }

    auto StorageId = Get_StorageId(InStructType);
    auto&& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    { return ECk_SucceededFailed::Failed; }

    Storage.remove(Entity);

    // Check if this entity has any other fragments
    auto HasOtherFragments = false;

    for (auto Pair = InHandle->Storage().begin(); Pair != InHandle->Storage().end(); ++Pair)
    {
        if (auto &Pool = Pair->second;
            Pool.type() == entt::type_id<ck::FFragment_DynamicFragment_Data>() && Pool.contains(Entity))
        {
            HasOtherFragments = true;
            break;
        }
    }

    // If no other fragments, remove the main fragment marker
    if (NOT HasOtherFragments)
    {
        InHandle.Try_Remove<ck::FFragment_DynamicFragment_Data>();
    }

    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Utils_DynamicFragment_UE::
    TryGet_Fragment(
        const FCk_Handle& InHandle,
        UScriptStruct* InStructType,
        FAngelscriptAnyStructParameter& OutStructData)
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to get Fragment"))
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type passed. Unable to get Fragment"))
    { return false; }

    auto StorageId = Get_StorageId(InStructType);
    auto& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    { return false; }

    auto& Fragment = Storage.get(Entity);
    OutStructData = Fragment.Get_StructData();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

ECk_SucceededFailed
    UCk_Utils_DynamicFragment_UE::
    Request_UpdateFragment(
        FCk_Handle& InHandle,
        const FAngelscriptAnyStructParameter& InStructData)
{
    auto StructType = const_cast<UScriptStruct*>(InStructData.InstancedStruct.GetScriptStruct());
    auto StructData = InStructData.InstancedStruct.GetMemory();

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to update Fragment"))
    { return ECk_SucceededFailed::Failed; }

    CK_ENSURE_IF_NOT(ck::IsValid(StructType) && StructData != nullptr, TEXT("Invalid struct data in FAngelscriptAnyStructParameter"))
    { return ECk_SucceededFailed::Failed; }

    auto StorageId = Get_StorageId(StructType);
    auto&& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
    { return ECk_SucceededFailed::Failed; }

    auto& Fragment = Storage.get(Entity);
    Fragment = FCk_Fragment_DynamicFragment_Data{InStructData};
    return ECk_SucceededFailed::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Has_Fragment(
        const FCk_Handle& InHandle,
        UScriptStruct* InStructType)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to check Fragment"))
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type passed. Unable to check Fragment"))
    { return false; }

    auto StorageId = Get_StorageId(InStructType);
    auto& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto Entity = InHandle.Get_Entity().Get_ID();
    return Storage.contains(Entity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    ForEach_EntityWithFragment(
        const FCk_Handle& InAnyHandle,
        UScriptStruct* InStructType,
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
    auto& Storage = InAnyHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto View = entt::basic_view{Storage};
    View.each([&](entt::entity InEntity, FCk_Fragment_DynamicFragment_Data& InFragment)
    {
        if (NOT InAnyHandle->IsValid(InEntity))
        { return; }

        const auto Handle = InAnyHandle.Get_ValidHandle(InEntity);

        // @NOTE: MUST be kept up to date with CkEcs\EntityLifetime\CkEntityLifetime_Fragment.h
        switch(InFilter)
        {
            case ECk_DestroyFilter::None:
            {
                break;
            }
            case ECk_DestroyFilter::IgnorePendingKill:
            {
                if (Handle.Has_Any<ck::FTag_DestroyEntity_Await, ck::FTag_DestroyEntity_Finalize>())
                { return; }
                break;
            }
            case ECk_DestroyFilter::Teardown:
            {
                if (NOT Handle.Has_Any<ck::FTag_DestroyEntity_Initiate_Confirm>())
                { return; }
                break;
            }
        }

        InDelegate.Execute(Handle, InFragment.Get_StructData());
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DynamicFragment_UE::
    Get_StorageId(
        UScriptStruct* InStructType)
    -> entt::id_type
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type"))
    { return entt::id_type{}; }

    // Use the struct type's path name hash as the storage ID
    return entt::id_type{GetTypeHash(InStructType->GetPathName())};
}

// --------------------------------------------------------------------------------------------------------------------
