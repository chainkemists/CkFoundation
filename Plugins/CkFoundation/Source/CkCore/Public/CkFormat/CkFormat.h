#pragma once

#pragma warning(push)
#pragma warning(disable:4583)
#pragma warning(disable:4582)

#define FMT_HEADER_ONLY
#include "CkThirdParty/fmt/include/fmt/xchar.h"
#include "CkThirdParty/fmt/include/fmt/format.h"
#pragma warning(pop)

// --------------------------------------------------------------------------------------------------------------------

// may appear unused but they are required when building Format Defaults
#include "CkValidation/CkIsValid.h"
#include "CkDebug/CkDebug_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

template <typename T_Enum>
struct FEnumToString;

}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_format_detail
    {
        template <typename T>
        auto&& ArgsForward(T&& InType)
        {
            using decayed_t = std::decay_t<T>;
            using original_t = std::remove_const_t<decayed_t>;

            if constexpr (std::is_pointer_v<decayed_t> && NOT std::is_void_v<std::remove_pointer_t<std::remove_cv_t<original_t>>> && NOT std::is_same_v<decayed_t, const wchar_t*>)
            {
                return ForwarderForPointers(InType);
            }
            else
            {
                return InType;
            }
        }
    }

    template <typename T, typename... TArgs>
    auto
    Format(const T& InString, TArgs&&... InArgs)
        -> std::basic_string<TCHAR>
    {
        return fmt::format(InString, ck_format_detail::ArgsForward(InArgs)...);
        //return fmt::format(TEXT("{}"), std::forward<TArgs>(InArgs)...);
    }

    template <typename T>
    auto
    Format_UE(const T& InString)
        -> FString
    {
        return FString{ Format(InString).c_str() };
    }

    template <typename T, typename... TArgs>
    auto
    Format_UE(const T& InString, TArgs&&... InArgs)
        -> FString
    {
        return FString{ Format(InString, std::forward<TArgs>(InArgs)...).c_str() };
    }

}

// --------------------------------------------------------------------------------------------------------------------

// convenience macro to enable the formatter to access private members of a class/struct
#define CK_ENABLE_CUSTOM_FORMATTER(_Class_)                  \
    friend struct fmt::formatter<_Class_, TCHAR>

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER(_Type_, _LambdaToReturnInvalidObj_)\
namespace ck { namespace ck_format_detail {                                         \
    inline auto&                                                                    \
    ForwarderForPointers(const _Type_* InObj)                                       \
    {                                                                               \
        if (ck::IsValid(InObj))                                                     \
        { return *InObj; }                                                          \
                                                                                    \
        return _LambdaToReturnInvalidObj_();                                        \
    }                                                                               \
}}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_FORMATTER_T(_Type_, _Lambda_)                              \
namespace fmt {                                                                     \
                                                                                    \
template<typename T>                                                                \
struct formatter<_Type_, TCHAR>                                                     \
{                                                                                   \
    template <typename ParseContext>                                                \
    constexpr auto parse(ParseContext& ctx)                                         \
    { return ctx.begin(); }                                                         \
                                                                                    \
    template<typename FormatContext>                                                \
    auto format(const _Type_& InObj, FormatContext& ctx)                            \
    {                                                                               \
        return fmt::format_to(ctx.out(), TEXT("{}"), _Lambda_());                   \
    }                                                                               \
};                                                                                  \
}

#define CK_DEFINE_CUSTOM_FORMATTER(_Type_, _Lambda_)                                \
namespace fmt {                                                                     \
                                                                                    \
template<>                                                                          \
struct formatter<_Type_, TCHAR>                                                     \
{                                                                                   \
    template <typename ParseContext>                                                \
    constexpr auto parse(ParseContext& ctx)                                         \
    { return ctx.begin(); }                                                         \
                                                                                    \
    template<typename FormatContext>                                                \
    auto format(const _Type_& InObj, FormatContext& ctx)                            \
    {                                                                               \
        return fmt::format_to(ctx.out(), TEXT("{}"), _Lambda_());                   \
    }                                                                               \
};                                                                                  \
}

#define CK_DEFINE_CUSTOM_FORMATTER_ENUM(_Type_)                                     \
CK_DEFINE_CUSTOM_FORMATTER(_Type_, [&]()                                            \
{                                                                                   \
    return UEnum::GetDisplayValueAsText(InObj).ToString();                          \
});                                                                                 \
namespace ck                                                                        \
{                                                                                   \
                                                                                    \
template <>                                                                         \
struct FEnumToString<_Type_>                                                        \
{                                                                                   \
    static FName Get() { return TEXT(#_Type_); }                                    \
    static const TCHAR* Get_AsTChar() { return TEXT(#_Type_); }                     \
};                                                                                  \
}

#include "CkFormat_Defaults.h"
