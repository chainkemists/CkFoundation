#include "CkEnsure.h"

#include "CkValidation/CkIsValid.h"
#include "CkDebug/CkDebug_Utils.h"

#if WITH_EDITOR
#include <Kismet2/KismetDebugUtilities.h>
#endif

#define CK_ENSURE_BP(InExpression, InString, ...)                                                                                              \
[&]() -> bool                                                                                                                                  \
{                                                                                                                                              \
    using namespace ck;                                                                                                                        \
                                                                                                                                               \
    if (LIKELY(InExpression))                                                                                                                  \
    { return InExpression; }                                                                                                                   \
                                                                                                                                               \
    const auto& message = ck::Format_UE(InString, ##__VA_ARGS__);                                                                              \
    const auto& callStack = ck::Format_UE                                                                                                      \
    (                                                                                                                                          \
        TEXT("== BP CallStack ==\n{}\n"),                                                                                                      \
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{})                                                             \
    );                                                                                                                                         \
    const auto& callstackPlusMessage = ck::Format_UE                                                                                           \
    (                                                                                                                                          \
        TEXT("{}\n\n"),                                                                                                                        \
        message,                                                                                                                               \
        callStack                                                                                                                              \
    );                                                                                                                                         \
                                                                                                                                               \
    if (UCk_Utils_Ensure_UE::Get_IsEnsureIgnored_WithCallstack(callStack))                                                                     \
    { return InExpression; }                                                                                                                   \
                                                                                                                                               \
    const auto& dialogMessage = FText::FromString(callstackPlusMessage);                                                                       \
    const auto& _res = UCk_Utils_MessageDialog_UE::YesNoYesAll(dialogMessage, FText::FromString(TEXT("Ignore and Continue?")));                \
    switch(_res)                                                                                                                               \
    {                                                                                                                                          \
        case ECk_MessageDialog_YesNoYesAll::Yes:                                                                                               \
        {                                                                                                                                      \
            return InExpression;                                                                                                               \
        }                                                                                                                                      \
        case ECk_MessageDialog_YesNoYesAll::No:                                                                                                \
        {                                                                                                                                      \
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(InContext);                                                                       \
            return InExpression;                                                                                                               \
        }                                                                                                                                      \
        case ECk_MessageDialog_YesNoYesAll::YesAll:                                                                                            \
        {                                                                                                                                      \
            UCk_Utils_Ensure_UE::Request_IgnoreEnsure_WithCallstack(callStack);                                                                \
            return InExpression;                                                                                                               \
        }                                                                                                                                      \
        default:                                                                                                                               \
        {                                                                                                                                      \
            return ensureMsgf(false, TEXT("Encountered an invalid value for Enum [{}]"), _res);                                                \
        }                                                                                                                                      \
    }                                                                                                                                          \
}()

// --------------------------------------------------------------------------------------------------------------------

TMap<FName, TSet<FCk_Ensure_IgnoredEntry>>	UCk_Utils_Ensure_UE::_IgnoredEnsures;
TSet<FString>                               UCk_Utils_Ensure_UE::_IgnoredEnsures_BP;
FCk_Ensure_OnEnsureIgnored_Delegate_MC      UCk_Utils_Ensure_UE::_OnIgnoredEnsure_MC;

// --------------------------------------------------------------------------------------------------------------------

FCk_Ensure_IgnoredEntry::
FCk_Ensure_IgnoredEntry(FName InName,
    int32 InLineNumber,
    const FText& InMessage)
    : _FileName  (InName)
    , _LineNumber(InLineNumber)
    , _Message   (InMessage)
{
}

auto FCk_Ensure_IgnoredEntry::
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

auto UCk_Utils_Ensure_UE::
EnsureMsgf(bool InExpression,
           FText InMsg,
           ECk_Ensure_HitStatus& OutHitStatus,
           const UObject* InContext)
-> void
{
    if (CK_ENSURE_BP(InExpression,
        TEXT("{}\n\n== BP Callstack ==\n{}"),
        InMsg.ToString(),
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{})))
    {
        OutHitStatus = ECk_Ensure_HitStatus::NotHit;
    }
    else
    {
        OutHitStatus = ECk_Ensure_HitStatus::Hit;
    }
}

auto UCk_Utils_Ensure_UE::
Get_AllIgnoredEnsures()
-> TArray<FCk_Ensure_IgnoredEntry>
{
    const auto& allIgnoredEnsures = [&]() -> TArray<FCk_Ensure_IgnoredEntry>
    {
        auto ret = TArray<FCk_Ensure_IgnoredEntry>{};

        auto outIgnoredEnsureEntrySets = TArray<TSet<FCk_Ensure_IgnoredEntry>>{};
        _IgnoredEnsures.GenerateValueArray(outIgnoredEnsureEntrySets);

        for (const auto& ignoredEnsureEntrySet : outIgnoredEnsureEntrySets)
        {
            ret.Append(ignoredEnsureEntrySet.Array());
        }

        return ret;
    }();

    return allIgnoredEnsures;
}

auto UCk_Utils_Ensure_UE::
Request_ClearAllIgnoredEnsures()
-> void
{
    _IgnoredEnsures.Empty();
    _IgnoredEnsures_BP.Empty();
}

auto UCk_Utils_Ensure_UE::
Bind_To_OnEnsureIgnored(const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate)
-> void
{
    _OnIgnoredEnsure_MC.Add(InDelegate);
}

auto UCk_Utils_Ensure_UE::
Unbind_From_OnEnsureIgnored(const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate)
-> void
{
    _OnIgnoredEnsure_MC.Remove(InDelegate);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsureAtFileAndLine(FName InFile, int32 InLine)
    -> void
{
    Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, FText::GetEmpty(), InLine);
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsureAtFileAndLineWithMessage
    (
        FName        InFile,
        const FText& InMessage,
        int32        InLine
    )
    -> void
{
    auto& lineSet = _IgnoredEnsures.FindOrAdd(InFile);
    const auto& ignoredEnsure = FCk_Ensure_IgnoredEntry{InFile, InLine, InMessage};
    lineSet.Add(ignoredEnsure);

    _OnIgnoredEnsure_MC.Broadcast
    (
        FCk_Ensure_OnEnsureIgnored_Payload{ignoredEnsure}
    );
}

auto
    UCk_Utils_Ensure_UE::
    Get_IsEnsureIgnored(FName InFile, int32 InLine)
    -> bool
{
    const auto* lineSet = _IgnoredEnsures.Find(InFile);

    if (NOT ck::IsValid(lineSet, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return ck::IsValid
    (
        lineSet->Find(FCk_Ensure_IgnoredEntry{InFile, InLine}),
        ck::IsValid_Policy_NullptrOnly{}
    );
}

auto
    UCk_Utils_Ensure_UE::
    Request_IgnoreEnsure_WithCallstack(const FString& InCallstack)
    -> void
{
    _IgnoredEnsures_BP.Add(InCallstack);
}

auto
    UCk_Utils_Ensure_UE::
    Get_IsEnsureIgnored_WithCallstack(const FString& InCallstack)
    -> bool
{
    return ck::IsValid(_IgnoredEnsures_BP.Find(InCallstack), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------

#undef CK_ENSURE_BP
