#include "CkEnsure.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Debug/CkDebug_Utils.h"
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
    const auto IsMessageOnly = UCk_Utils_Core_ProjectSettings_UE::Get_EnsureDetailsPolicy() == ECk_Core_EnsureDetailsPolicy::MessageOnly;      \
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
    _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE("CkEnsure Blueprints", CallstackPlusMessage, InContext);                                             \
                                                                                                                                               \
    const auto& DialogMessage = FText::FromString(CallstackPlusMessage);                                                                       \
    const auto& _Res = UCk_Utils_MessageDialog_UE::YesNoYesAll(DialogMessage, FText::FromString(Title));                                       \
    switch(_Res)                                                                                                                               \
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
            return ensureMsgf(false, TEXT("Encountered an invalid value for Enum [{}]"), _Res);                                                \
        }                                                                                                                                      \
    }                                                                                                                                          \
}()

// --------------------------------------------------------------------------------------------------------------------

TMap<FName, TSet<FCk_Ensure_IgnoredEntry>>	UCk_Utils_Ensure_UE::_IgnoredEnsures;
TSet<FString>                               UCk_Utils_Ensure_UE::_IgnoredEnsures_BP;
FCk_Ensure_OnEnsureIgnored_Delegate_MC      UCk_Utils_Ensure_UE::_OnIgnoredEnsure_MC;

// --------------------------------------------------------------------------------------------------------------------

FCk_Ensure_IgnoredEntry::
    FCk_Ensure_IgnoredEntry(
        FName InName,
        int32 InLineNumber,
        const FText& InMessage)
    : _FileName  (InName)
    , _LineNumber(InLineNumber)
    , _Message   (InMessage)
{
}

auto
    FCk_Ensure_IgnoredEntry::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_LineNumber() == InOther.Get_LineNumber() && Get_FileName().IsEqual(InOther.Get_FileName());
}

auto
    GetTypeHash(const FCk_Ensure_IgnoredEntry& InA)
    -> uint8
{
    return GetTypeHash(InA.Get_LineNumber()) + GetTypeHash(InA.Get_FileName());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Ensure_OnEnsureIgnored_Payload::
    FCk_Ensure_OnEnsureIgnored_Payload(const FCk_Ensure_IgnoredEntry& InIgnoredEnsure)
    : _IgnoredEnsure(InIgnoredEnsure)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ensure_UE::
    EnsureMsgf(
        const bool        InExpression,
        const FText       InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject*    InContext)
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
        UObject*          InObject,
        const FText       InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject*    InContext)
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
        FText          InMsg,
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
    const auto& AllIgnoredEnsures = [&]() -> TArray<FCk_Ensure_IgnoredEntry>
    {
        auto Ret = TArray<FCk_Ensure_IgnoredEntry>{};

        auto IgnoredEnsureEntrySets = TArray<TSet<FCk_Ensure_IgnoredEntry>>{};
        _IgnoredEnsures.GenerateValueArray(IgnoredEnsureEntrySets);

        for (const auto& IgnoredEnsureEntrySet : IgnoredEnsureEntrySets)
        {
            Ret.Append(IgnoredEnsureEntrySet.Array());
        }

        return Ret;
    }();

    return AllIgnoredEnsures;
}

auto
    UCk_Utils_Ensure_UE::
    Request_ClearAllIgnoredEnsures()
    -> void
{
    _IgnoredEnsures.Empty();
    _IgnoredEnsures_BP.Empty();
}

auto
    UCk_Utils_Ensure_UE::
    Bind_To_OnEnsureIgnored(const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate)
    -> void
{
    _OnIgnoredEnsure_MC.Add(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    Unbind_From_OnEnsureIgnored(const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate)
    -> void
{
    _OnIgnoredEnsure_MC.Remove(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsureAtFileAndLine(
        FName InFile,
        int32 InLine)
    -> void
{
    Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, FText::GetEmpty(), InLine);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsureAtFileAndLineWithMessage(
        FName        InFile,
        const FText& InMessage,
        int32        InLine)
    -> void
{
    auto& LineSet = _IgnoredEnsures.FindOrAdd(InFile);
    const auto& IgnoredEnsure = FCk_Ensure_IgnoredEntry{InFile, InLine, InMessage};
    LineSet.Add(IgnoredEnsure);

    _OnIgnoredEnsure_MC.Broadcast
    (
        FCk_Ensure_OnEnsureIgnored_Payload{IgnoredEnsure}
    );
}

auto
    UCk_Utils_Ensure_UE::
    Get_IsEnsureIgnored(
        FName InFile,
        int32 InLine)
    -> bool
{
    const auto* LineSet = _IgnoredEnsures.Find(InFile);

    if (NOT ck::IsValid(LineSet, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return ck::IsValid
    (
        LineSet->Find(FCk_Ensure_IgnoredEntry{InFile, InLine}),
        ck::IsValid_Policy_NullptrOnly{}
    );
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsure_WithCallstack(
        const FString& InCallstack)
    -> void
{
    _IgnoredEnsures_BP.Add(InCallstack);
}

auto
    UCk_Utils_Ensure_UE::
    Get_IsEnsureIgnored_WithCallstack(
        const FString& InCallstack)
    -> bool
{
    return ck::IsValid(_IgnoredEnsures_BP.Find(InCallstack), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------

#undef CK_ENSURE_BP
