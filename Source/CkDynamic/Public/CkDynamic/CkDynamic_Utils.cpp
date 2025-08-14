#include "CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkDynamic/CkDynamic_Fragment.h"

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

    CK_ENSURE_IF_NOT(ck::IsValid(InFragmentData) , TEXT("Invalid struct data in FAngelscriptAnyStructParameter"))
    { return InHandle; }

    // Create fragment with the struct data
    auto Fragment = ck::FFragment_DynamicFragment_Data{InFragmentData};

    // Get the storage ID for this struct type
    auto StorageId = Get_StorageId(InFragmentData.GetScriptStruct());

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

auto
    UCk_Utils_DynamicFragment_UE::
    Get_Fragment(
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
    auto& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

    auto Entity = InHandle.Get_Entity().Get_ID();

    if (NOT Storage.contains(Entity))
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
    auto& Storage = InHandle->Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

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
        const UScriptStruct* InStructType)
    -> entt::id_type
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStructType), TEXT("Invalid struct type"))
    { return entt::id_type{}; }

    // Use the struct type's path name hash as the storage ID
    return entt::id_type{GetTypeHash(InStructType->GetPathName())};
}

// --------------------------------------------------------------------------------------------------------------------
