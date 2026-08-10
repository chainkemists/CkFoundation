#include "CkEnsure_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ensure_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _WorldBeginTearDown_DelegateHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisType::OnWorldBeginTearDown);
}

auto
    UCk_Ensure_Subsystem_UE::
    OnWorldBeginTearDown(
        UWorld* World)
    -> void
{
    if (ck::IsValid(World->GetGameInstance()))
    {
        ck::ensure::Request_LogEnsureOccurrenceSummaryAndReset();
        _IgnoreAllEnsure = false;
        _IgnoredEnsures.Reset();
        _IgnoredEnsures_BP.Reset();

        _OnIgnoredEnsure_MC.Clear();
        _OnEnsureCountChanged_MC.Clear();
    }
}

auto
    UCk_Ensure_Subsystem_UE::
    Deinitialize()
    -> void
{
    ck::ensure::Request_LogEnsureOccurrenceSummaryAndReset();
    _IgnoreAllEnsure = false;
    _IgnoredEnsures.Reset();
    _IgnoredEnsures_BP.Reset();

    _OnIgnoredEnsure_MC.Clear();
    _OnEnsureCountChanged_MC.Clear();

    FWorldDelegates::OnWorldBeginTearDown.Remove(_WorldBeginTearDown_DelegateHandle);

    Super::Deinitialize();
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
    Get_EnsureCount() const
    -> int32
{
    return static_cast<int32>(FMath::Min<uint64>(
        ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount(),
        MAX_int32));
}

auto
    UCk_Ensure_Subsystem_UE::
    Get_UniqueEnsureCount() const
    -> int32
{
    return ck::ensure::Get_EnsureOccurrenceTracker().GetUniqueCount();
}

auto
    UCk_Ensure_Subsystem_UE::
    Get_IsEnsureIgnored(
        FName InFile,
        int32 InLine) const
    -> bool
{
    if (_IgnoreAllEnsure)
    { return true; }

    const auto& EnsureEntry = FCk_Ensure_IgnoredEntry{InFile, InLine};

    const auto CheckIgnoreSet = [&](const TMap<FName, TSet<FCk_Ensure_IgnoredEntry>>& InIgnoreMap) -> bool
    {
        const auto* LineSet = InIgnoreMap.Find(InFile);
        if (ck::Is_NOT_Valid(LineSet, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }
        return ck::IsValid(LineSet->Find(EnsureEntry), ck::IsValid_Policy_NullptrOnly{});
    };

    if (CheckIgnoreSet(_PersistentIgnoredEnsures))
    { return true; }

    return CheckIgnoreSet(_IgnoredEnsures);
}

auto
    UCk_Ensure_Subsystem_UE::
    BindTo_OnEnsureIgnored(
        const FCk_Delegate_OnEnsureIgnored& InDelegate)
    -> void
{
    _OnIgnoredEnsure_MC.Add(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    UnbindFrom_OnEnsureIgnored(
        const FCk_Delegate_OnEnsureIgnored& InDelegate)
    -> void
{
    _OnIgnoredEnsure_MC.Remove(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    BindTo_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate)
    -> void
{
    _OnEnsureCountChanged_MC.Add(InDelegate);
}

auto
    UCk_Ensure_Subsystem_UE::
    UnbindFrom_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate)
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
        FName InFile,
        const FText& InMessage,
        int32 InLine)
    -> void
{
    auto& LineSet = _IgnoredEnsures.FindOrAdd(InFile);
    const auto& IgnoredEnsure = FCk_Ensure_IgnoredEntry{InFile, InLine}.Set_Message(InMessage);
    LineSet.Add(IgnoredEnsure);

    _OnIgnoredEnsure_MC.Broadcast(FCk_Payload_OnEnsureIgnored{IgnoredEnsure});
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
    Request_IgnoreEnsurePermanently_AtFileAndLine(
        FName InFile,
        int32 InLine)
    -> void
{
    auto& LineSet = _PersistentIgnoredEnsures.FindOrAdd(InFile);
    const auto& IgnoredEnsure = FCk_Ensure_IgnoredEntry{InFile, InLine};
    LineSet.Add(IgnoredEnsure);

    _OnIgnoredEnsure_MC.Broadcast(FCk_Payload_OnEnsureIgnored{IgnoredEnsure});
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IgnoreEnsurePermanently_WithCallstack(
        const FString& InCallstack)
    -> void
{
    _PersistentIgnoredEnsures_BP.Add(InCallstack);
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IgnoreAllEnsures()
    -> void
{
    _IgnoreAllEnsure = true;
}

auto
    UCk_Ensure_Subsystem_UE::
    Get_IsEnsureIgnored_WithCallstack(
        const FString& InCallstack) const
    -> bool
{
    if (_IgnoreAllEnsure)
    { return true; }

    if (ck::IsValid(_PersistentIgnoredEnsures_BP.Find(InCallstack), ck::IsValid_Policy_NullptrOnly{}))
    { return true; }

    return ck::IsValid(_IgnoredEnsures_BP.Find(InCallstack), ck::IsValid_Policy_NullptrOnly{});
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_NotifyEnsureCountChanged(
        const ck::ensure::FCk_EnsureRecordResult& InRecord)
    -> void
{
    _OnEnsureCountChanged_MC.Broadcast(
        static_cast<int32>(FMath::Min<uint64>(InRecord.TotalCount, MAX_int32)),
        InRecord.UniqueCount);
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IncrementEnsureCountAtFileAndLine(
        FName InFile,
        int32 InLine)
    -> void
{
    const auto& Record = ck::ensure::Get_EnsureOccurrenceTracker().Record(
        ck::ensure::FCk_EnsureSignature{InFile, InLine, {}, {}});
    Request_NotifyEnsureCountChanged(Record);
}

auto
    UCk_Ensure_Subsystem_UE::
    Request_IncrementEnsureCountWithCallstack(
        const FString& InCallstack)
    -> void
{
    const auto& Record = ck::ensure::Get_EnsureOccurrenceTracker().Record(
        ck::ensure::FCk_EnsureSignature{{}, 0, {}, InCallstack});
    Request_NotifyEnsureCountChanged(Record);
}

// --------------------------------------------------------------------------------------------------------------------
