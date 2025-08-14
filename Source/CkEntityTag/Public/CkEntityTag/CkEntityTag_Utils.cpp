#include "CkEntityTag_Utils.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Add(
        FCk_Handle& InHandle,
        FName InTag)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to add EntityTag [{}] to Handle [{}] that is INVALID"), InTag, InHandle)
    { return {}; }

    auto Params = ck::FFragment_EntityTag_Params{InTag};
    InHandle.Add<ck::FFragment_EntityTag_Params>(Params);

    auto&& Storage = InHandle->Storage<ck::FFragment_EntityTag_Params>(entt::id_type{GetTypeHash(InTag)});
    Storage.emplace<ck::FFragment_EntityTag_Params>(InHandle.Get_Entity().Get_ID(), std::move(Params));

    return InHandle;
}

auto
    UCk_Utils_EntityTag_UE::
    Add_UsingGameplayTag(
        FCk_Handle& InHandle,
        FGameplayTag InTag)
    -> FCk_Handle
{
    const auto Name = InTag.GetTagName();
    return Add(InHandle, Name);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityTag_UE::
    Request_TryRemove(
        FCk_Handle& InHandle,
        FName InTag)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle passed. Unable to remove Tag [{}] from Entity"), InTag)
    { return ECk_SucceededFailed::Failed; }

    auto&& Storage = InHandle->Storage<ck::FFragment_EntityTag_Params>(entt::id_type{GetTypeHash(InTag)});
    InHandle.Try_Remove<ck::FFragment_EntityTag_Params>();

    const auto Entity = InHandle.Get_Entity().Get_ID();

    if(NOT Storage.contains(Entity))
    { return ECk_SucceededFailed::Failed; }

    Storage.remove(Entity);
    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_EntityTag_UE::
    TryGet_Tag(
        const FCk_Handle& InHandle)
    -> FName
{
    if (InHandle.Has<ck::FFragment_EntityTag_Params>())
    {
        return *InHandle.Get<ck::FFragment_EntityTag_Params>().Get_Tag().ToString();
    }

    return {};
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity(
        const FCk_Handle& InAnyHandle,
        FName InTag)
        -> TArray<FCk_Handle>
{
    auto EntityTags = TArray<FCk_Handle>{};

    ForEach_Entity(InAnyHandle, InTag, [&](FCk_Handle InEntity)
    {
        EntityTags.Emplace(InEntity);
    });

    return EntityTags;
}

auto
    UCk_Utils_EntityTag_UE::
    ForEach_Entity(
        const FCk_Handle& InAnyHandle,
        FName InTag,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandle), TEXT("Invalid Handle passed. Unable to iterate over Entities with Tag [{}]"), InTag)
    { return; }

    auto Handle = InAnyHandle;
    auto& Storage = Handle->Storage<ck::FFragment_EntityTag_Params>(entt::id_type{GetTypeHash(InTag)});

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