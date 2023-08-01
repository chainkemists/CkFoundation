#pragma once

#include "CkMacros/CkMacros.h"
#include "CkTypes/CkConstWrapper.h"

#include "CkEcs/Entity/CkEntity.h"

#include "entt/entt.hpp"

#include "CkRegistry.generated.h"

USTRUCT(BlueprintType)
struct CKECS_API FCk_Registry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Registry);

public:
    using ThisType = FCk_Registry;
    using InternalRegistryType = entt::registry;

    using EntityType = FCk_Entity;

public:
    template <typename T_ComponentType, typename... T_Args>
    auto Add(EntityType InEntity, T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_ComponentType>>;

    template <typename T_ComponentType, typename... T_Args>
    auto Replace(EntityType InEntity, T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_ComponentType>>;

    template <typename T_Component>
    auto Remove(EntityType InEntity) -> void;

    template <typename T_Component>
    auto Try_Remove(EntityType InEntity) -> void;

    template <typename T_Component>
    auto View(EntityType InEntity);

    template <typename T_Component>
    auto View(EntityType InEntity) const;

    template <typename T_Component, typename T_Comparitor>
    auto Sort(T_Comparitor InComparitor) -> void;

public:
    template <typename T_Component, typename T_Policy, std::enable_if_t<std::is_same_v<T_Policy, ck::policy::All> || std::is_same_v<T_Policy, ck::policy::Any>> = 0>
    auto Has(T_Policy);


private:
    ck::FPtrWrapper<TUniquePtr<InternalRegistryType>> _InternalRegistry;
};

template <typename T_ComponentType, typename ... T_Args>
auto FCk_Registry::Add(EntityType InEntity, T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_ComponentType>>
{
    return {};
}

template <typename T_ComponentType, typename ... T_Args>
auto FCk_Registry::Replace(EntityType InEntity,
    T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_ComponentType>>
{
    return {};
}

template <typename T_Component>
auto FCk_Registry::Remove(EntityType InEntity) -> void
{
}

template <typename T_Component>
auto FCk_Registry::Try_Remove(EntityType InEntity) -> void
{
}

template <typename T_Component>
auto FCk_Registry::View(EntityType InEntity)
{
}

template <typename T_Component>
auto FCk_Registry::View(EntityType InEntity) const
{
}

template <typename T_Component, typename T_Comparitor>
auto FCk_Registry::Sort(T_Comparitor InComparitor) -> void
{
}

template <typename T_Component, typename T_Policy, std::enable_if_t<std::is_same_v<T_Policy, ck::policy::All> || std::
    is_same_v<T_Policy, ck::policy::Any>>>
auto FCk_Registry::Has(T_Policy)
{
}
