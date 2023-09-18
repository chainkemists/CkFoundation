#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkHandle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Entity;

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak="CkEcs.Ck_Utils_Handle_UE.Break_Handle"))
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
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

public:
    template <typename T_FragmentType, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename... T_Args>
    auto AddOrGet(T_Args&&... InArgs) -> T_FragmentType&;

    template <typename T_FragmentType, typename T_Func>
    auto Try_Transform(T_Func InFunc) -> void;

    template <typename T_FragmentType, typename... T_Args>
    auto Replace(T_Args&&... InArgs) -> T_FragmentType&;

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
    auto Has() const -> bool;

    template <typename... T_Fragment>
    auto Has_Any() const -> bool;

    template <typename... T_Fragment>
    auto Has_All() const -> bool;

    template <typename T_Fragment>
    auto Get() -> T_Fragment&;

    template <typename T_Fragment>
    auto Get() const -> const T_Fragment&;

public:
    // FCk_Handle is a core concept of this architecture, we are taking some liberties
    // with the operator overloading for the sake of improving readability. Overloading
    // operators like this for non-core types is generally forbidden.
    auto operator*() -> TOptional<FCk_Registry>;
    auto operator*() const -> TOptional<FCk_Registry>;

    auto operator->() -> TOptional<FCk_Registry>;
    auto operator->() const -> TOptional<FCk_Registry>;

public:
    auto IsValid() const -> bool;
    auto Get_ValidHandle(FEntityType::IdType InEntity) const -> ThisType;

    auto Get_Registry() -> FCk_Registry&;
    auto Get_Registry() const -> const FCk_Registry&;

private:
    UPROPERTY()
    FCk_Entity _Entity;

    TOptional<FCk_Registry> _Registry;

public:
    CK_PROPERTY(_Entity);

#if WITH_EDITORONLY_DATA
private:
    UPROPERTY(VisibleAnywhere)
    int32 _EntityID;

    UPROPERTY(VisibleAnywhere)
    int32 _EntityNumber;

    UPROPERTY(VisibleAnywhere)
    int32 _EntityVersion;
#endif
};

auto CKECS_API GetTypeHash(FCk_Handle InHandle) -> uint32;

namespace ck
{
    auto CKECS_API MakeHandle(FCk_Entity InEntity, FCk_Handle InValidHandle) -> FCk_Handle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(FCk_Handle, ck::IsValid_Policy_Default, [&](const FCk_Handle& InHandle)
{
    return InHandle.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Handle, [&]()
{
    return ck::Format(TEXT("[{}][{}]"), InObj._Entity, InObj._Registry);
});

// --------------------------------------------------------------------------------------------------------------------

template <typename T_FragmentType, typename ... T_Args>
auto FCk_Handle::Add(T_Args&&... InArgs) -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_FragmentType>, *this)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ctti::nameof_v<T_FragmentType>, *this)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _Registry->Add<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Handle::
    AddOrGet(T_Args&&... InArgs)
    -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_FragmentType>, *this)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ctti::nameof_v<T_FragmentType>, *this)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _Registry->AddOrGet<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_FragmentType, typename T_Func>
auto
    FCk_Handle::
    Try_Transform(T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Try_Transform Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_FragmentType>, *this)
    { return; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Try_Transform Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ctti::nameof_v<T_FragmentType>, *this)
    { return; }

    _Registry->Try_Transform<T_FragmentType>(_Entity, InFunc);
}

template <typename T_FragmentType, typename ... T_Args>
auto FCk_Handle::Replace(T_Args&&... InArgs) -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_FragmentType>, *this)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ctti::nameof_v<T_FragmentType>, *this)
    {
        static T_FragmentType INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _Registry->Replace<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto FCk_Handle::Remove() -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_Fragment>, *this)
    { return; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ctti::nameof_v<T_Fragment>, *this)
    { return; }

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
        TEXT("Unable to Try_Remove Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ctti::nameof_v<T_Fragment>, *this)
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
auto FCk_Handle::Has() const -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ctti::nameof_v<T_Fragment>, *this)
    { return {}; }

    return _Registry->Has<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto FCk_Handle::Has_Any() const -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_Any query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has_Any<T_Fragment...>(_Entity);
}

template <typename ... T_Fragment>
auto FCk_Handle::Has_All() const -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_All query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has_All<T_Fragment...>(_Entity);
}

template <typename T_Fragment>
auto FCk_Handle::Get() -> T_Fragment&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Get fragment. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static T_Fragment INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

template <typename T_Fragment>
auto FCk_Handle::Get() const -> const T_Fragment&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Get fragment. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static T_Fragment INVALID_Fragment;
        return INVALID_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

// --------------------------------------------------------------------------------------------------------------------
