#include "CkDialogEmitter_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkDialog/CkDialog_Log.h"
#include "CkDialog/Line/CkDialogLine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_DialogEmitter_UE, FCk_Handle_DialogEmitter,
    ck::FFragment_DialogEmitter_Params, ck::FTag_DialogEmitter);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dialog_emitter_utils
{
    // Presence is nearly the answer once TickCooldowns has retired finished entries — but a Forever entry is never
    // ticked and so never reports Done, and an entry can be read in the same frame it was started.
    auto Is_EntryActive(const FCk_DialogEmitter_CooldownEntry& InEntry) -> bool
    {
        return InEntry.Get_DurationMode() == ECk_Dialog_CooldownDuration::Forever ||
               NOT InEntry.Get_Cooldown().Get_IsDone();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogEmitter_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_DialogEmitter_ParamsData& InParams)
    -> FCk_Handle_DialogEmitter
{
    const auto HandleValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleValid, TEXT("Invalid Handle [{}] passed to DialogEmitter Add"), InHandle)
    {}
    if (NOT HandleValid)
    { return {}; }

    const auto AlreadyHasEmitter = Has(InHandle);
    CK_ENSURE_IF_NOT(NOT AlreadyHasEmitter,
        TEXT("Entity [{}] already has a DialogEmitter — one emitter per entity. Returning the existing one."), InHandle)
    {}
    if (AlreadyHasEmitter)
    { return ck::StaticCast<FCk_Handle_DialogEmitter>(InHandle); }

    InHandle.Add<ck::FFragment_DialogEmitter_Params>(InParams);
    InHandle.Add<ck::FFragment_DialogEmitter_Cooldowns>();
    InHandle.Add<ck::FFragment_DialogEmitter_Debug>();
    InHandle.Add<ck::FTag_DialogEmitter>();

    return ck::StaticCast<FCk_Handle_DialogEmitter>(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogEmitter_UE::
    Get_EmitterTags(
        const FCk_Handle_DialogEmitter& InEmitter)
    -> FGameplayTagContainer
{
    return InEmitter.Get<ck::FFragment_DialogEmitter_Params>().Get_EmitterTags();
}

auto
    UCk_Utils_DialogEmitter_UE::
    Get_IsLineOnCooldown(
        const FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Handle_DialogLine& InLine)
    -> bool
{
    const auto& Cooldowns = InEmitter.Get<ck::FFragment_DialogEmitter_Cooldowns>().Get_Cooldowns();
    const auto* Entry = Cooldowns.Find(InLine);
    if (Entry == nullptr)
    { return false; }

    return ck_dialog_emitter_utils::Is_EntryActive(*Entry);
}

auto
    UCk_Utils_DialogEmitter_UE::
    Get_ActiveCooldowns(
        const FCk_Handle_DialogEmitter& InEmitter)
    -> TArray<FCk_Handle_DialogLine>
{
    const auto& Cooldowns = InEmitter.Get<ck::FFragment_DialogEmitter_Cooldowns>().Get_Cooldowns();

    auto Result = TArray<FCk_Handle_DialogLine>{};
    for (const auto& Cooldown : Cooldowns)
    {
        if (ck_dialog_emitter_utils::Is_EntryActive(Cooldown.Value))
        { Result.Emplace(Cooldown.Key); }
    }
    return Result;
}

auto
    UCk_Utils_DialogEmitter_UE::
    Get_CooldownRemaining(
        const FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Handle_DialogLine& InLine)
    -> FCk_Time
{
    const auto& Cooldowns = InEmitter.Get<ck::FFragment_DialogEmitter_Cooldowns>().Get_Cooldowns();
    const auto* Entry = Cooldowns.Find(InLine);
    if (Entry == nullptr)
    { return FCk_Time::ZeroSecond(); }

    // A Forever cooldown reports whatever goal it was constructed with (usually zero) — deliberately not a sentinel,
    // which a caller comparing magnitudes could not tell from a genuinely long Timed window. Branch on
    // Get_CooldownEntry(...).Get_DurationMode() instead.
    return Entry->Get_Cooldown().Get_TimeRemaining();
}

auto
    UCk_Utils_DialogEmitter_UE::
    Get_CooldownEntry(
        const FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Handle_DialogLine& InLine)
    -> FCk_DialogEmitter_CooldownEntry
{
    const auto& Cooldowns = InEmitter.Get<ck::FFragment_DialogEmitter_Cooldowns>().Get_Cooldowns();
    const auto* Entry = Cooldowns.Find(InLine);
    if (Entry == nullptr)
    { return {}; }

    return *Entry;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogEmitter_UE::
    Request_Query(
        FCk_Handle_DialogEmitter& InEmitter,
        FCk_Request_DialogEmitter_Query InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    CK_CALLSTACK_RECORD(ck::FFragment_DialogEmitter_Requests, InEmitter);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InEmitter.AddOrGet<ck::FFragment_DialogEmitter_Requests>()._Requests.Emplace(InRequest);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    Request_QueryFollowUp(
        FCk_Handle_DialogEmitter& InEmitter,
        FCk_Handle_DialogLine InPlayedLine,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    // A missing exit link (or an already-invalid played line) is a NORMAL end-of-chain state, not a failure.
    if (ck::Is_NOT_Valid(InPlayedLine))
    {
        ck::dialog::Display(TEXT("Dialog QueryFollowUp on Emitter [{}]: invalid played line — no-op"), InEmitter);
        InDelegate.ExecuteIfBound(InEmitter, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InEmitter;
    }

    const auto LinkedEventTag = UCk_Utils_DialogLine_UE::Get_LinkedEventTag(InPlayedLine);
    if (NOT LinkedEventTag.IsValid())
    {
        ck::dialog::Display(TEXT("Dialog QueryFollowUp on Emitter [{}]: played line [{}] has no exit tag — no-op"),
            InEmitter, InPlayedLine);
        InDelegate.ExecuteIfBound(InEmitter, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InEmitter;
    }

    return Request_Query(InEmitter, FCk_Request_DialogEmitter_Query{LinkedEventTag}, InDelegate);
}

auto
    UCk_Utils_DialogEmitter_UE::
    Request_StartCooldown(
        FCk_Handle_DialogEmitter& InEmitter,
        FCk_Request_DialogEmitter_StartCooldown InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    CK_CALLSTACK_RECORD(ck::FFragment_DialogEmitter_Requests, InEmitter);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InEmitter.AddOrGet<ck::FFragment_DialogEmitter_Requests>()._Requests.Emplace(InRequest);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    Request_ClearCooldown(
        FCk_Handle_DialogEmitter& InEmitter,
        FCk_Handle_DialogLine InLine,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    CK_CALLSTACK_RECORD(ck::FFragment_DialogEmitter_Requests, InEmitter);

    const auto Request = FCk_Request_DialogEmitter_ClearCooldown{InLine};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InEmitter.AddOrGet<ck::FFragment_DialogEmitter_Requests>()._Requests.Emplace(Request);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    Request_ClearAllCooldowns(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    CK_CALLSTACK_RECORD(ck::FFragment_DialogEmitter_Requests, InEmitter);

    const auto Request = FCk_Request_DialogEmitter_ClearAllCooldowns{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InEmitter.AddOrGet<ck::FFragment_DialogEmitter_Requests>()._Requests.Emplace(Request);

    return InEmitter;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogEmitter_UE::
    BindTo_OnQueryCompleted(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnQueryCompleted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_DialogEmitter
{
    ck::UUtils_Signal_OnDialogQueryCompleted::Bind(InEmitter, InDelegate, InBindingPolicy);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    UnbindFrom_OnQueryCompleted(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnQueryCompleted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    ck::UUtils_Signal_OnDialogQueryCompleted::Unbind(InEmitter, InDelegate);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    BindTo_OnCooldownStarted(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownStarted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_DialogEmitter
{
    ck::UUtils_Signal_OnDialogCooldownStarted::Bind(InEmitter, InDelegate, InBindingPolicy);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    UnbindFrom_OnCooldownStarted(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownStarted& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    ck::UUtils_Signal_OnDialogCooldownStarted::Unbind(InEmitter, InDelegate);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    BindTo_OnCooldownEnded(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownEnded& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_DialogEmitter
{
    ck::UUtils_Signal_OnDialogCooldownEnded::Bind(InEmitter, InDelegate, InBindingPolicy);

    return InEmitter;
}

auto
    UCk_Utils_DialogEmitter_UE::
    UnbindFrom_OnCooldownEnded(
        FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownEnded& InDelegate)
    -> FCk_Handle_DialogEmitter
{
    ck::UUtils_Signal_OnDialogCooldownEnded::Unbind(InEmitter, InDelegate);

    return InEmitter;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogEmitter_UE::
    Get_BestLine(
        const FCk_DialogEmitter_QueryResult& InResult,
        int32 InSeed)
    -> FCk_DialogLine_QueryEntry
{
    auto PassedEntries = TArray<FCk_DialogLine_QueryEntry>{};
    for (const auto& Entry : InResult.Get_Entries())
    {
        if (Entry.Get_Result() == ECk_DialogLine_QueryResult::Passed)
        { PassedEntries.Emplace(Entry); }
    }

    if (PassedEntries.IsEmpty())
    { return {}; }

    auto MaxConditions = 0;
    for (const auto& Entry : PassedEntries)
    { MaxConditions = FMath::Max(MaxConditions, Entry.Get_NumConditions()); }

    const auto TopTier = PassedEntries.FilterByPredicate([&](const FCk_DialogLine_QueryEntry& InEntry)
    { return InEntry.Get_NumConditions() == MaxConditions; });

    auto Stream = FRandomStream{InSeed};
    const auto Index = Stream.RandRange(0, TopTier.Num() - 1);
    return TopTier[Index];
}

// --------------------------------------------------------------------------------------------------------------------
