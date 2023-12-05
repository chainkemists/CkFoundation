#include "CkEnsure_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

TWeakObjectPtr<UCk_Ensure_Subsystem_UE> UCk_Ensure_Subsystem_UE::_Instance;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ensure_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _Instance = this;
}

auto
    UCk_Ensure_Subsystem_UE::
    Deinitialize()
    -> void
{
    _Instance.Reset();

    _NumberOfEnsuresTriggered = 0;
    _IgnoredEnsures.Reset();
    _IgnoredEnsures_BP.Reset();

    _OnIgnoredEnsure_MC.Clear();
    _OnEnsureCountChanged_MC.Clear();

    Super::Deinitialize();
}

auto
    UCk_Ensure_Subsystem_UE::
    Get_Instance()
    -> UCk_Ensure_Subsystem_UE*
{
    return _Instance.Get();
}


auto
    UCk_Ensure_Subsystem_UE::
    Get_AllIgnoredEnsures() const
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
    UCk_Ensure_Subsystem_UE::
    Get_NumberOfEnsuresTriggered() const
    -> int32
{
    return _NumberOfEnsuresTriggered;
}

auto
    UCk_Ensure_Subsystem_UE::
    Get_IsEnsureIgnored(
        FName InFile,
        int32 InLine) const
    -> bool
{
    const auto* LineSet = _IgnoredEnsures.Find(InFile);

    if (ck::Is_NOT_Valid(LineSet, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return ck::IsValid
    (
        LineSet->Find(FCk_Ensure_IgnoredEntry{InFile, InLine}),
        ck::IsValid_Policy_NullptrOnly{}
    );
}

auto
    UCk_Ensure_Subsystem_UE::
    BindTo_OnEnsureIgnored(
        const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate)
    -> void
{
    _OnIgnoredEnsure_MC.Add(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    UnbindFrom_OnEnsureIgnored(
        const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate)
    -> void
{
    _OnIgnoredEnsure_MC.Remove(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    BindTo_OnEnsureCountChanged(
        const FCk_Ensure_OnEnsureCountChanged_Delegate& InDelegate)
    -> void
{
    _OnEnsureCountChanged_MC.Add(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    UnbindFrom_OnEnsureCountChanged(
        const FCk_Ensure_OnEnsureCountChanged_Delegate& InDelegate)
    -> void
{
    _OnEnsureCountChanged_MC.Remove(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_ClearAllIgnoredEnsures()
    -> void
{
    _IgnoredEnsures.Empty();
    _IgnoredEnsures_BP.Empty();
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IgnoreEnsureAtFileAndLine(
        FName InFile,
        int32 InLine)
    -> void
{
    Request_IgnoreEnsureAtFileAndLineWithMessage(InFile, FText::GetEmpty(), InLine);
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IgnoreEnsureAtFileAndLineWithMessage(
        FName        InFile,
        const FText& InMessage,
        int32        InLine)
    -> void
{
    auto& LineSet = _IgnoredEnsures.FindOrAdd(InFile);
    const auto& IgnoredEnsure = FCk_Ensure_IgnoredEntry{InFile, InLine, InMessage};
    LineSet.Add(IgnoredEnsure);

    _OnIgnoredEnsure_MC.Broadcast(FCk_Ensure_OnEnsureIgnored_Payload{IgnoredEnsure});
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IgnoreEnsure_WithCallstack(
        const FString& InCallstack)
    -> void
{
    _IgnoredEnsures_BP.Add(InCallstack);
}

auto
    UCk_Ensure_Subsystem_UE::
    Get_IsEnsureIgnored_WithCallstack(
        const FString& InCallstack) const
    -> bool
{
    return ck::IsValid(_IgnoredEnsures_BP.Find(InCallstack), ck::IsValid_Policy_NullptrOnly{});
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IncrementEnsureCount()
    -> void
{
    ++_NumberOfEnsuresTriggered;

    _OnEnsureCountChanged_MC.Broadcast(_NumberOfEnsuresTriggered);
}

// --------------------------------------------------------------------------------------------------------------------
