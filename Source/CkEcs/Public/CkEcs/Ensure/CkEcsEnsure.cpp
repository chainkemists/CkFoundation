#include "CkEcsEnsure.h"

#define CK_ENSURE_BP(InExpression, InString, ...)                                                                                              \
[&]() -> bool                                                                                                                                  \
{                                                                                                                                              \
    using namespace ck;                                                                                                                        \
                                                                                                                                               \
    const auto ExpressionResult = InExpression;                                                                                                \
                                                                                                                                               \
    if (LIKELY(ExpressionResult))                                                                                                              \
    { return ExpressionResult; }                                                                                                               \
                                                                                                                                               \
    UCk_Utils_Ensure_UE::Request_IncrementEnsureCount();                                                                                       \
                                                                                                                                               \
    const auto IsMessageOnly = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;             \
                                                                                                                                               \
    const auto& Message = ck::Format_UE(InString, ##__VA_ARGS__);                                                                              \
    const auto& Title = ck::Format_UE(TEXT("Ignore and Continue? Frame#[{}]"), GFrameCounter);                                                 \
    const auto& CallStack = ck::Format_UE                                                                                                      \
    (                                                                                                                                          \
        TEXT("== BP CallStack ==\n{}\n"),                                                                                                      \
        IsMessageOnly ? TEXT("[BP StackTrace DISABLED]") :                                                                                     \
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{})                                                   \
    );                                                                                                                                         \
                                                                                                                                               \
    if (UCk_Utils_Ensure_UE::Get_IsEnsureIgnored_WithCallstack(CallStack))                                                                     \
    { return ExpressionResult; }                                                                                                               \
                                                                                                                                               \
    const auto& CallstackPlusMessage = ck::Format_UE                                                                                           \
    (                                                                                                                                          \
        TEXT("{}\n\n{}"),                                                                                                                      \
        Message,                                                                                                                               \
        CallStack                                                                                                                              \
    );                                                                                                                                         \
                                                                                                                                               \
    const auto& DialogMessage = FText::FromString(CallstackPlusMessage);                                                                       \
                                                                                                                                               \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)                                   \
    {                                                                                                                                          \
        ck::core::Error(TEXT("{}"), CallstackPlusMessage);                                                                                     \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, DialogMessage, __LINE__);                                  \
        return false;                                                                                                                          \
    }                                                                                                                                          \
                                                                                                                                               \
    _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE("CkEnsure Blueprints", CallstackPlusMessage, InContext);                                             \
                                                                                                                                               \
    switch(const auto& Ans = UCk_Utils_MessageDialog_UE::YesNoYesAll(DialogMessage, FText::FromString(Title)))                                 \
    {                                                                                                                                          \
        case ECk_MessageDialog_YesNoYesAll::Yes:                                                                                               \
        {                                                                                                                                      \
            return ExpressionResult;                                                                                                           \
        }                                                                                                                                      \
        case ECk_MessageDialog_YesNoYesAll::No:                                                                                                \
        {                                                                                                                                      \
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(InContext);                                                                       \
            return ExpressionResult;                                                                                                           \
        }                                                                                                                                      \
        case ECk_MessageDialog_YesNoYesAll::YesAll:                                                                                            \
        {                                                                                                                                      \
            UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(CallStack);                                                                \
            return ExpressionResult;                                                                                                           \
        }                                                                                                                                      \
        default:                                                                                                                               \
        {                                                                                                                                      \
            return ensureMsgf(false, TEXT("Encountered an invalid value for Enum [{}]"), Ans);                                                 \
        }                                                                                                                                      \
    }                                                                                                                                          \
}()

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsEnsure_UE::
    EnsureMsgf_IsValid(
        const FCk_Handle& InHandle,
        FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext)
    -> void
{
    if (NOT CK_ENSURE_BP(ck::IsValid(InHandle), TEXT("{}.{}"), InMsg.ToString(), ck::Context(InContext)))
    {
        OutHitStatus = ECk_ValidInvalid::Invalid;
        return;
    }

    OutHitStatus = ECk_ValidInvalid::Valid;
}

// --------------------------------------------------------------------------------------------------------------------

#undef CK_ENSURE_BP
