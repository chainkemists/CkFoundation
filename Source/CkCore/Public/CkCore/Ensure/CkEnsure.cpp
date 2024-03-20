#include "CkEnsure.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/Ensure/CkEnsure_Subsystem.h"
#include "CkCore/Settings/CkCore_Settings.h"

#if WITH_EDITOR
#include <Kismet2/KismetDebugUtilities.h>
#endif

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
    const auto IsMessageOnly = UCk_Utils_Core_ProjectSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;          \
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
    if (UCk_Utils_Core_ProjectSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)                                \
    {                                                                                                                                          \
        ck::core::Error(TEXT("{}"), CallstackPlusMessage);                                                                                     \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, DialogMessage, __LINE__);                                  \
        return false;                                                                                                                          \
    }                                                                                                                                          \
                                                                                                                                               \
    _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE("CkEnsure Blueprints", CallstackPlusMessage, InContext);                                             \
                                                                                                                                               \
    if (UCk_Utils_Core_ProjectSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::MessageLog)                                  \
    {                                                                                                                                          \
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(InContext, DialogMessage);                                                            \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(CallStack);                                                                    \
        return false;                                                                                                                          \
    }                                                                                                                                          \
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
    FCk_Ensure_IgnoredEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_LineNumber() == InOther.Get_LineNumber() && Get_FileName().IsEqual(InOther.Get_FileName());
}

auto
    GetTypeHash(
        const FCk_Ensure_IgnoredEntry& InA)
    -> uint8
{
    return GetTypeHash(InA.Get_LineNumber()) + GetTypeHash(InA.Get_FileName());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ensure_UE::
    EnsureMsgf(
        const bool InExpression,
        const FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext)
    -> void
{
    if (NOT CK_ENSURE_BP(InExpression, TEXT("{}.{}"), InMsg.ToString(), ck::Context(InContext)))
    {
        OutHitStatus = ECk_ValidInvalid::Invalid;
        return;
    }

    OutHitStatus = ECk_ValidInvalid::Valid;
}

auto
    UCk_Utils_Ensure_UE::
    EnsureMsgf_IsValid(
        UObject* InObject,
        const FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext)
    -> void
{
    if (NOT CK_ENSURE_BP(ck::IsValid(InObject), TEXT("{}.{}"), InMsg.ToString(), ck::Context(InContext)))
    {
        OutHitStatus = ECk_ValidInvalid::Invalid;
        return;
    }

    OutHitStatus = ECk_ValidInvalid::Valid;
}

auto
    UCk_Utils_Ensure_UE::
    TriggerEnsure(
        FText InMsg,
        const UObject* InContext)
    -> void
{
    CK_ENSURE_BP(false, TEXT("{}.{}"), InMsg.ToString(), ck::Context(InContext));
}

auto
    UCk_Utils_Ensure_UE::
    Get_AllIgnoredEnsures()
    -> TArray<FCk_Ensure_IgnoredEntry>
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return {}; }

    return EnsureSubsystem->Get_AllIgnoredEnsures();
}

auto
    UCk_Utils_Ensure_UE::
    Get_EnsureCount()
    -> int32
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return {}; }

    return EnsureSubsystem->Get_EnsureCount();
}

auto
    UCk_Utils_Ensure_UE::
    Request_ClearAllIgnoredEnsures()
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->Request_ClearAllIgnoredEnsures();
}

auto
    UCk_Utils_Ensure_UE::
    BindTo_OnEnsureIgnored(const FCk_Delegate_OnEnsureIgnored& InDelegate)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->BindTo_OnEnsureIgnored(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    UnbindFrom_OnEnsureIgnored(const FCk_Delegate_OnEnsureIgnored& InDelegate)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->UnbindFrom_OnEnsureIgnored(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    BindTo_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->BindTo_OnEnsureCountChanged(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    UnbindFrom_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->UnbindFrom_OnEnsureCountChanged(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsureAtFileAndLine(
        FName InFile,
        int32 InLine)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->Request_IgnoreEnsureAtFileAndLine(InFile, InLine);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsureAtFileAndLineWithMessage(
        FName        InFile,
        const FText& InMessage,
        int32        InLine)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, InMessage, InLine);
}

auto
    UCk_Utils_Ensure_UE::
    Get_IsEnsureIgnored(
        FName InFile,
        int32 InLine)
    -> bool
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return {}; }

    return EnsureSubsystem->Get_IsEnsureIgnored(InFile, InLine);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsure_WithCallstack(
        const FString& InCallstack)
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->Request_IgnoreEnsure_WithCallstack(InCallstack);
}

auto
    UCk_Utils_Ensure_UE::
    Get_IsEnsureIgnored_WithCallstack(
        const FString& InCallstack)
    -> bool
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return {}; }

    return EnsureSubsystem->Get_IsEnsureIgnored_WithCallstack(InCallstack);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IncrementEnsureCount()
    -> void
{
    const auto EnsureSubsystem = UCk_Ensure_Subsystem_UE::Get_Instance();

    if (ck::Is_NOT_Valid(EnsureSubsystem))
    { return; }

    EnsureSubsystem->Request_IncrementEnsureCount();
}

// --------------------------------------------------------------------------------------------------------------------

#undef CK_ENSURE_BP
