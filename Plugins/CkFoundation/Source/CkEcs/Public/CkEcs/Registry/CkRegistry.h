#pragma once

#include "CkMacros/CkMacros.h"
#include "CkTypes/CkConstWrapper.h"

#include "CkEcs/Entity/CkEntity.h"

#include "CkEnsure/CkEnsure.h"

#include "entt/entt.hpp"

#include "CkRegistry.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // this is equivalent to entt::exclude for use with FRegistry::TView<...>
    // usage: Registry.View<CompA, CompB, TExclude<CompC>>().Each(...)
    template <typename... T>
    struct TExclude { using FValueType = entt::type_list<T...>; };
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Registry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Registry);
    CK_ENABLE_CUSTOM_FORMATTER(FCk_Registry);

public:
    friend class UCk_Utils_EntityLifetime_UE;

public:
    using InternalRegistryType = entt::registry;
    using InternalRegistryPtrType = TSharedPtr<InternalRegistryType>;
    using EntityType = FCk_Entity;

public:
    template <typename T_RegistryType, typename ... T_Fragments>
    class TView
    {
    public:
        template <typename T>
        struct TIsEmpty { static constexpr auto value = std::is_empty_v<T>; };

        template <typename T>
        struct TIsEmpty<ck::TExclude<T>>{ static constexpr auto value = std::is_empty_v<T>; };

        template <typename... T_Args>
        struct TTypeOnly { using FTypeList = entt::type_list<T_Args...>; };

        template <typename... T_Args>
        struct TTypeOnly<ck::TExclude<T_Args>...> { using FTypeList = entt::type_list<T_Args...>; };

        template <typename... T_Args>
        using TTypeOnly_T = entt::type_list_cat_t<typename TTypeOnly<T_Args>::FTypeList...>;

        template <typename T>
        struct TIsExcluded : std::false_type { };

        template <typename T>
        struct TIsExcluded<ck::TExclude<T>> : std::true_type { };

        template <typename... T_Args>
        using TExcludesStripped = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value, entt::type_list<>, TTypeOnly_T<T_Args>>...>;

        template <typename... T_Args>
        using TExcludesOnly = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value, TTypeOnly_T<T_Args>, entt::type_list<>>...>;

        template <typename... T_Args>
        using TFragmentsOnly = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value || TIsEmpty<T_Args>::value, entt::type_list<>, TTypeOnly_T<T_Args>>...>;

    public:
        using FRegistryType = T_RegistryType;
        using FFragmentsAndTags = TExcludesStripped<T_Fragments...>;
        using FOnlyExcludes = TExcludesOnly<T_Fragments...>;
        using FOnlyFragments = TFragmentsOnly<T_Fragments...>;

    public:
        explicit TView(FRegistryType& InRegistry)
            : _Registry(InRegistry)
        {
        }

        template <typename T_Func>
        auto Each(T_Func InFunc)
        {
            DoEach(InFunc, FFragmentsAndTags{}, FOnlyExcludes{}, FOnlyFragments{});
        }

    private:
        template <typename T_Func, typename... T_FragmentsAndTags, typename... T_OnlyExcludes, typename... T_OnlyFragments>
        auto DoEach(T_Func InFunc, entt::type_list<T_FragmentsAndTags...>, entt::type_list<T_OnlyExcludes...>, entt::type_list<T_OnlyFragments...>)
        {
            _Registry.template view<T_FragmentsAndTags...>(entt::exclude<T_OnlyExcludes...>).each(
            [InFunc](const EntityType::IdType InEntityId, T_OnlyFragments&... InFragments)
            {
                const auto TypeSafeEntity = FCk_Entity{InEntityId};
                InFunc(TypeSafeEntity, InFragments...);
            });
        }

    private:
        FRegistryType& _Registry;
    };

public:
    FCk_Registry();

public:
    template <typename T_FragmentType, typename... T_Args>
    auto Add(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto AddOrGet(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename T_Func>
    auto Try_Transform(EntityType InEntity, T_Func InFunc) -> void;

    template <typename T_FragmentType, typename... T_Args>
    auto Replace(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_Fragment>
    auto Remove(EntityType InEntity) -> void;

    template <typename T_Fragment>
    auto Try_Remove(EntityType InEntity) -> void;

    template <typename... T_Fragments>
    auto Clear() -> void;

    template <typename... T_Fragments>
    auto View() -> TView<InternalRegistryType, T_Fragments...>;

    template <typename... T_Fragments>
    auto View() const -> TView<const InternalRegistryType, T_Fragments...>;

    template <typename T_Fragment, typename T_Compare>
    auto Sort(T_Compare InCompare) -> void;

public:
    template <typename T_Fragment>
    auto Has(EntityType InEntity) const -> bool;

    template <typename... T_Fragment>
    auto Has_Any(EntityType InEntity) const -> bool;

    template <typename... T_Fragment>
    auto Has_All(EntityType InEntity) const -> bool;

    template <typename T_Fragment>
    auto Get(EntityType InEntity) -> T_Fragment&;

    template <typename T_Fragment>
    auto Get(EntityType InEntity) const -> const T_Fragment&;

private:
    auto CreateEntity() -> EntityType;
    auto CreateEntity(EntityType InEntityHint) -> EntityType;
    auto DestroyEntity(EntityType InEntity) -> void;

public:
    auto IsValid(EntityType InEntity) const -> bool;
    auto Get_ValidEntity(EntityType::IdType InEntity) const -> EntityType;

private:
    ck::TPtrWrapper<InternalRegistryPtrType> _InternalRegistry;
    EntityType _TransientEntity;
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Registry, [&]()
{
    return ck::Format
    (
        TEXT("{}"), static_cast<const void*>(&*InObj._InternalRegistry)
    );
});

// --------------------------------------------------------------------------------------------------------------------


template <typename T_FragmentType, typename ... T_Args>
auto FCk_Registry::Add(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity) == false, TEXT("Fragment/Tag [{}] already exists in Entity [{}]."), ctti::nameof<T_FragmentType>(), InEntity)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    if constexpr (std::is_empty_v<T_FragmentType>)
    {
        _InternalRegistry->emplace<T_FragmentType>(InEntity.Get_ID());
        static T_FragmentType EMPTY_Tag;
        return EMPTY_Tag;
    }
    else
    {
        auto& Fragment = _InternalRegistry->emplace<T_FragmentType>(InEntity.Get_ID(), std::forward<T_Args>(InArgs)...);
        return Fragment;
    }
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::AddOrGet(EntityType InEntity,
        T_Args&&... InArgs) -> T_FragmentType&
{
    if (Has<T_FragmentType>(InEntity))
    { return Get<T_FragmentType>(InEntity); }

    return Add<T_FragmentType>(InEntity, std::forward<T_Args>(InArgs)...);
}

template <typename T_FragmentType, typename T_Func>
auto
    FCk_Registry::
    Try_Transform(
        EntityType InEntity,
        T_Func InFunc)
    -> void
{
    if (NOT Has<T_FragmentType>(InEntity))
    { return; }

    InFunc(Get<T_FragmentType>(InEntity));
}

template <typename T_FragmentType, typename ... T_Args>
auto FCk_Registry::Replace(EntityType InEntity,
    T_Args&&... InArgs) -> T_FragmentType&
{
    static_assert(std::is_empty_v<T_FragmentType> == false, "You can only replace Fragments with data.");

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Replace Fragment"), InEntity)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity),
                     TEXT("Unable to Replace Fragment. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
                     ctti::nameof<T_FragmentType>(), InEntity)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    auto& Fragment = _InternalRegistry->get<T_FragmentType>(InEntity.Get_ID());
    Fragment = T_FragmentType{ std::forward<T_Args>(InArgs)... };

    return Fragment;
}

template <typename T_Fragment>
auto FCk_Registry::Remove(EntityType InEntity) -> void
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return; }

    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
                     TEXT("Unable to Remove Fragment/Tag. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
                     ctti::nameof<T_Fragment>(), InEntity)
    { return; }

    _InternalRegistry->remove<T_Fragment>(InEntity.Get_ID());
}

template <typename T_Fragment>
auto FCk_Registry::Try_Remove(EntityType InEntity) -> void
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return {}; }

    _InternalRegistry->remove<T_Fragment>(InEntity);
}

template <typename ... T_Fragments>
auto FCk_Registry::Clear() -> void
{
    _InternalRegistry->clear<T_Fragments...>();
}

template <typename... T_Fragments>
auto FCk_Registry::View() -> TView<InternalRegistryType, T_Fragments...>
{
    return TView<InternalRegistryType, T_Fragments...>{*_InternalRegistry};
}

template <typename... T_Fragments>
auto FCk_Registry::View() const -> TView<const InternalRegistryType, T_Fragments...>
{
    return TView<const InternalRegistryType, T_Fragments...>{*_InternalRegistry};
}

template <typename T_Fragment, typename T_Compare>
auto FCk_Registry::Sort(T_Compare InCompare) -> void
{
    _InternalRegistry->sort<T_Fragment>(InCompare);
}

template <typename T_Fragment>
auto FCk_Registry::Has(EntityType InEntity) const -> bool
{
    return _InternalRegistry->any_of<T_Fragment>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto FCk_Registry::Has_Any(EntityType InEntity) const -> bool
{
    return _InternalRegistry->any_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto FCk_Registry::Has_All(EntityType InEntity) const -> bool
{
    return _InternalRegistry->all_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename T_Fragment>
auto FCk_Registry::Get(EntityType InEntity) -> T_Fragment&
{
    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
                     TEXT("Unable to Get Fragment. Fragment [{}] does NOT exist in Entity [{}]."),
                     ctti::nameof<T_Fragment>(), InEntity)
    {
        static T_Fragment INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _InternalRegistry->get<T_Fragment>(InEntity.Get_ID());
}

template <typename T_Fragment>
auto FCk_Registry::Get(EntityType InEntity) const -> const T_Fragment&
{
    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
                     TEXT("Unable to Get Fragment. Fragment [{}] does NOT exist in Entity [{}]."),
                     ctti::nameof<T_Fragment>(), InEntity)
    {
        static T_Fragment INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _InternalRegistry->get<T_Fragment>(InEntity.Get_ID());
}
