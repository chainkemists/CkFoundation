#pragma once
#include "CkEcs/Handle/CkHandle.h"

#include "CkHandle_TypeSafe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
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
    FCk_Handle_TypeSafe(EntityType InEntity, const RegistryType& InRegistry);

    FCk_Handle_TypeSafe(ThisType&& InOther) noexcept;
    FCk_Handle_TypeSafe(const ThisType& InHandle);
    explicit FCk_Handle_TypeSafe(const FCk_Handle& InHandle);

    auto operator=(const ThisType& InOther) -> ThisType&;
    auto operator=(ThisType&& InOther) noexcept -> ThisType&;
    auto operator=(const FCk_Handle& InOther) -> ThisType&;
};

// --------------------------------------------------------------------------------------------------------------------

#define CK_GENERATED_BODY_HANDLE_TYPESAFE(_ClassType_)                                                              \
    CK_GENERATED_BODY(_ClassType_);                                                                                 \
    using FCk_Handle_TypeSafe::operator==;                                                                          \
    using FCk_Handle_TypeSafe::operator!=;                                                                          \
    _ClassType_() = default;                                                                                        \
    _ClassType_(EntityType InEntity, const RegistryType& InRegistry) : FCk_Handle_TypeSafe(InEntity, InRegistry) { }\
    _ClassType_(ThisType&& InOther) noexcept : FCk_Handle_TypeSafe(MoveTemp(InOther)) { }                           \
    _ClassType_(const ThisType& InHandle) : FCk_Handle_TypeSafe(InHandle) { }                                       \
    explicit _ClassType_(const FCk_Handle& InHandle) : FCk_Handle_TypeSafe(InHandle) { }                            \
    auto operator=(const ThisType& InOther) -> ThisType& { Super::operator=(InOther); return *this; } \
    auto operator=(ThisType&& InOther) noexcept -> ThisType& { _Entity = InOther._Entity; _Registry = InOther._Registry; _Mapper = InOther._Mapper; return *this; } \
    auto operator=(const FCk_Handle& InOther) -> ThisType& { Super::operator=(InOther); return *this; }

// --------------------------------------------------------------------------------------------------------------------

// we're NOT casting the derived Handle to the base FCk_Handle mainly for perf reasons (avoiding too many conversions when formatting)
#define CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(_HandleType_)                                                    \
    CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS(_HandleType_,                                                                       \
    [&]()                                                                                                                       \
    {                                                                                                                           \
        if (ck::IsValid(InObj) && InObj.Has<DEBUG_NAME>())                                                                      \
        { return ck::Format(TEXT("{}({})"), InObj.Get_Entity(), InObj.Get<DEBUG_NAME>().Get_Name()); }                          \
                                                                                                                                \
        return ck::Format(TEXT("{}"), InObj.Get_Entity());                                                                      \
    },                                                                                                                          \
    [&]()                                                                                                                       \
    {                                                                                                                           \
        if (ck::IsValid(InObj) && InObj.Has<DEBUG_NAME>())                                                                      \
        { return ck::Format(TEXT("{}[{}]({})"), InObj.Get_Entity(), InObj.Get_Registry(), InObj.Get<DEBUG_NAME>().Get_Name()); }\
                                                                                                                                \
        return ck::Format(TEXT("{}({})"), InObj.Get_Entity(), InObj.Get_Registry());                                            \
    })

// --------------------------------------------------------------------------------------------------------------------

// the ... are the Fragments for the Has check (see other usages)
#define CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(_FeatureName_, _ClassType_, _HandleType_, ...)                                          \
auto                                                                                                                                    \
    _ClassType_::                                                                                                                       \
    Has(                                                                                                                                \
        const FCk_Handle& InAbilityEntity)                                                                                              \
    -> bool                                                                                                                             \
{                                                                                                                                       \
    return InAbilityEntity.Has_All<##__VA_ARGS__>();                                                                                    \
}                                                                                                                                       \
                                                                                                                                        \
auto                                                                                                                                    \
    _ClassType_::                                                                                                                       \
    Cast(                                                                                                                               \
        const FCk_Handle&    InHandle,                                                                                                  \
        ECk_SucceededFailed& OutResult)                                                                                                 \
    -> _HandleType_                                                                                                                     \
{                                                                                                                                       \
    if (NOT Has(InHandle))                                                                                                              \
    {                                                                                                                                   \
        OutResult = ECk_SucceededFailed::Failed;                                                                                        \
        return {};                                                                                                                      \
    }                                                                                                                                   \
                                                                                                                                        \
    OutResult = ECk_SucceededFailed::Succeeded;                                                                                         \
    return ck::Cast<_HandleType_>(InHandle);                                                                                            \
}                                                                                                                                       \
                                                                                                                                        \
auto                                                                                                                                    \
    _ClassType_::                                                                                                                       \
    Conv_HandleTo ##_FeatureName_(                                                                                                      \
        const FCk_Handle& InHandle)                                                                                                     \
    -> _HandleType_                                                                                                                     \
{                                                                                                                                       \
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have a [{}]. Unable to convert Handle."), InHandle, TEXT(#_FeatureName_))\
    { return {}; }                                                                                                                      \
                                                                                                                                        \
    return ck::Cast<_HandleType_>(InHandle);                                                                                            \
}

// --------------------------------------------------------------------------------------------------------------------
