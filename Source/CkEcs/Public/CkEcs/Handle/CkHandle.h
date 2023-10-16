#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle_Debugging.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkHandle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak="/Script/CkEcs.Ck_Utils_Handle_UE:Break_Handle"))
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

    FCk_Handle(ThisType&& InOther) noexcept;
    FCk_Handle(const ThisType& InOther);

    auto operator=(ThisType InOther) -> ThisType&;
    // auto operator=(ThisType&& InOther) -> ThisType&; // intentionally not implemented

    ~FCk_Handle();

public:
    auto Swap(ThisType& InOther) -> void;

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
    auto View() -> FCk_Registry::RegistryViewType<T_Fragment...>;

    template <typename... T_Fragment>
    auto View() const -> FCk_Registry::ConstRegistryViewType<T_Fragment...>;

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
    void DoUpdate_MapperAndFragments();

private:
    UPROPERTY()
    FCk_Entity _Entity;

    TOptional<FCk_Registry> _Registry;

    // TODO: wrap in pre-processor macro to compile this out in builds such as Shipping
    const struct FEntity_FragmentMapper* _Mapper = nullptr;

public:
    CK_PROPERTY(_Entity);

#if WITH_EDITORONLY_DATA
private:
    UPROPERTY(VisibleAnywhere)
    int32 _EntityID = -1;

    UPROPERTY(VisibleAnywhere)
    int32 _EntityNumber = -1;

    UPROPERTY(VisibleAnywhere)
    int32 _EntityVersion = -1;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class UCk_Handle_FragmentsDebug> _Fragments = nullptr;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

auto CKECS_API GetTypeHash(const FCk_Handle& InHandle) -> uint32;

namespace ck
{
    // Having an overload with FCK_Handle helps usages where a type T that could be an Entity or Handle is used and the code
    // does not know or care. Another case is when we use FCk_Entity in debug builds and FCk_Handle in release
    // builds but the code that uses the handle (or entity) does not care and wants a handle back

    auto CKECS_API MakeHandle(FCk_Entity InEntity, FCk_Handle InValidHandle) -> FCk_Handle;
    auto CKECS_API MakeHandle(FCk_Handle InEntity, FCk_Handle InValidHandle) -> FCk_Handle;

    auto CKECS_API IsValid(FCk_Entity InEntity, FCk_Handle InValidHandle) -> bool;
    auto CKECS_API IsValid(FCk_Handle InEntity, FCk_Handle InValidHandle) -> bool;

    auto CKECS_API GetEntity(FCk_Entity InEntity) -> FCk_Entity;
    auto CKECS_API GetEntity(FCk_Handle InEntity) -> FCk_Entity;
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
auto
    FCk_Handle::
    Add(
        T_Args&&... InArgs)
    -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_FragmentType>, *this)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ck::TypeToString<T_FragmentType>, *this)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    if (NOT Has<T_FragmentType>())
    {
        _Mapper = &_Registry->AddOrGet<FEntity_FragmentMapper>(_Entity);
        _Mapper->Add_FragmentGetter<T_FragmentType>();
    }

    auto& NewFragment = _Registry->Add<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);

    DoUpdate_MapperAndFragments();

    return NewFragment;
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Handle::
    AddOrGet(
        T_Args&&... InArgs)
    -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_FragmentType>, *this)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Add Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ck::TypeToString<T_FragmentType>, *this)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    if (NOT Has<T_FragmentType>())
    {
        _Mapper = &_Registry->AddOrGet<FEntity_FragmentMapper>(_Entity);
        _Mapper->Add_FragmentGetter<T_FragmentType>();
    }

    auto& NewOrExistingFragment = _Registry->AddOrGet<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);

    DoUpdate_MapperAndFragments();

    return NewOrExistingFragment;
}

template <typename T_FragmentType, typename T_Func>
auto
    FCk_Handle::
    Try_Transform(
        T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Try_Transform Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_FragmentType>, *this)
    { return; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Try_Transform Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ck::TypeToString<T_FragmentType>, *this)
    { return; }

    _Registry->Try_Transform<T_FragmentType>(_Entity, InFunc);
}

template <typename T_FragmentType, typename ... T_Args>
auto
    FCk_Handle::
    Replace(
        T_Args&&... InArgs)
    -> T_FragmentType&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_FragmentType>, *this)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Replace Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ck::TypeToString<T_FragmentType>, *this)
    {
        static T_FragmentType Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->Replace<T_FragmentType>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Remove()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_Fragment>, *this)
    { return; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Remove Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ck::TypeToString<T_Fragment>, *this)
    { return; }

    _Registry->Remove<T_Fragment>(_Entity);

    DoUpdate_MapperAndFragments();
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Try_Remove()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Try_Remove Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_Fragment>, *this)
    { return {}; }

    CK_ENSURE_IF_NOT(IsValid(),
        TEXT("Unable to Try_Remove Fragment [{}]. Handle [{}] does NOT have a valid Entity."),
        ck::TypeToString<T_Fragment>, *this)
    { return {}; }

    return _Registry->Remove<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    View()
    -> FCk_Registry::RegistryViewType<T_Fragment...>
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static FRegistryType Invalid_Registry;
        return Invalid_Registry.View<T_Fragment...>();
    }

    return _Registry->View<T_Fragment...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    View() const
    -> FCk_Registry::ConstRegistryViewType<T_Fragment...>
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to prepare a View. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static FRegistryType Invalid_Registry;
        return Invalid_Registry.View<T_Fragment...>();
    }

    return _Registry->View<T_Fragment...>();
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Has() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has query with Fragment [{}]. Handle [{}] does NOT have a valid Registry."),
        ck::TypeToString<T_Fragment>, *this)
    { return {}; }

    return _Registry->Has<T_Fragment>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    Has_Any() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_Any query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has_Any<T_Fragment...>(_Entity);
}

template <typename ... T_Fragment>
auto
    FCk_Handle::
    Has_All() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_All query. Handle [{}] does NOT have a valid Registry."), *this)
    { return {}; }

    return _Registry->Has_All<T_Fragment...>(_Entity);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Get()
    -> T_Fragment&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Get fragment. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

template <typename T_Fragment>
auto
    FCk_Handle::
    Get() const
    -> const T_Fragment&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Get fragment. Handle [{}] does NOT have a valid Registry."), *this)
    {
        static T_Fragment Invalid_Fragment;
        return Invalid_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

// --------------------------------------------------------------------------------------------------------------------

// FEntity_FragmentMapper::ConceptImpl_GetFragment definition here instead of CkHandle_Debugging.h due to a circular
// dependency
template <typename T_Fragment>
auto
    FEntity_FragmentMapper::ConceptImpl_GetFragment<T_Fragment>::
    Get_Fragment(
        const FCk_Handle& InHandle) const
    -> FCk_DebugWrapper*
{
    if (NOT InHandle.Has<T_Fragment>())
    {
        return {};
    }

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        return new TCk_DebugWrapper<T_Fragment>{nullptr};
    }
    else
    {
        return new TCk_DebugWrapper<T_Fragment>{&InHandle.Get<T_Fragment>()};
    }
}

// --------------------------------------------------------------------------------------------------------------------
