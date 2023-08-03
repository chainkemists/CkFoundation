#pragma once

#include "CkMacros/CkMacros.h"
#include "CkTypes/CkConstWrapper.h"

#include "CkEcs/Entity/CkEntity.h"

#include "CkEnsure/CkEnsure.h"

#include "entt/entt.hpp"
#include "ctti/type_id.hpp"

#include "CkRegistry.generated.h"

CK_DEFINE_CUSTOM_FORMATTER(ctti::detail::cstring, [&]()
{
    return FString(InObj.size(), InObj.begin());
});

USTRUCT(BlueprintType)
struct CKECS_API FCk_Registry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Registry);
    CK_ENABLE_CUSTOM_FORMATTER(FCk_Registry);

public:
    using ThisType = FCk_Registry;
    using InternalRegistryType = entt::registry;
    using InternalRegistryPtrType = TSharedPtr<InternalRegistryType>;
    using EntityType = FCk_Entity;

public:
    template <typename T_FragmentType, typename... T_Args>
    auto Add(EntityType InEntity, T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>;

    template <typename T_FragmentType, typename... T_Args>
    auto Replace(EntityType InEntity, T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>;

    template <typename T_Fragment>
    auto Remove(EntityType InEntity) -> void;

    template <typename T_Fragment>
    auto Try_Remove(EntityType InEntity) -> void;

    template <typename... T_Fragment>
    auto View(EntityType InEntity);

    template <typename... T_Fragment>
    auto View(EntityType InEntity) const;

    template <typename T_Fragment, typename T_Compare>
    auto Sort(T_Compare InCompare) -> void;

public:
    template <typename T_Fragment>
    auto Has(EntityType InEntity) -> bool;

    template <typename... T_Fragment>
    auto Has_Any(EntityType InEntity) -> bool;

    template <typename... T_Fragment>
    auto Has_All(EntityType InEntity) -> bool;

public:
    auto IsValid(EntityType InEntity) -> bool;

private:
    ck::TPtrWrapper<InternalRegistryPtrType> _InternalRegistry;
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Registry, [&]()
{
    return ck::Format
    (
        TEXT("{}"), static_cast<void*>(*InObj._InternalRegistry)
    );
});

// --------------------------------------------------------------------------------------------------------------------


template <typename T_FragmentType, typename ... T_Args>
auto FCk_Registry::Add(EntityType InEntity, T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return {}; }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity) == false, TEXT("Fragment/Tag [{}] already exists in Entity [{}]."), ctti::nameof<T_FragmentType>(), InEntity)
    { return {}; }

    if constexpr (std::is_empty_v<T_FragmentType>)
    {
        return {};
    }
    else
    {
        auto& Fragment = _InternalRegistry->emplace<T_FragmentType>(InEntity.Get_ID(), std::forward<T_Args>(InArgs)...);
        return Fragment;
    }
}

template <typename T_FragmentType, typename ... T_Args>
auto FCk_Registry::Replace(EntityType InEntity,
    T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>
{
    static_assert(std::is_empty_v<T_FragmentType> == false, "You can only replace Fragments with data.");

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Replace Fragment"), InEntity)
    { return {}; }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity),
                     TEXT("Unable to Replace Fragment. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
                     ctti::nameof<T_FragmentType>(), InEntity)
    { return {}; }

    auto& Fragment = _InternalRegistry->get<T_FragmentType>(InEntity.Get_ID());
    Fragment = T_FragmentType{ std::forward<T_Args>(InArgs)... };

    return Fragment;
}

template <typename T_Fragment>
auto FCk_Registry::Remove(EntityType InEntity) -> void
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return {}; }

    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
                     TEXT("Unable to Remove Fragment/Tag. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
                     ctti::nameof<T_Fragment>(), InEntity)
    { return {}; }

    _InternalRegistry->remove<T_Fragment>(InEntity);
}

template <typename T_Fragment>
auto FCk_Registry::Try_Remove(EntityType InEntity) -> void
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return {}; }

    _InternalRegistry->remove<T_Fragment>(InEntity);
}

template <typename... T_Fragment>
auto FCk_Registry::View(EntityType InEntity)
{
    return _InternalRegistry->view<T_Fragment...>(InEntity.Get_ID());
}

template <typename... T_Fragment>
auto FCk_Registry::View(EntityType InEntity) const
{
    return _InternalRegistry->view<T_Fragment...>(InEntity.Get_ID());
}

template <typename T_Fragment, typename T_Compare>
auto FCk_Registry::Sort(T_Compare InCompare) -> void
{
    _InternalRegistry->sort<T_Fragment>(InCompare);
}

template <typename T_Fragment>
auto FCk_Registry::Has(EntityType InEntity) -> bool
{
    return _InternalRegistry->any_of<T_Fragment>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto FCk_Registry::Has_Any(EntityType InEntity) -> bool
{
    return _InternalRegistry->any_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto FCk_Registry::Has_All(EntityType InEntity) -> bool
{
    return _InternalRegistry->all_of<T_Fragment...>(InEntity.Get_ID());
}
