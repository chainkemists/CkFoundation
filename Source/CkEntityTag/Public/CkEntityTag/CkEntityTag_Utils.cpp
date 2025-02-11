#include "CkEntityTag_Utils.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Add(
        FCk_Handle& InHandle,
        FCk_Fragment_EntityTag_ParamsData InParams)
    -> FCk_Handle
{
    // TODO: think about whether we even need to store anything in the Entity. It's extra cost we can do without
    // downside is that Entity debugging will not show the tags

    auto&& Storage = InHandle->Storage<ck::FFragment_EntityTag_Params>(entt::id_type{GetTypeHash(InParams.Get_Tag())});

    Storage.emplace<ck::FFragment_EntityTag_Params>(InHandle.Get_Entity().Get_ID(), std::move(InParams));

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Request_TryRemove(
        FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to remove Tag [{}] from Entity"), InTag)
    { return ECk_SucceededFailed::Failed; }

    auto&& Storage = InHandle->Storage<ck::FFragment_EntityTag_Params>(entt::id_type{GetTypeHash(InTag)});

    const auto Entity = InHandle.Get_Entity().Get_ID();

    if(NOT Storage.contains(Entity))
    { return ECk_SucceededFailed::Failed; }

    Storage.remove(Entity);
    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity(
        FCk_Handle InAnyHandle,
        FGameplayTag InTag,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
        -> TArray<FCk_Handle>
{
    auto EntityTags = TArray<FCk_Handle>{};

    ForEach_Entity(InAnyHandle, InTag, [&](FCk_Handle InEntity)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InEntity, InOptionalPayload); }
        else
        { EntityTags.Emplace(InEntity); }
    });

    return EntityTags;
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity(
        FCk_Handle InAnyHandle,
        FGameplayTag InTag,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandle), TEXT("Invalid Handle passed"))
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandle), TEXT("Invalid Handle passed. Unable to iterate over Entities with Tag [{}]"), InTag)
    { return; }

    auto& Storage = InAnyHandle->Storage<ck::FFragment_EntityTag_Params>(entt::id_type{GetTypeHash(InTag)});

    const auto View = entt::basic_view{Storage};
    View.each([&](const auto InEntity, const ck::FFragment_EntityTag_Params& InParams)
    {
        if (NOT InAnyHandle->IsValid(InEntity))
        { return; }

        auto Handle = InAnyHandle.Get_ValidHandle(InEntity);
        InFunc(Handle);
    });
}


// --------------------------------------------------------------------------------------------------------------------