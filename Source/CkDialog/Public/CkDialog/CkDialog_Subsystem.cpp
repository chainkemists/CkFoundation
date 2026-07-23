#include "CkDialog_Subsystem.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkDialog/CkDialog_Log.h"
#include "CkDialog/Line/CkDialogLine_Utils.h"
#include "CkDialog/Settings/CkDialog_ProjectSettings.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogRegistry_Subsystem_UE::
    PostInitialize()
    -> void
{
    Super::PostInitialize();

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    _RegistryRoot = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(World);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(_RegistryRoot, TEXT("DialogRegistryRoot"));
#endif

    const auto Banks = UCk_Utils_Dialog_Settings_UE::Get_DialogBanks();
    for (const auto& SoftBank : Banks)
    {
        auto* Bank = SoftBank.LoadSynchronous();

        const auto BankValid = ck::IsValid(Bank);
        CK_ENSURE_IF_NOT(BankValid,
            TEXT("Dialog settings bank [{}] failed to load — skipping"), SoftBank.ToString())
        {}
        if (NOT BankValid)
        { continue; }

        DoRegisterBank(Bank);
    }

    _AllBanksProcessed = true;

    ck::dialog::Display(TEXT("Dialog registry initialized: [{}] line(s), [{}] pending condition spawn(s)"),
        _AllLines.Num(), _PendingConditionSpawns);
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (ck::IsValid(_RegistryRoot))
    {
        // Cascades every line entity and its condition children.
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_RegistryRoot);
    }

    _RegistryRoot = FCk_Handle{};
    _AllLines.Reset();
    _LineIDs.Reset();
    _LineHandleToID.Reset();
    _RegisteredBanks.Reset();
    _BankToLines.Reset();
    _AllBanksProcessed = false;
    _PendingConditionSpawns = 0;

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogRegistry_Subsystem_UE::
    Get_DialogRegistry(
        const UObject* InWorldContextObject)
    -> UCk_DialogRegistry_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InWorldContextObject))
    { return nullptr; }

    auto* World = InWorldContextObject->GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCk_DialogRegistry_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogRegistry_Subsystem_UE::
    Get_IsReady() const
    -> bool
{
    return _AllBanksProcessed && _PendingConditionSpawns == 0;
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Get_AllLines()
    -> TArray<FCk_Handle_DialogLine>
{
    DoPruneInvalidLines();
    return _AllLines;
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Get_Lines_ByEventTag(
        FGameplayTag InEventTag)
    -> TArray<FCk_Handle_DialogLine>
{
    DoPruneInvalidLines();

    auto Result = TArray<FCk_Handle_DialogLine>{};
    for (auto& Line : _AllLines)
    {
        if (UCk_Utils_DialogLine_UE::Get_EventTag(Line) == InEventTag)
        { Result.Emplace(Line); }
    }
    return Result;
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Get_RegisteredBanks() const
    -> TArray<UCk_DialogBank_DataAsset*>
{
    auto Result = TArray<UCk_DialogBank_DataAsset*>{};
    for (const auto& Bank : _RegisteredBanks)
    { Result.Emplace(Bank.Get()); }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogRegistry_Subsystem_UE::
    Request_RegisterBank(
        UCk_DialogBank_DataAsset* InBank)
    -> void
{
    const auto BankValid = ck::IsValid(InBank);
    CK_ENSURE_IF_NOT(BankValid, TEXT("Null bank passed to Dialog Request_RegisterBank"))
    {}
    if (NOT BankValid)
    { return; }

    DoRegisterBank(InBank);
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Request_UnregisterBank(
        UCk_DialogBank_DataAsset* InBank)
    -> void
{
    if (NOT _RegisteredBanks.Contains(InBank))
    { return; }

    if (auto* Lines = _BankToLines.Find(InBank))
    {
        // Copy: Request_UnregisterLine mutates _AllLines/_LineIDs, and destroys entities.
        const auto LinesCopy = *Lines;
        for (auto Line : LinesCopy)
        { Request_UnregisterLine(Line); }

        _BankToLines.Remove(InBank);
    }

    _RegisteredBanks.Remove(InBank);
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Request_RegisterLine(
        FCk_DialogBank_LineData InLineData,
        FGameplayTagContainer InBankTags)
    -> FCk_Handle_DialogLine
{
    const auto RootValid = ck::IsValid(_RegistryRoot);
    CK_ENSURE_IF_NOT(RootValid,
        TEXT("Dialog registry root invalid; cannot register line [{}]"), InLineData.Get_LineID())
    {}
    if (NOT RootValid)
    { return {}; }

    const auto LineID = InLineData.Get_LineID();

    const auto LineIDValid = NOT LineID.IsNone();
    CK_ENSURE_IF_NOT(LineIDValid, TEXT("Dialog line has an empty LineID — skipping registration"))
    {}
    if (NOT LineIDValid)
    { return {}; }

    // Drop any IDs whose line entity was destroyed out-of-band (e.g. a test's Track_ForCleanup) BEFORE the duplicate
    // check, so a legitimately-reused ID is not falsely rejected.
    DoPruneInvalidLines();

    // Duplicate LineID across all banks: reject the duplicate atomically (a registration is all-or-nothing).
    const auto DuplicateLineID = _LineIDs.Contains(LineID);
    CK_ENSURE_IF_NOT(NOT DuplicateLineID,
        TEXT("Duplicate Dialog LineID [{}] — skipping the duplicate"), LineID)
    {}
    if (DuplicateLineID)
    { return {}; }

    const auto WeakThis = TWeakObjectPtr<UCk_DialogRegistry_Subsystem_UE>{this};

    auto NumConditionsQueued = 0;
    auto NewLine = UCk_Utils_DialogLine_UE::Create(_RegistryRoot, InBankTags, InLineData,
        [WeakThis]()
        {
            if (auto* Self = WeakThis.Get())
            { Self->DoOnConditionConstructed(); }
        },
        NumConditionsQueued);

    // Increment AFTER Create returns (condition post-construction callbacks are deferred and cannot have fired yet).
    _PendingConditionSpawns += NumConditionsQueued;
    _LineIDs.Add(LineID);
    _LineHandleToID.Add(NewLine, LineID);
    _AllLines.Emplace(NewLine);

    return NewLine;
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Request_RegisterLine_WithCondition(
        FCk_DialogBank_LineData InLineData,
        FGameplayTagContainer InBankTags,
        UCk_DialogCondition_EntityScript* InCondition)
    -> FCk_Handle_DialogLine
{
    if (ck::IsValid(InCondition))
    {
        auto Conditions = InLineData.Get_Conditions();
        Conditions.Emplace(InCondition);
        InLineData.Set_Conditions(Conditions);
    }

    return Request_RegisterLine(InLineData, InBankTags);
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    Request_UnregisterLine(
        FCk_Handle_DialogLine InLine)
    -> void
{
    if (ck::Is_NOT_Valid(InLine))
    { return; }

    _LineIDs.Remove(UCk_Utils_DialogLine_UE::Get_LineID(InLine));
    _LineHandleToID.Remove(InLine);
    _AllLines.Remove(InLine);

    auto LineToDestroy = FCk_Handle{InLine};
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(LineToDestroy);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogRegistry_Subsystem_UE::
    DoRegisterBank(
        UCk_DialogBank_DataAsset* InBank)
    -> void
{
    if (_RegisteredBanks.Contains(InBank))
    { return; }

    _RegisteredBanks.Add(InBank);
    auto& BankLines = _BankToLines.FindOrAdd(InBank);

    for (const auto& LineData : InBank->Get_Lines())
    {
        auto NewLine = Request_RegisterLine(LineData, InBank->Get_BankTags());
        if (ck::IsValid(NewLine))
        { BankLines.Emplace(NewLine); }
    }
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    DoOnConditionConstructed()
    -> void
{
    if (_PendingConditionSpawns > 0)
    { --_PendingConditionSpawns; }
}

auto
    UCk_DialogRegistry_Subsystem_UE::
    DoPruneInvalidLines()
    -> void
{
    _AllLines.RemoveAll([&](const FCk_Handle_DialogLine& InLine)
    {
        if (ck::IsValid(InLine))
        { return false; }

        // Stale: its entity was destroyed out-of-band. Drop its ID so the ID can be reused.
        if (const auto* StaleID = _LineHandleToID.Find(InLine))
        { _LineIDs.Remove(*StaleID); }
        _LineHandleToID.Remove(InLine);
        return true;
    });
}

// --------------------------------------------------------------------------------------------------------------------
