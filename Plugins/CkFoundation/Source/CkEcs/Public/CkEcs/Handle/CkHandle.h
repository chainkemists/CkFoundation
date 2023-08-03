#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkMacros/CkMacros.h"

#include "CkHandle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Entity;

USTRUCT(BlueprintType)
struct CKECS_API FCk_Handle
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle);
    CK_ENABLE_CUSTOM_FORMATTER(FCk_Handle);

public:
    using FEntityType = FCk_Entity;
    using FRegistryType = FCk_Registry;

public:
    FCk_Handle() = default;
    FCk_Handle(FEntityType InEntity, const FRegistryType& InRegistry);

public:
    template <typename T_FragmentType, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>;

    template <typename T_FragmentType, typename... T_Args>
    auto Replace(T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>;

    template <typename T_Fragment>
    auto Remove() -> void;

    template <typename T_Fragment>
    auto Try_Remove() -> void;

    template <typename... T_Fragment>
    auto View();

    template <typename... T_Fragment>
    auto View() const;

public:
    template <typename T_Fragment>
    auto Has() -> bool;

    template <typename... T_Fragment>
    auto Has_Any() -> bool;

    template <typename... T_Fragment>
    auto Has_All() -> bool;

public:
    auto IsValid() -> bool;

private:
    FEntityType _Entity;
    TOptional<FCk_Registry> _Registry;
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Handle, [&]()
{
    return ck::Format(TEXT("[{}][{}]"), InObj._Entity, InObj._Registry);
});

// --------------------------------------------------------------------------------------------------------------------

template <typename T_FragmentType, typename ... T_Args>
auto FCk_Handle::Add(T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_FragmentType>, *this)
    { return {}; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Entity."), *this)
    { return {}; }

    return _Registry->Add<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_FragmentType, typename ... T_Args>
auto FCk_Handle::Replace(T_Args&&... InArgs) -> TOptional<std::reference_wrapper<T_FragmentType>>
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_FragmentType>, *this)
    { return {}; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] does NOT have a valid Entity."), *this)
    { return {}; }

    return _Registry->Replace<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto FCk_Handle::Remove() -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_Fragment>, *this)
    { return {}; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] does NOT have a valid Entity."), *this)
    { return {}; }

    return _Registry->Remove<T_Fragment>(_Entity);
}

template <typename T_Fragment>
auto FCk_Handle::Try_Remove() -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Try_Remove Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_Fragment>, *this)
    { return {}; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Try_Remove Fragment [{}]. Handle [{}] does NOT have a valid Entity."), *this)
    { return {}; }

    return _Registry->Remove<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto FCk_Handle::View()
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->View<T_Fragment...>(_Entity);
}

template <typename ... T_Fragment>
auto FCk_Handle::View() const
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->View<T_Fragment...>(_Entity);
}

template <typename T_Fragment>
auto FCk_Handle::Has() -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_Fragment>, *this)
    { return {}; }

    return _Registry->Has<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto FCk_Handle::Has_Any() -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_Any query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto FCk_Handle::Has_All() -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_All query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has<T_Fragment>(_Entity);
}

// --------------------------------------------------------------------------------------------------------------------
