#pragma once
#include "CkEcs/Handle/CkHandle.h"

#include "CkHandle_TypeSafe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedHandle, typename T_HandleType>
    requires(std::is_base_of_v<FCk_Handle, std::remove_cvref_t<T_HandleType>>)
    auto
        StaticCast(
            T_HandleType&& InHandle) -> T_DerivedHandle;
}

USTRUCT()
struct CKECS_API FCk_Handle_TypeSafe : public FCk_Handle
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_TypeSafe);

public:
    using FCk_Handle::operator==;
    using FCk_Handle::operator!=;
    using FCk_Handle::operator<;

public:
    FCk_Handle_TypeSafe() = default;
    FCk_Handle_TypeSafe(ThisType&& InOther) noexcept;
    FCk_Handle_TypeSafe(const ThisType& InHandle);

    auto operator=(ThisType InOther) -> ThisType&;

protected:
    FCk_Handle_TypeSafe(const FCk_Handle& InOther);
};

template<>
struct TStructOpsTypeTraits<FCk_Handle_TypeSafe> : public TStructOpsTypeTraitsBase2<FCk_Handle_TypeSafe>
{
    enum
    { WithNetSerializer = true };
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
    template <typename T_DerivedHandle, typename T_HandleType>                                                      \
    requires(std::is_base_of_v<FCk_Handle, std::remove_cvref_t<T_HandleType>>)                                      \
    friend auto                                                                                                     \
        ck::StaticCast(                                                                                             \
            T_HandleType&& InHandle) -> T_DerivedHandle;                                                            \
    CK_GENERATED_BODY(_ClassType_);                                                                                 \
    using FCk_Handle_TypeSafe::operator==;                                                                          \
    using FCk_Handle_TypeSafe::operator!=;                                                                          \
    using FCk_Handle_TypeSafe::operator<;                                                                           \
    _ClassType_() = default;                                                                                        \
    _ClassType_(ThisType&& InOther) noexcept : FCk_Handle_TypeSafe(MoveTemp(InOther)) { }                           \
    _ClassType_(const ThisType& InHandle) : FCk_Handle_TypeSafe(InHandle) { }                                       \
    auto operator=( ThisType InOther) -> ThisType& { Swap(InOther); return *this; }                                 \
    private:                                                                                                        \
    _ClassType_(const FCk_Handle& InOther) : FCk_Handle_TypeSafe(InOther) { }

// --------------------------------------------------------------------------------------------------------------------

// we're NOT casting the derived Handle to the base FCk_Handle mainly for perf reasons (avoiding too many conversions when formatting and validating)
#define CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(_HandleType_)                                      \
    CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS(_HandleType_,                                                         \
    [&]()                                                                                                         \
    {                                                                                                             \
        return ck::Format(TEXT("{}({})"), InObj.Get_Entity(), InObj.Get_DebugName());                             \
    },                                                                                                            \
    [&]()                                                                                                         \
    {                                                                                                             \
        return ck::Format(TEXT("{}[{}]({})"), InObj.Get_Entity(), InObj.Get_Registry(), InObj.Get_DebugName());   \
    });                                                                                                           \
    static_assert(sizeof(_HandleType_) == sizeof(FCk_Handle),                                                     \
        "Type-Safe Handle should be EXACTLY the same size as FCk_Handle");                                        \
\
    template<> \
    struct TStructOpsTypeTraits<_HandleType_> : public TStructOpsTypeTraitsBase2<_HandleType_> \
    { \
        enum \
        { WithNetSerializer = true }; \
    };

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(_HandleType_)                                                 \
    template <typename T>                                                                                \
    static auto CastChecked(                                                                             \
        T&& InHandle)                                                                                    \
        -> _HandleType_                                                                                  \
    {                                                                                                    \
        if (ck::Is_NOT_Valid(InHandle, ck::IsValid_Policy_IncludePendingKill{}))                         \
        { return {}; }                                                                                   \
                                                                                                         \
        CK_ENSURE_IF_NOT(Has(InHandle),                                                                  \
            TEXT("Handle [{}] does NOT have a [{}]. Unable to convert Handle."),                         \
            InHandle,                                                                                    \
            ck::Get_RuntimeTypeToString<_HandleType_>())                                                 \
        { return {}; }                                                                                   \
                                                                                                         \
        return ck::StaticCast<_HandleType_>(std::forward<T>(InHandle));                                  \
    }                                                                                                    \
                                                                                                         \
    template <typename T>                                                                                \
    static auto Cast(                                                                                    \
        T&& InHandle)                                                                                    \
        -> _HandleType_                                                                                  \
    {                                                                                                    \
        if (ck::Is_NOT_Valid(InHandle, ck::IsValid_Policy_IncludePendingKill{}))                         \
        { return {}; }                                                                                   \
                                                                                                         \
        if (NOT Has(InHandle))                                                                           \
        { return {}; }                                                                                   \
                                                                                                         \
        return ck::StaticCast<_HandleType_>(std::forward<T>(InHandle));                                  \
    }                                                                                                    \

// NOTES:
// - the ... are the Fragments for the Has check (see other usages)
// - the FCk_Handle _should_ be passed by ref but isn't because passing by ref makes BlueprintAutoCast to not work
#define CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(_ClassType_, _HandleType_, ...)                          \
auto                                                                                                     \
    _ClassType_::                                                                                        \
    Has(                                                                                                 \
        const FCk_Handle& InAbilityEntity)                                                               \
    -> bool                                                                                              \
{                                                                                                        \
    return InAbilityEntity.Has_All<EXPAND_ALL(__VA_ARGS__)>();                                           \
}                                                                                                        \
                                                                                                         \
auto                                                                                                     \
    _ClassType_::                                                                                        \
    DoCast(                                                                                              \
        FCk_Handle& InHandle,                                                                            \
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
    return ck::StaticCast<EXPAND(_HandleType_)>(InHandle);                                               \
}                                                                                                        \
                                                                                                         \
auto                                                                                                     \
    _ClassType_::                                                                                        \
    DoCastChecked(                                                                                       \
        FCk_Handle InHandle)                                                                             \
    -> _HandleType_                                                                                      \
{                                                                                                        \
    if (ck::Is_NOT_Valid(InHandle))                                                                      \
    { return {}; }                                                                                       \
                                                                                                         \
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have a [{}]. Unable to convert Handle."), \
        InHandle, ck::Get_RuntimeTypeToString<EXPAND(_HandleType_)>())                                   \
    { return {}; }                                                                                       \
                                                                                                         \
    return ck::StaticCast<EXPAND(_HandleType_)>(InHandle);                                               \
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace details
    {
        template <typename T_DerivedHandle>
        auto
            StaticCast(
                const FCk_Handle& InHandle) -> const T_DerivedHandle&
        {
            static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>> ||
                std::is_same_v<std::remove_cvref_t<T_DerivedHandle>, FCk_Handle>, "T_DerivedHandle MUST be type-safe Handle");
            static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

            return *static_cast<const T_DerivedHandle*>(&InHandle);
        }

        template <typename T_DerivedHandle>
        auto
            StaticCast(
                FCk_Handle& InHandle) -> T_DerivedHandle&
        {
            static_assert(std::is_lvalue_reference_v<T_DerivedHandle&&>, "Only lvalue references are allowed");
            static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>> ||
                std::is_same_v<std::remove_cvref_t<T_DerivedHandle>, FCk_Handle>, "T_DerivedHandle MUST be type-safe Handle");
            static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

            return *static_cast<T_DerivedHandle*>(&InHandle);
        }

        template <typename T_DerivedHandle, typename T_Handle>
        using StaticCastReturnType = decltype(StaticCast<T_DerivedHandle>(T_Handle{}));
    }

    // This mainly exists to allow generic functions to work with type-safe handles. Originally, MakeHandle was used to get
    // a Handle from an Entity. Generic functions needed to work with Entities that could be FCk_Entity or FCk_Handle, so
    // MakeHandle was overloaded for that purpose (see CkHandle.h). This is another overload to have those generic functions
    // work with type-safe Handles as well (which is why it's essentially a pass-through)
    template <typename T_DerivedHandle>
    auto
        MakeHandle(
            T_DerivedHandle&& InEntity,
            FCk_Handle)
        -> T_DerivedHandle
    {
        static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>>, "T_DerivedHandle MUST be type-safe Handle");
        static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

        return InEntity;
    }

    template <typename T_DerivedHandle, typename T_HandleType>
    requires(std::is_base_of_v<FCk_Handle, std::remove_cvref_t<T_HandleType>>)
    auto
        StaticCast(
            T_HandleType&& InHandle) -> T_DerivedHandle
    {
        static_assert(std::is_base_of_v<FCk_Handle_TypeSafe, std::remove_cvref_t<T_DerivedHandle>> ||
            std::is_same_v<std::remove_cvref_t<T_DerivedHandle>, FCk_Handle>, "T_DerivedHandle MUST be type-safe Handle");
        static_assert(sizeof(T_DerivedHandle) == sizeof(FCk_Handle), "T_DerivedHandle MUST be the same size as FCk_Handle");

        return T_DerivedHandle{InHandle};
    }
}

// --------------------------------------------------------------------------------------------------------------------
