#pragma once

#include "CkCore/Build/CkBuild_Macros.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------
namespace ck::ensure
{
    CKCORE_API auto Ensure_Impl(
        const FString& InMessage,
        const FString& InExpressionText,
        const FName& InFile,
        int32 InLine,
        bool& OutBreakInCode,
        bool& OutBreakInScript) -> void;

    CKCORE_API auto Do_BreakInScript() -> void;
    CKCORE_API auto Do_Push_EnsureIsFromScript() -> void;
    CKCORE_API auto Do_Pop_EnsureIsFromScript() -> void;
}

#define CK_ENSURE(InExpression, InString, ...)                                                                                             \
[&]() -> bool                                                                                                                              \
{                                                                                                                                          \
    const auto ExpressionResult = InExpression;                                                                                            \
                                                                                                                                           \
    if (LIKELY(ExpressionResult))                                                                                                          \
    { return true; }                                                                                                                       \
                                                                                                                                           \
    const auto& Message = ck::Format_UE(InString, ##__VA_ARGS__);                                                                          \
    const auto& ExpressionText = TEXT(#InExpression);                                                                                      \
    auto ShouldBreakInCode = false;                                                                                                        \
    auto ShouldBreakInScript = false;                                                                                                      \
    ck::ensure::Ensure_Impl(Message, ExpressionText, __FILE__, __LINE__, ShouldBreakInCode, ShouldBreakInScript);                          \
    if (ShouldBreakInCode)                                                                                                                 \
    {                                                                                                                                      \
        ensureAlwaysMsgf(false, TEXT("[DEBUG BREAK HIT]"));                                                                                \
        if (ShouldBreakInScript)                                                                                                           \
        {                                                                                                                                  \
            ck::ensure::Do_BreakInScript();                                                                                                \
        }                                                                                                                                  \
    }                                                                                                                                      \
    return false;                                                                                                                          \
}()

#if CK_DISABLE_ENSURE_CHECKS
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if constexpr(false)
#elif CK_DISABLE_ENSURE_DEBUGGING
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if (NOT InExpression)
#else
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if (NOT CK_ENSURE(InExpression, InFormat, ##__VA_ARGS__))
#endif

#define CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(InWorldContextObject)\
if(\
NOT [InWorldContextObject]()\
{\
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("Invalid World Context object used to validate UWorld"))\
    { return false; }\
\
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject->GetWorld()), TEXT("Invalid UWorld"))\
    { return false; }\
\
    return true;\
}())

#define CK_TRIGGER_ENSURE(InString, ...)\
CK_ENSURE(false, InString, ##__VA_ARGS__)

// Technically, the same as CK_ENSURE(...), but semantically it's different i.e. we WANT the ensure to be triggered by
// an expression that is not really part of the ensure itself
#define CK_TRIGGER_ENSURE_IF(InExpression, InString, ...)\
if(InExpression) { CK_TRIGGER_ENSURE(InString, ##__VA_ARGS__); }

#define CK_INVALID_ENUM(InEnsure)\
CK_TRIGGER_ENSURE(TEXT("Encountered an invalid value for Enum [{}]"), InEnsure)

// --------------------------------------------------------------------------------------------------------------------