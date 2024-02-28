#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/CkPtrWrapper.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkMemory/Allocator/CkMemoryAllocator.h"

#include "entt/entity/registry.hpp"

#include "CkRegistry.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityLifetime_DestroyEntity;
}

namespace ck
{
    // this is equivalent to entt::exclude for use with FRegistry::TView<...>
    // usage: Registry.View<CompA, CompB, TExclude<CompC>>().ForEach(...)
    template <typename... T_Args>
    struct TExclude { using ValueType = entt::type_list<T_Args...>; };
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
    friend class ck::FProcessor_EntityLifetime_DestroyEntity;

    friend struct FCk_Handle;

public:
    using EntityType = FCk_Entity;

#if CK_MEMORY_TRACKING
    // TODO: Replace 'std::allocator' with custom one when it is created
    using InternalRegistryType = entt::basic_registry<EntityType::IdType, std::allocator<EntityType::IdType>>;
#else
    using InternalRegistryType = entt::basic_registry<EntityType::IdType, std::allocator<EntityType::IdType>>;
#endif

    using InternalRegistryPtrType = TSharedPtr<InternalRegistryType>;

public:
    template <typename T_RegistryType, typename ... T_Fragments>
    class TView
    {
    public:
        template <typename T>
        // ReSharper disable once CppInconsistentNaming
        struct TIsEmpty { static constexpr auto value = std::is_empty_v<T>; };

        template <typename T>
        // ReSharper disable once CppInconsistentNaming
        struct TIsEmpty<ck::TExclude<T>>{ static constexpr auto value = std::is_empty_v<T>; };

        template <typename... T_Args>
        struct TTypeOnly { using TypeList = entt::type_list<T_Args...>; };

        template <typename... T_Args>
        struct TTypeOnly<ck::TExclude<T_Args>...> { using TypeList = entt::type_list<T_Args...>; };

        template <typename... T_Args>
        using TypeOnly_T = entt::type_list_cat_t<typename TTypeOnly<T_Args>::TypeList...>;

        template <typename T>
        struct TIsExcluded : std::false_type { };

        template <typename T>
        struct TIsExcluded<ck::TExclude<T>> : std::true_type { };

        template <typename... T_Args>
        using ExcludesStripped = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value, entt::type_list<>, TypeOnly_T<T_Args>>...>;

        template <typename... T_Args>
        using ExcludesOnly = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value, TypeOnly_T<T_Args>, entt::type_list<>>...>;

        template <typename... T_Args>
        using FragmentsOnly = entt::type_list_cat_t<std::conditional_t<TIsExcluded<T_Args>::value || TIsEmpty<T_Args>::value, entt::type_list<>, TypeOnly_T<T_Args>>...>;

    public:
        using RegistryType = T_RegistryType;
        using FragmentsAndTags = ExcludesStripped<T_Fragments...>;
        using OnlyExcludes = ExcludesOnly<T_Fragments...>;
        using OnlyFragments = FragmentsOnly<T_Fragments...>;

    public:
        explicit TView(RegistryType& InRegistry)
            : _Registry(InRegistry)
        {
        }

        template <typename T_Func>
        auto ForEach(T_Func InFunc)
        {
            DoForEach(InFunc, FragmentsAndTags{}, OnlyExcludes{}, OnlyFragments{});
        }

    private:
        template <typename T_Func, typename... T_FragmentsAndTags, typename... T_OnlyExcludes, typename... T_OnlyFragments>
        auto DoForEach(T_Func InFunc, entt::type_list<T_FragmentsAndTags...>, entt::type_list<T_OnlyExcludes...>, entt::type_list<T_OnlyFragments...>)
        {
            _Registry.template view<T_FragmentsAndTags...>(entt::exclude<T_OnlyExcludes...>).each(
            [InFunc](const EntityType::IdType InEntityId, T_OnlyFragments&... InFragments)
            {
                const auto TypeSafeEntity = FCk_Entity{InEntityId};
                InFunc(TypeSafeEntity, InFragments...);
            });
        }

    private:
        RegistryType& _Registry;
    };

    template <typename... T_Fragments>
    using RegistryViewType = TView<InternalRegistryType, T_Fragments...>;

    template <typename... T_Fragments>
    using ConstRegistryViewType = TView<const InternalRegistryType, T_Fragments...>;

public:
    FCk_Registry();

public:
    template <typename T_Fragment, typename T_Compare>
    auto Sort(T_Compare InCompare) -> void;

    template <typename T_FragmentType, typename T_Func>
    auto Try_Transform(EntityType InEntity, T_Func InFunc) -> void;

private:
    template <typename T_FragmentType, typename... T_Args>
    auto Add(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto AddOrGet(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto Replace(EntityType InEntity, T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_Fragment>
    auto Remove(EntityType InEntity) -> void;

    template <typename T_Fragment>
    auto Try_Remove(EntityType InEntity) -> void;

    template <typename... T_Fragments>
    auto Clear() -> void;

public:
    template <typename... T_Fragments>
    auto View() -> RegistryViewType<T_Fragments...>;

    template <typename... T_Fragments>
    auto View() const -> ConstRegistryViewType<T_Fragments...>;

private:
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
    auto Orphan(EntityType InEntity) const -> bool;
    auto Get_ValidEntity(EntityType::IdType InEntity) const -> EntityType;

public:
    friend auto CKECS_API GetTypeHash(const ThisType& InRegistry) -> uint32;

private:
    ck::TPtrWrapper<InternalRegistryPtrType> _InternalRegistry;
    EntityType _TransientEntity;

private:
    CK_PROPERTY(_TransientEntity);
};

// --------------------------------------------------------------------------------------------------------------------

auto CKECS_API GetTypeHash(const FCk_Registry& InRegistry) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Registry, [&]()
{
    return ck::Format
    (
        TEXT("{}"), static_cast<const void*>(&*InObj._InternalRegistry)
    );
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(FCk_Registry, ck::IsValid_Policy_Default, [=](const FCk_Registry& InRegistry)
{
    return ck::IsValid(InRegistry._InternalRegistry);
});

// --------------------------------------------------------------------------------------------------------------------


template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::
    Add(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity) == false,
        TEXT("Fragment [{}] already exists in Entity [{}]."),
        ck::TypeToString<T_FragmentType>, InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    if constexpr (std::is_empty_v<T_FragmentType>)
    {
        static_assert(std::is_base_of_v<ck::TTag<T_FragmentType>, T_FragmentType>, "Tags must derive from ck::TTag (see helper macro)");

        _InternalRegistry->emplace<T_FragmentType>(InEntity.Get_ID());
        static T_FragmentType Empty_Tag;
        return Empty_Tag;
    }
    else
    {
        auto& Fragment = _InternalRegistry->emplace<T_FragmentType>(InEntity.Get_ID(), std::forward<T_Args>(InArgs)...);
        return Fragment;
    }
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Registry::
    AddOrGet(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
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
auto
    FCk_Registry::
    Replace(
        EntityType InEntity,
        T_Args&&... InArgs)
    -> T_FragmentType&
{
    static_assert(std::is_empty_v<T_FragmentType> == false, "You can only replace Fragments with data.");

    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Replace Fragment"), InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    CK_ENSURE_IF_NOT(Has<T_FragmentType>(InEntity),
        TEXT("Unable to Replace Fragment. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
        ck::TypeToString<T_FragmentType>, InEntity)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    auto& Fragment = _InternalRegistry->get<T_FragmentType>(InEntity.Get_ID());
    Fragment = T_FragmentType{ std::forward<T_Args>(InArgs)... };

    return Fragment;
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Remove(
        EntityType InEntity)
    -> void
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return; }

    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
        TEXT("Unable to Remove Fragment/Tag. Fragment/Tag [{}] does NOT exist in Entity [{}]."),
        ck::TypeToString<T_Fragment>, InEntity)
    { return; }

    _InternalRegistry->remove<T_Fragment>(InEntity.Get_ID());
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Try_Remove(
        EntityType InEntity)
    -> void
{
    CK_ENSURE_IF_NOT(IsValid(InEntity), TEXT("Invalid Entity [{}]. Unable to Add Fragment/Tag."), InEntity)
    { return; }

    _InternalRegistry->remove<T_Fragment>(InEntity.Get_ID());
}

template <typename ... T_Fragments>
auto
    FCk_Registry::
    Clear()
    -> void
{
    _InternalRegistry->clear<T_Fragments...>();
}

template <typename... T_Fragments>
auto
    FCk_Registry::
    View()
    -> RegistryViewType<T_Fragments...>
{
    return TView<InternalRegistryType, T_Fragments...>{*_InternalRegistry};
}

template <typename... T_Fragments>
auto
    FCk_Registry::
    View() const
    -> ConstRegistryViewType<T_Fragments...>
{
    return TView<const InternalRegistryType, T_Fragments...>{*_InternalRegistry};
}

template <typename T_Fragment, typename T_Compare>
auto
    FCk_Registry::
    Sort(
        T_Compare InCompare)
    -> void
{
    _InternalRegistry->sort<T_Fragment>(InCompare);
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Has(
        EntityType InEntity) const
    -> bool
{
    return _InternalRegistry->any_of<T_Fragment>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto
    FCk_Registry::
    Has_Any(
        EntityType InEntity) const
    -> bool
{
    return _InternalRegistry->any_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename ... T_Fragment>
auto
    FCk_Registry::
    Has_All(
        EntityType InEntity) const
    -> bool
{
    return _InternalRegistry->all_of<T_Fragment...>(InEntity.Get_ID());
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Get(
        EntityType InEntity)
    -> T_Fragment&
{
    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
         TEXT("Unable to Get Fragment. Fragment [{}] does NOT exist in Entity [{}]."),
         ck::TypeToString<T_Fragment>, InEntity)
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        static T_Fragment Empty_Tag;
        return Empty_Tag;
    }
    else
    {
        return _InternalRegistry->get<T_Fragment>(InEntity.Get_ID());
    }
}

template <typename T_Fragment>
auto
    FCk_Registry::
    Get(
        EntityType InEntity) const
    -> const T_Fragment&
{
    CK_ENSURE_IF_NOT(Has<T_Fragment>(InEntity),
        TEXT("Unable to Get Fragment. Fragment [{}] does NOT exist in Entity [{}]."),
        ck::TypeToString<T_Fragment>, InEntity)
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        static T_Fragment Empty_Tag;
        return Empty_Tag;
    }
    else
    {
        return _InternalRegistry->get<T_Fragment>(InEntity.Get_ID());
    }
}

// --------------------------------------------------------------------------------------------------------------------
