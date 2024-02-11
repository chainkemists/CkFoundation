#pragma once
#include "CkEcs/Handle/CkHandle.h"

#include "CkHandle_TypeSafe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedHandle>
    auto
        StaticCast(
            const FCk_Handle& InHandle) -> const T_DerivedHandle&;

    template <typename T_DerivedHandle>
    auto
        StaticCast(
            FCk_Handle& InHandle) -> T_DerivedHandle&;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKECS_API FCk_Handle_TypeSafe : public FCk_Handle
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_TypeSafe);

public:
    using FCk_Handle::operator==;
    using FCk_Handle::operator!=;

public:
    FCk_Handle_TypeSafe() = default;
    FCk_Handle_TypeSafe(ThisType&& InOther) noexcept;
    FCk_Handle_TypeSafe(const ThisType& InHandle);

    auto operator=(ThisType InOther) -> ThisType&;
};

// --------------------------------------------------------------------------------------------------------------------
// DO NOT REMOVE THIS STATIC_ASSERT

static_assert
(
    sizeof(FCk_Handle_TypeSafe) == sizeof(FCk_Handle),
    "Type-Safe Handle should be EXACTLY the same size as FCk_Handle"
);

// --------------------------------------------------------------------------------------------------------------------

#define CK_GENERATED_BODY_HANDLE_TYPESAFE(_ClassType_)                                                              \
    CK_GENERATED_BODY(_ClassType_);                                                                                 \
    using FCk_Handle_TypeSafe::operator==;                                                                          \
    using FCk_Handle_TypeSafe::operator!=;                                                                          \
    _ClassType_() = default;                                                                                        \
    _ClassType_(ThisType&& InOther) noexcept : FCk_Handle_TypeSafe(MoveTemp(InOther)) { }                           \
    _ClassType_(const ThisType& InHandle) : FCk_Handle_TypeSafe(InHandle) { }                                       \
    auto operator=( ThisType InOther) -> ThisType& { Swap(InOther); return *this; }

// --------------------------------------------------------------------------------------------------------------------

// we're NOT casting the derived Handle to the base FCk_Handle mainly for perf reasons (avoiding too many conversions when formatting and validating)
#define CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(_HandleType_)                          \
    CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS(_HandleType_,                                             \
    [&]()                                                                                             \
    {                                                                                                 \
        if (ck::IsValid(InObj) && InObj.Has<DEBUG_NAME>())                                            \
        { return ck::Format(TEXT("{}({})"), InObj.Get_Entity(), InObj.Get<DEBUG_NAME>().Get_Name()); }\
                                                                                                      \
        return ck::Format(TEXT("{}"), InObj.Get_Entity());                                            \
    },                                                                                                \
    [&]()                                                                                             \
    {                                                                                                 \
        if (ck::IsValid(InObj) && InObj.Has<DEBUG_NAME>())                                            \
        {                                                                                             \
            return ck::Format(TEXT("{}[{}]({})"),                                                     \
                InObj.Get_Entity(),                                                                   \
                InObj.Get_Registry(),                                                                 \
                InObj.Get<DEBUG_NAME>().Get_Name());                                                  \
        }                                                                                             \
                                                                                                      \
        return ck::Format(TEXT("{}({})"), InObj.Get_Entity(), InObj.Get_Registry());                  \
    });                                                                                               \
    static_assert(sizeof(_HandleType_) == sizeof(FCk_Handle),                                         \
        "Type-Safe Handle should be EXACTLY the same size as FCk_Handle")

// --------------------------------------------------------------------------------------------------------------------

// the ... are the Fragments for the Has check (see other usages)
#define CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(_FeatureName_, _ClassType_, _HandleType_, ...)           \
auto                                                                                                     \
    _ClassType_::                                                                                        \
    Has(                                                                                                 \
        const FCk_Handle& InAbilityEntity)                                                               \
    -> bool                                                                                              \
{                                                                                                        \
    return InAbilityEntity.Has_All<##__VA_ARGS__>();                                                     \
}                                                                                                        \
                                                                                                         \
auto                                                                                                     \
    _ClassType_::                                                                                        \
    Cast(                                                                                                \
        FCk_Handle InHandle,                                                                             \
        ECk_SucceededFailed& OutResult)                                                                  \
    -> _HandleType_                                                                                      \
{                                                                                                        \
    if (ck::Is_NOT_Valid(InHandle))                                                                      \
    {                                                                                                    \
        OutResult = ECk_SucceededFailed::Failed;                                                         \
        return {};                                                                                       \
    }                                                                                                    \
                                                                                                         \
    if (NOT Has(InHandle))                                                                               \
    {                                                                                                    \
        OutResult = ECk_SucceededFailed::Failed;                                                         \
        return {};                                                                                       \
    }                                                                                                    \
                                                                                                         \
    OutResult = ECk_SucceededFailed::Succeeded;                                                          \
    return ck::StaticCast<_HandleType_>(InHandle);                                                       \
}                                                                                                        \
                                                                                                         \
auto                                                                                                     \
    _ClassType_::                                                                                        \
    Conv_HandleTo ##_FeatureName_(                                                                       \
        FCk_Handle InHandle)                                                                             \
    -> _HandleType_                                                                                      \
{                                                                                                        \
    if (ck::Is_NOT_Valid(InHandle))                                                                      \
    { return {}; }                                                                                       \
                                                                                                         \
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have a [{}]. Unable to convert Handle."), \
        InHandle, ck::Get_RuntimeTypeToString<_HandleType_>())                                           \
    { return {}; }                                                                                       \
                                                                                                         \
    return ck::StaticCast<_HandleType_>(InHandle);                                                       \
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedHandle>
    auto
        MakeHandle(
            T_DerivedHandle&& InEntity,
            FCk_Handle InValidHandle)
        -> T_DerivedHandle
    {
        static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>>, "T_DerivedHandle MUST be type-safe Handle");
        static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

        return InEntity;
    }

    template <typename T_DerivedHandle>
    auto
        StaticCast(
            const FCk_Handle& InHandle) -> const T_DerivedHandle&
    {
        static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>> ||
            std::is_same_v<std::remove_cvref_t<T_DerivedHandle>, FCk_Handle>, "T_DerivedHandle MUST be type-safe Handle");
        static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

        return *static_cast<T_DerivedHandle*>(&InHandle);
    }

    template <typename T_DerivedHandle>
    auto
        StaticCast(
            FCk_Handle& InHandle) -> T_DerivedHandle&
    {
        static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>> ||
            std::is_same_v<std::remove_cvref_t<T_DerivedHandle>, FCk_Handle>, "T_DerivedHandle MUST be type-safe Handle");
        static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

        return *static_cast<T_DerivedHandle*>(&InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
