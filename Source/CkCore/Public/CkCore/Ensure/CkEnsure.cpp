#include "CkEnsure.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/Ensure/CkEnsure_Subsystem.h"
#include "CkCore/Settings/CkCore_Settings.h"

#if WITH_EDITOR
#include <Kismet2/KismetDebugUtilities.h>
#endif

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
