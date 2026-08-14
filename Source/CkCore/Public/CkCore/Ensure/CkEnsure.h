#pragma once

#include "CkCore/Build/CkBuild_Macros.h"
#include "CkCore/Macros/CkMacros.h"

#include <HAL/PlatformMisc.h> // for PLATFORM_BREAK() + FPlatformMisc::IsDebuggerPresent()

struct FCk_Utils_EditorOnly_PushNewEditorMessage_Params;

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

    // Returns true when the CALLER must trigger the debug break itself; a script break, if requested,
    // is handled internally before returning.
    CKCORE_API auto Do_HandleFail(
        const FString& InMessage,
        const FString& InExpressionText,
        const FName& InFile,
        int32 InLine) -> bool;

    CKCORE_API auto Do_BreakInScript() -> void;
    CKCORE_API auto Get_IsEnsureFromScript() -> bool;
    CKCORE_API auto Do_Push_EnsureIsFromScript() -> void;
    CKCORE_API auto Do_Pop_EnsureIsFromScript() -> void;

#if WITH_DEV_AUTOMATION_TESTS
    CKCORE_API auto Ensure_Impl_ForTesting(
        const FString& InMessage,
        const FString& InExpressionText,
        const FName& InFile,
        int32 InLine,
        bool& OutBreakInCode,
        bool& OutBreakInScript,
        TFunctionRef<void(const FString&)> InWorkerReportEmitter,
        TFunctionRef<void(const FCk_Utils_EditorOnly_PushNewEditorMessage_Params&)> InEditorMessageEmitter) -> void;
#endif
}

// PLATFORM_BREAK expands textually here rather than inside a lambda or an ensureAlwaysMsgf wrapper,
// so the debugger lands directly in the user's function.
#define CK_ENSURE(InExpression, InString, ...)                                                                                             \
(LIKELY(static_cast<bool>(InExpression))                                                                                                   \
    ? true                                                                                                                                 \
    : (ck::ensure::Do_HandleFail(ck::Format_UE(InString, ##__VA_ARGS__), TEXT(#InExpression), __FILE__, __LINE__)                          \
        && FPlatformMisc::IsDebuggerPresent()                                                                                              \
            ? (PLATFORM_BREAK(), false)                                                                                                    \
            : false))

// CK_ENSURE_IF_NOT is an inverted if: the body IS the failure path, and the recovery belongs in it. Evaluate a
// side-effect-safe condition once (hoist it to a local) so the message and the branch share one evaluation.
//
// The CK_DISABLE_ENSURE_CHECKS expansion below is `if constexpr(false)`, which WOULD discard that recovery — but
// that configuration is unreachable: CkBuildConfig.Build.cs pins BuildConfigurationOverride to MatchWithUnreal as a
// compile-time const, so only the two expansions that preserve control flow are ever compiled.
//
// WARNING: ~2,300 call sites now rely on that. If BuildConfiguration::Profile is ever enabled, change this
// expansion to `if (NOT (InExpression))` (matching CK_DISABLE_ENSURE_DEBUGGING) FIRST, or every one of those
// early-outs vanishes silently and simultaneously.
#if CK_DISABLE_ENSURE_CHECKS
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if constexpr(false)
#elif CK_DISABLE_ENSURE_DEBUGGING
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if (NOT (InExpression))
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

// Same expansion as CK_ENSURE, but semantically the trigger expression is NOT part of the ensure itself.
#define CK_TRIGGER_ENSURE_IF(InExpression, InString, ...)\
if(InExpression) { CK_TRIGGER_ENSURE(InString, ##__VA_ARGS__); }

#define CK_INVALID_ENUM(InEnsure)\
CK_TRIGGER_ENSURE(TEXT("Encountered an invalid value for Enum [{}]"), InEnsure)

// --------------------------------------------------------------------------------------------------------------------

#define CK_ENSURE_VALID_IF_NOT(_ToValidate_)                    \
CK_ENSURE_IF_NOT(ck::IsValid(_ToValidate_), TEXT(#_ToValidate_" [{}] is INVALID"), _ToValidate_)

#define CK_ENSURE_VALID_IF_NOT_MSG(_ToValidate_, InFormat, ...) \
CK_ENSURE_IF_NOT(ck::IsValid(_ToValidate_), TEXT(#_ToValidate_ " [{}] is INVALID. {}"), _ToValidate_, ck::Format_UE(InFormat, ##__VA_ARGS__))

// --------------------------------------------------------------------------------------------------------------------
