#pragma once

#pragma warning(push)
#pragma warning(disable:4583)
#pragma warning(disable:4582)

#define FMT_HEADER_ONLY
#include "CkThirdParty/fmt/include/fmt/xchar.h"
#include "CkThirdParty/fmt/include/fmt/core.h"
#include "CkThirdParty/ctti/include/ctti/nameof.hpp"
#pragma warning(pop)

// --------------------------------------------------------------------------------------------------------------------

// may appear unused but they are required when building Format Defaults
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Debug/CkDebug_Utils.h"

#include "cleantype/details/cleantype_clean.hpp"

#ifndef WITH_ANGELSCRIPT_CK
#define WITH_ANGELSCRIPT_CK 1
#endif

#if WITH_ANGELSCRIPT_CK
#include "CkFormat_AngelScript.h"
#endif

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
        constexpr auto&& ArgsForward(T&& InType);

        template <typename... TArgs>
        auto
            DoFormat(fmt::wformat_string<TArgs...> fmt, TArgs&&... InArgs)
            -> std::basic_string<TCHAR>
        {
            using namespace ck_format_detail;

            return fmt::format(fmt, std::forward<TArgs>(InArgs)...);
        }
    }

    template <typename TString, typename... TArgs>
    auto
    Format(TString InStr, TArgs&&... InArgs)
        -> std::basic_string<TCHAR>
    {
        using namespace ck_format_detail;

        // Notes:
        // - with C++20, fmt has support for compile time strings and verification of the formatting
        // - we cannot directly pass a string literal to fmt::format with the C++20 interface
        // - fmt::format takes a fmt::wformat_string which takes the same arg types as the args we are
        //   attempting to format
        // - we cannot pass the args directly because they have to go through our forwarded which resolves pointers
        // - we feed the converted args to a manually constructed fmt::wformat_string, making sure the
        //   arg types match the ones returned by ArgsForward
        // - this results in a complex looking expression below, although most of it is forwarding and
        //   figuring out the new return types

        return ck_format_detail::DoFormat(
            fmt::wformat_string<decltype(ArgsForward(InArgs))...>(fmt::runtime_format_string<TCHAR>{InStr}),
            std::forward<decltype(ArgsForward(InArgs))>(ArgsForward(std::forward<TArgs>(InArgs)))...);
    }

    template <typename TString, typename... TArgs>
    auto
    Format_ANSI(TString InStr, TArgs&&... InArgs)
        -> std::basic_string<char>
    {
        using namespace ck_format_detail;

        // Notes:
        // - with C++20, fmt has support for compile time strings and verification of the formatting
        // - we cannot directly pass a string literal to fmt::format with the C++20 interface
        // - fmt::format takes a fmt::wformat_string which takes the same arg types as the args we are
        //   attempting to format
        // - we cannot pass the args directly because they have to go through our forwarded which resolves pointers
        // - we feed the converted args to a manually constructed fmt::wformat_string, making sure the
        //   arg types match the ones returned by ArgsForward
        // - this results in a complex looking expression below, although most of it is forwarding and
        //   figuring out the new return types

        const auto& WideStr = ck_format_detail::DoFormat(
            fmt::wformat_string<decltype(ArgsForward(InArgs))...>(fmt::runtime_format_string<TCHAR>{InStr}),
            std::forward<decltype(ArgsForward(InArgs))>(ArgsForward(std::forward<TArgs>(InArgs)))...);
        return std::basic_string<char>(WideStr.begin(), WideStr.end());
    }

    template <typename T>
    auto
    Format_UE(const T& InString)
        -> FString
    {
        return FString{ Format(InString).c_str() };
    }

    template <typename... TArgs>
    auto
    Format_UE(TArgs&&... InArgs)
        -> FString
    {
        return FString{ Format(std::forward<TArgs>(InArgs)...).c_str() };
    }

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T>
    constexpr ctti::detail::cstring TypeToString = ctti::nameof<T>();

    template <typename T>
    auto Get_RuntimeTypeToString() -> FString
    {
        const auto& CleanName = cleantype::clean<T>();
        return FString{static_cast<int32>(CleanName.length()), CleanName.data()};
    }
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_ANSI_TEXT(_Text_, ...)\
    ck::Format_ANSI(TEXT(_Text_), __VA_ARGS__).c_str()

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

#define CK_DEFINE_CUSTOM_FORMATTER_PTR_FORWARDER_NULLPTR_VALIDITY(_Type_, _LambdaToReturnInvalidObj_) \
namespace ck { namespace ck_format_detail {                                                           \
    inline auto&                                                                                      \
    ForwarderForPointers(const _Type_* InObj)                                                         \
    {                                                                                                 \
        if (ck::IsValid(InObj, ck::IsValid_Policy_NullptrOnly{}))                                     \
        { return *InObj; }                                                                            \
                                                                                                      \
        return _LambdaToReturnInvalidObj_();                                                          \
    }                                                                                                 \
}}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DECLARE_CUSTOM_FORMATTER_WITH_DETAILS_INTERNAL(_API_Name_, _Type_, _TypeName_)                        \
namespace fmt {                                                                                                  \
                                                                                                                 \
_API_Name_ auto DoFormat_##_TypeName_(const _Type_& InObj) -> FString;                                           \
_API_Name_ auto DoFormat_Details_##_TypeName_(const _Type_& InObj) -> FString;                                   \
};                                                                                                               \
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(, _Type_,                                                          \
    [](const _Type_& InObj){ return DoFormat_##_TypeName_(InObj); },                                             \
    [](const _Type_& InObj){ return DoFormat_Details_##_TypeName_(InObj); })

#define CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_INTERNAL(_Type_, _TypeName_, _Lambda_, _DetailsLambda_)   \
namespace fmt {                                                                                           \
                                                                                                          \
auto DoFormat_##_TypeName_(const _Type_& InObj) -> FString                                                \
{                                                                                                         \
    return _Lambda_(InObj);                                                                               \
}                                                                                                         \
auto DoFormat_Details_##_TypeName_(const _Type_& InObj) -> FString                                        \
{                                                                                                         \
    return _DetailsLambda_(InObj);                                                                        \
}                                                                                                         \
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DECLARE_CUSTOM_FORMATTER_WITH_DETAILS(_API_Name_, _Type_)\
CK_DECLARE_CUSTOM_FORMATTER_WITH_DETAILS_INTERNAL(_API_Name_, _Type_, _Type_)

#define CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS(_Type_, _Lambda_, _DetailsLambda_)\
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_INTERNAL(_Type_, _Type_, _Lambda_, _DetailsLambda_) \

#define CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_INLINE(_Type_, _Lambda_, _DetailsLambda_)\
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(,_Type_, _Lambda_, _DetailsLambda_)

// --------------------------------------------------------------------------------------------------------------------

#if NOT CK_FORMAT_FORCE_DETAILED
#define CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(_TemplateArgs_, _Type_, _Lambda_, _DetailsLambda_)  \
namespace fmt {                                                                                           \
                                                                                                          \
template<_TemplateArgs_>                                                                                  \
struct formatter<_Type_, TCHAR>                                                                           \
{                                                                                                         \
    bool printDetails = false;                                                                            \
                                                                                                          \
    template <typename ParseContext>                                                                      \
    constexpr auto parse(ParseContext& ctx)                                                               \
    {                                                                                                     \
        auto it = ctx.begin(), end = ctx.end();                                                           \
        if (it == end) return it;                                                                         \
        if (*it == 'd')                                                                                   \
        { printDetails = true; ++it;}                                                                     \
        return it;                                                                                        \
    }                                                                                                     \
                                                                                                          \
    template<typename FormatContext>                                                                      \
    auto format(const _Type_& InObj, FormatContext& ctx)                                                  \
    {                                                                                                     \
        if (NOT printDetails)                                                                             \
        { return fmt::format_to(ctx.out(), TEXT("{}"), _Lambda_(InObj)); }                                \
                                                                                                          \
        return fmt::format_to(ctx.out(), TEXT("{}"), _DetailsLambda_(InObj));                             \
    }                                                                                                     \
};                                                                                                        \
}
#else
#define CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(_TemplateArgs_, _Type_, _Lambda_, _DetailsLambda_)  \
namespace fmt {                                                                                           \
                                                                                                          \
template<_TemplateArgs_>                                                                                  \
struct formatter<_Type_, TCHAR>                                                                           \
{                                                                                                         \
    template <typename ParseContext>                                                                      \
    constexpr auto parse(ParseContext& ctx)                                                               \
    {                                                                                                     \
        return ctx.begin();                                                                               \
    }                                                                                                     \
                                                                                                          \
    template<typename FormatContext>                                                                      \
    auto format(const _Type_& InObj, FormatContext& ctx)                                                  \
    {                                                                                                     \
        return fmt::format_to(ctx.out(), TEXT("{}"), _DetailsLambda_());                                  \
    }                                                                                                     \
};                                                                                                        \
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_FORMATTER_T(_Type_, _Lambda_)                                                    \
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(typename T, _Type_, _Lambda_, _Lambda_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_T(_Type_, _Lambda_, _DetailsLambda_)\
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(typename T, _Type_, _Lambda_, _DetailsLambda_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_DECLARE_CUSTOM_FORMATTER(_API_Name_, _Type_)\
CK_DECLARE_CUSTOM_FORMATTER_WITH_DETAILS(_API_Name_, _Type_)

#define CK_DEFINE_CUSTOM_FORMATTER(_Type_, _Lambda_)\
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS(_Type_, _Lambda_, _Lambda_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_DECLARE_CUSTOM_FORMATTER_NAMESPACE(_API_Name_, _TypeNamespaced_, _Type_)\
CK_DECLARE_CUSTOM_FORMATTER_WITH_DETAILS_INTERNAL(_API_Name_, _TypeNamespaced_, _Type_)

#define CK_DEFINE_CUSTOM_FORMATTER_NAMESPACE(_TypeNamespaced_, _Type_, _Lambda_)\
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_INTERNAL(_TypeNamespaced_, _Type_, _Lambda_, _Lambda_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_FORMATTER_INLINE(_Type_, _Lambda_)\
CK_DEFINE_CUSTOM_FORMATTER_WITH_DETAILS_TARGS(,_Type_, _Lambda_, _Lambda_)


// --------------------------------------------------------------------------------------------------------------------

#define CK_DEFINE_CUSTOM_FORMATTER_ENUM(_Type_)                                                           \
CK_DEFINE_CUSTOM_FORMATTER_INLINE(_Type_, [](const _Type_& InObj)                                         \
{                                                                                                         \
    return UEnum::GetDisplayValueAsText(InObj).ToString();                                                \
});                                                                                                       \
namespace ck                                                                                              \
{                                                                                                         \
                                                                                                          \
template <>                                                                                               \
struct FEnumToString<_Type_>                                                                              \
{                                                                                                         \
    static FName Get() { return TEXT(#_Type_); }                                                          \
    static const TCHAR* Get_AsTChar() { return TEXT(#_Type_); }                                           \
};                                                                                                        \
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkFormat_Defaults.h"

// --------------------------------------------------------------------------------------------------------------------