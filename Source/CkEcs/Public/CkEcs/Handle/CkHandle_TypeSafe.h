#pragma once
#include "CkEcs/Handle/CkHandle.h"

#include "CkHandle_TypeSafe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Why not inherit from FCk_Handle? Solely to provide a way, in Blueprints, to convert from a type-safe Handle to a regular
// handle simply by breaking up the struct.
//
// Why not inherit and provide a custom break or a function to convert? To reduce
// some boilerplate in our utils library
//
// Why not include FCk_Handle AND inherit from FCk_Handle to eliminate the boilerplate forwarding functions? Because then
// the Handle copy will be more expensive. In Shipping it might not matter too much since Handles are very cheap to copy
// but in Debug builds Handle copies are expensive due to Debugging information that's included
USTRUCT(BlueprintType)
struct CKECS_API FCk_Handle_TypeSafe
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_TypeSafe);

public:
    template <typename T_Fragment, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_Fragment& ;

    template <typename T_Fragment, typename T_ValidationPolicy, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_Fragment& ;

    template <typename T_Fragment, typename... T_Args>
    auto AddOrGet(T_Args&&... InArgs) -> T_Fragment& ;

    template <typename T_Fragment, typename T_Func>
    auto Try_Transform(T_Func InFunc) -> void ;

    template <typename T_Fragment, typename... T_Args>
    auto Replace(T_Args&&... InArgs) -> T_Fragment& ;

    template <typename T_Fragment>
    auto Remove() -> void ;

    template <typename T_Fragment>
    auto Try_Remove() -> void ;

    template <typename... T_Fragment>
    auto View() -> FCk_Registry::RegistryViewType<T_Fragment...> ;

    template <typename... T_Fragment>
    auto View() const -> FCk_Registry::ConstRegistryViewType<T_Fragment...> ;

public:
    template <typename T_Fragment>
    auto Has() const -> bool ;

    template <typename... T_Fragment>
    auto Has_Any() const -> bool ;

    template <typename... T_Fragment>
    auto Has_All() const -> bool ;

    template <typename T_Fragment>
    auto Get() -> T_Fragment& ;

    template <typename T_Fragment>
    auto Get() const -> const T_Fragment& ;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() -> T_Fragment& ;

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() const -> const T_Fragment& ;

public:
    auto operator*() -> TOptional<FCk_Registry> ;
    auto operator*() const -> TOptional<FCk_Registry> ;

    auto operator->() -> TOptional<FCk_Registry> ;
    auto operator->() const -> TOptional<FCk_Registry> ;

public:
    auto IsValid(ck::IsValid_Policy_Default) const -> bool ;
    auto IsValid(ck::IsValid_Policy_IncludePendingKill) const -> bool ;

    auto Orphan() const -> bool ;
    auto Get_ValidHandle(FCk_Entity::IdType InEntity) const -> FCk_Handle ;

    auto Get_Registry() -> FCk_Registry& ;
    auto Get_Registry() const -> const FCk_Registry& ;

private:
    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FCk_Handle _RawHandle;

public:
    CK_PROPERTY_GET(_RawHandle);
    CK_PROPERTY_GET_NON_CONST(_RawHandle);
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_WrappedHandle, class = std::enable_if_t<std::is_base_of_v<FCk_Handle_TypeSafe, T_WrappedHandle>>>
auto
    operator==(const FCk_Handle& InHandle, const T_WrappedHandle& InHandle_TypeSafe) -> bool
{ return InHandle == InHandle_TypeSafe.Get_RawHandle(); }

template <typename T_WrappedHandle, class = std::enable_if_t<std::is_base_of_v<FCk_Handle_TypeSafe, T_WrappedHandle>>>
auto
    operator==(const T_WrappedHandle& InHandle_TypeSafe, const FCk_Handle& InHandle) -> bool
{ return InHandle_TypeSafe.Get_RawHandle() == InHandle; }

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(_HandleType_)                                        \
    CK_DEFINE_CUSTOM_IS_VALID(_HandleType_, ck::IsValid_Policy_Default, [&](const _HandleType_& InHandle)           \
    {                                                                                                               \
        return InHandle.IsValid(ck::IsValid_Policy_Default{});                                                      \
    });                                                                                                             \
                                                                                                                    \
    CK_DEFINE_CUSTOM_IS_VALID(_HandleType_, ck::IsValid_Policy_IncludePendingKill, [&](const _HandleType_& InHandle)\
    {                                                                                                               \
        return InHandle.IsValid(ck::IsValid_Policy_IncludePendingKill{});                                           \
    });                                                                                                             \
    CK_DEFINE_CUSTOM_FORMATTER(_HandleType_, [&]() { return ck::Format(TEXT("{}"), InObj.Get_RawHandle()); })

// --------------------------------------------------------------------------------------------------------------------

// the ... are the Fragments for the Has check (see other usages)
#define CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(_FeatureName_, _ClassType_, _HandleType_, ...)                                        \
auto                                                                                                                                  \
    _ClassType_::                                                                                                                     \
    Has(                                                                                                                              \
        const FCk_Handle& InAbilityEntity)                                                                                            \
    -> bool                                                                                                                           \
{                                                                                                                                     \
    return InAbilityEntity.Has_All<##__VA_ARGS__>();                                                                                  \
}                                                                                                                                     \
                                                                                                                                      \
auto                                                                                                                                  \
    _ClassType_::                                                                                                                     \
    Cast(                                                                                                                             \
        const FCk_Handle&    InHandle,                                                                                                \
        ECk_SucceededFailed& OutResult)                                                                                               \
    -> _HandleType_                                                                                                                   \
{                                                                                                                                     \
    if (Has(InHandle))                                                                                                                \
    {                                                                                                                                 \
        OutResult = ECk_SucceededFailed::Failed;                                                                                      \
        return {};                                                                                                                    \
    }                                                                                                                                 \
                                                                                                                                      \
    OutResult = ECk_SucceededFailed::Succeeded;                                                                                       \
    return ck::Cast<_HandleType_>(InHandle);                                                                                          \
}                                                                                                                                     \
                                                                                                                                      \
auto                                                                                                                                  \
    _ClassType_::                                                                                                                     \
    Conv_HandleTo ##_FeatureName_(                                                                                                    \
        const FCk_Handle& InHandle)                                                                                                   \
    -> _HandleType_                                                                                                                   \
{                                                                                                                                     \
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have a {}. Unable to convert Handle."), InHandle, TEXT(#_FeatureName_))\
    { return {}; }                                                                                                                    \
                                                                                                                                      \
    return ck::Cast<_HandleType_>(InHandle);                                                                                          \
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle_TypeSafe::
    Add(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    return _RawHandle.Add<T_Fragment>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename T_ValidationPolicy, typename ... T_Args>
auto
    FCk_Handle_TypeSafe::
    Add(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    return _RawHandle.Add<T_Fragment, T_ValidationPolicy>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle_TypeSafe::
    AddOrGet(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    return _RawHandle.AddOrGet<T_Fragment>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename T_Func>
auto
    FCk_Handle_TypeSafe::
    Try_Transform(
        T_Func InFunc)
    -> void
{
    _RawHandle.Try_Transform(InFunc);
}

template <typename T_Fragment, typename ... T_Args>
auto
    FCk_Handle_TypeSafe::
    Replace(
        T_Args&&... InArgs)
    -> T_Fragment&
{
    return _RawHandle.Replace<T_Fragment>(std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Remove()
    -> void
{
    _RawHandle.Remove<T_Fragment>();
}

template <typename T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Try_Remove()
    -> void
{
    _RawHandle.Try_Remove<T_Fragment>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle_TypeSafe::
    View()
    -> FCk_Registry::RegistryViewType<T_Fragment...>
{
    return _RawHandle.View<T_Fragment...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle_TypeSafe::
    View() const
    -> FCk_Registry::ConstRegistryViewType<T_Fragment...>
{
    return _RawHandle.View<T_Fragment...>();
}

template <typename T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Has() const
    -> bool
{
    return _RawHandle.Has<T_Fragment>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Has_Any() const
    -> bool
{
    return _RawHandle.Has_Any<T_Fragment...>();
}

template <typename ... T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Has_All() const
    -> bool
{
    return _RawHandle.Has_All<T_Fragment...>();
}

template <typename T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Get()
    -> T_Fragment&
{
    return _RawHandle.Get<T_Fragment>();
}

template <typename T_Fragment>
auto
    FCk_Handle_TypeSafe::
    Get() const
    -> const T_Fragment&
{
    return _RawHandle.Get<T_Fragment>();
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle_TypeSafe::
    Get()
    -> T_Fragment&
{
    return _RawHandle.Get<T_Fragment, T_ValidationPolicy>();
}

template <typename T_Fragment, typename T_ValidationPolicy>
auto
    FCk_Handle_TypeSafe::
    Get() const
    -> const T_Fragment&
{
    return _RawHandle.Get<T_Fragment, T_ValidationPolicy>();
}

// --------------------------------------------------------------------------------------------------------------------
