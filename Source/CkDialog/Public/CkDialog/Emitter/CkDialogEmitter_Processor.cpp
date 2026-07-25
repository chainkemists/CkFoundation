#include "CkDialogEmitter_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkDialog/CkDialog_Log.h"
#include "CkDialog/CkDialog_Subsystem.h"
#include "CkDialog/Common/CkDialog_QueryHelpers.h"
#include "CkDialog/Line/CkDialogLine_Utils.h"
#include "CkDialog/Settings/CkDialog_ProjectSettings.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_DialogEmitter_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_DialogEmitter_EvaluateQueries);
CK_REGISTER_PROCESSOR(ck::FProcessor_DialogEmitter_TickCooldowns);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_DialogEmitter_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEmitter,
            const FFragment_DialogEmitter_Params& InParams,
            FFragment_DialogEmitter_Requests& InRequests) const
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InEmitter);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto Now = FDialog_QueryHelpers::Get_WorldTimeNow(World);

        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InEmitter, Now, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            { InRequest.GetAndDestroyRequestHandle(); }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InEmitter.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_DialogEmitter_HandleRequests::
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_Query& InRequest)
        -> void
    {
        // Benign drop: an invalid ENTER tag matches nothing, so fall-through would only enqueue a dead query. Safe to
        // collapse the recovery into the ensure body.
        CK_ENSURE_IF_NOT(InRequest.Get_EventTag().IsValid(),
            TEXT("Dialog Query on Emitter [{}] has an invalid ENTER tag — dropped"), InEmitter)
        { return; }

        auto& Pending = InEmitter.AddOrGet<FFragment_DialogEmitter_PendingQueries>();
        Pending._Queries.Emplace(InRequest);

        if (NOT Pending._HasOldestPendingQueryTime)
        {
            Pending._OldestPendingQueryTime = InNow;
            Pending._HasOldestPendingQueryTime = true;
        }
    }

    auto
        FProcessor_DialogEmitter_HandleRequests::
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_StartCooldown& InRequest)
        -> void
    {
        const auto& Line = InRequest.Get_Line();

        // Benign drop: an invalid line handle would only add a junk cooldown entry that the next prune discards. Safe
        // to collapse the recovery into the ensure body.
        CK_ENSURE_IF_NOT(ck::IsValid(Line),
            TEXT("Dialog StartCooldown on Emitter [{}] has an invalid line handle — dropped"), InEmitter)
        { return; }

        const auto Expiry = InRequest.Get_DurationMode() == ECk_Dialog_CooldownDuration::Forever
            ? FDialog_CooldownSentinels::Forever()
            : InNow + InRequest.Get_Duration();

        InEmitter.Get<FFragment_DialogEmitter_Cooldowns>()._Cooldowns.Add(Line,
            FCk_DialogEmitter_CooldownEntry{Expiry, InRequest.Get_Duration(), InRequest.Get_DurationMode()});

        // The ticker's view key. Adding it per start is idempotent and cheaper than tracking emptiness here.
        InEmitter.AddOrGet<FTag_DialogEmitter_HasCooldowns>();

        UUtils_Signal_OnDialogCooldownStarted::Broadcast(InEmitter, ck::MakePayload(InEmitter, Line));

        if (const auto& OnComplete = InRequest.Get_OnComplete();
            OnComplete.IsBound())
        { OnComplete.Execute(InEmitter, Line); }
    }

    auto
        FProcessor_DialogEmitter_HandleRequests::
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_ClearCooldown& InRequest)
        -> void
    {
        // Removing an absent key is a harmless no-op — but it is NOT a transition, so neither the signal nor the
        // completion delegate fires for it. A caller told "cleared" when nothing was cooling would be misled.
        auto& Cooldowns = InEmitter.Get<FFragment_DialogEmitter_Cooldowns>();
        const auto Line = InRequest.Get_Line();

        if (Cooldowns._Cooldowns.Remove(Line) == 0)
        { return; }

        if (Cooldowns._Cooldowns.IsEmpty())
        { InEmitter.Try_Remove<FTag_DialogEmitter_HasCooldowns>(); }

        UUtils_Signal_OnDialogCooldownEnded::Broadcast(InEmitter, ck::MakePayload(InEmitter, Line));

        if (const auto& OnComplete = InRequest.Get_OnComplete();
            OnComplete.IsBound())
        { OnComplete.Execute(InEmitter, Line); }
    }

    auto
        FProcessor_DialogEmitter_HandleRequests::
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_ClearAllCooldowns& InRequest)
        -> void
    {
        auto& Cooldowns = InEmitter.Get<FFragment_DialogEmitter_Cooldowns>();

        // Snapshot the keys before emptying: each line still gets its own Ended signal, so an observer watching one
        // line cannot miss the transition just because it happened as part of a bulk clear.
        auto Cleared = TArray<FCk_Handle_DialogLine>{};
        Cooldowns._Cooldowns.GenerateKeyArray(Cleared);
        Cooldowns._Cooldowns.Empty();

        InEmitter.Try_Remove<FTag_DialogEmitter_HasCooldowns>();

        for (const auto& Line : Cleared)
        { UUtils_Signal_OnDialogCooldownEnded::Broadcast(InEmitter, ck::MakePayload(InEmitter, Line)); }

        if (const auto& OnComplete = InRequest.Get_OnComplete();
            OnComplete.IsBound())
        { OnComplete.Execute(InEmitter, Cleared.Num()); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_DialogEmitter_EvaluateQueries::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEmitter,
            const FFragment_DialogEmitter_Params& InParams,
            FFragment_DialogEmitter_PendingQueries& InPending) const
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InEmitter);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto Now = FDialog_QueryHelpers::Get_WorldTimeNow(World);

        auto* Registry = World->GetSubsystem<UCk_DialogRegistry_Subsystem_UE>();
        const auto RegistryReady = ck::IsValid(Registry) && Registry->Get_IsReady();

        if (NOT RegistryReady)
        {
            // Deferred: leave the pending-queries fragment in place (idempotent). Log once; ensure once past the
            // settings timeout (a never-ready registry is a misconfiguration, not silence).
            if (NOT InPending._LoggedNotReadyOnce)
            {
                dialog::Display(TEXT("Dialog Emitter [{}] deferring query: Dialog registry not ready yet"), InEmitter);
                InPending._LoggedNotReadyOnce = true;
            }

            if (InPending._HasOldestPendingQueryTime)
            {
                const auto Timeout = UCk_Utils_Dialog_Settings_UE::Get_QueryReadinessTimeoutSeconds();
                const auto Waited = Now - InPending._OldestPendingQueryTime;
                const auto WithinTimeout = NOT (Timeout < Waited);

                CK_ENSURE_IF_NOT(WithinTimeout,
                    TEXT("Dialog Emitter [{}] query still deferred after [{}] — the Dialog registry never became "
                         "ready. Check that banks are configured/registered."), InEmitter, Timeout)
                {}
            }

            return;
        }

        // Pruning belongs to FProcessor_DialogEmitter_TickCooldowns, which runs before this one and is the only
        // place expiry is observed. Classification compares against Now anyway, so a not-yet-pruned entry that has
        // already lapsed still classifies correctly here.
        const auto& Cooldowns = InEmitter.Get<FFragment_DialogEmitter_Cooldowns>();

        // Evaluate from a COPY: a per-request OnComplete delegate below runs caller code that may enqueue a follow-up
        // query (Request_QueryFollowUp). That lands in FFragment_DialogEmitter_Requests, not here, but iterating the
        // live array while arbitrary caller code runs is a reentrancy hazard not worth taking.
        const auto QueriesCopy = InPending._Queries;

        for (const auto& Query : QueriesCopy)
        {
            const auto Result = DoEvaluateQuery(InEmitter, InParams, Cooldowns, *Registry, Query, Now);

            DoRecordDebug(InEmitter, Result, Now);

            UUtils_Signal_OnDialogQueryCompleted::Broadcast(InEmitter, ck::MakePayload(InEmitter, Result));

            // Per-request continuation, correlated to THIS query — fired after the broadcast so emitter-wide observers
            // see the result before the requester can chain a follow-up. Executed directly rather than bound through
            // the signal (the CkEqs approach): CkEqs owns a per-query entity whose signal it can bind + auto-unbind,
            // whereas OnDialogQueryCompleted is emitter-wide, so a bind-per-iteration would deliver query N's payload
            // to every delegate still bound when the payload flushes.
            if (const auto& OnComplete = Query.Get_OnComplete();
                OnComplete.IsBound())
            { OnComplete.Execute(InEmitter, Result); }
        }

        // Draining the fragment discards the queries + readiness-defer bookkeeping in one step (fresh next query).
        InEmitter.Remove<FFragment_DialogEmitter_PendingQueries>();
    }

    auto
        FProcessor_DialogEmitter_EvaluateQueries::
        DoEvaluateQuery(
            HandleType InEmitter,
            const FFragment_DialogEmitter_Params& InParams,
            const FFragment_DialogEmitter_Cooldowns& InCooldowns,
            const UCk_DialogRegistry_Subsystem_UE& InRegistry,
            const FCk_Request_DialogEmitter_Query& InQuery,
            FCk_Time InNow)
        -> FCk_DialogEmitter_QueryResult
    {
        auto Result = FCk_DialogEmitter_QueryResult{InQuery.Get_EventTag()};
        auto Entries = TArray<FCk_DialogLine_QueryEntry>{};

        const auto& EmitterTags = InParams.Get_EmitterTags();
        const auto& ExtraFilterTags = InQuery.Get_ExtraFilterTags();

        // Candidates: entities filed under the ENTER tag in per-tag EnTT storage (hierarchy included).
        UCk_Utils_EntityTag_UE::ForEach_Entity_UsingGameplayTag(InEmitter, InQuery.Get_EventTag(),
        [&](FCk_Handle InCandidate)
        {
            if (ck::Is_NOT_Valid(InCandidate))
            { return; }

            if (NOT UCk_Utils_DialogLine_UE::Has(InCandidate))
            { return; }

            auto Line = UCk_Utils_DialogLine_UE::Cast(InCandidate);

            const auto FilterTags = UCk_Utils_DialogLine_UE::Get_FilterTags(Line);
            if (NOT FDialog_QueryHelpers::Get_IsLineVisible(FilterTags, EmitterTags, ExtraFilterTags))
            { return; }

            const auto LineResult = DoClassifyLine(InEmitter, InCooldowns, Line);
            Entries.Emplace(FDialog_QueryHelpers::Make_QueryEntry(Line, LineResult));
        });

        FDialog_QueryHelpers::Sort_Entries(Entries, FDialog_QueryHelpers::Get_ResolvedSortPolicy(InParams, InQuery));

        Result.Set_Entries(MoveTemp(Entries));
        return Result;
    }

    auto
        FProcessor_DialogEmitter_EvaluateQueries::
        DoClassifyLine(
            HandleType InEmitter,
            const FFragment_DialogEmitter_Cooldowns& InCooldowns,
            const FCk_Handle_DialogLine& InLine)
        -> ECk_DialogLine_QueryResult
    {
        // Cooldown FIRST (cheap map lookup): a cooling line reports Fail_EmitterCondition and its conditions are
        // skipped (documented precedence). Compared against Now rather than trusting presence, so an entry the
        // ticker has not retired yet cannot wrongly suppress a line that has in fact lapsed.
        if (const auto* Entry = InCooldowns.Get_Cooldowns().Find(InLine);
            Entry != nullptr && FDialog_QueryHelpers::Get_WorldTimeNow(InEmitter) < Entry->Get_Expiry())
        { return ECk_DialogLine_QueryResult::Fail_EmitterCondition; }

        auto Failed = false;
        UCk_Utils_DialogLine_UE::ForEach_Condition(InLine, [&](FCk_Handle InConditionEntity)
        {
            if (Failed)
            { return; }

            if (FDialog_QueryHelpers::Get_ConditionResult(InConditionEntity, InLine, InEmitter)
                == ECk_Dialog_ConditionResult::Fail)
            { Failed = true; }
        });

        return Failed
            ? ECk_DialogLine_QueryResult::Fail_LineCondition
            : ECk_DialogLine_QueryResult::Passed;
    }

    auto
        FProcessor_DialogEmitter_EvaluateQueries::
        DoRecordDebug(
            HandleType InEmitter,
            const FCk_DialogEmitter_QueryResult& InResult,
            FCk_Time InNow)
        -> void
    {
        if (NOT InEmitter.Has<FFragment_DialogEmitter_Debug>())
        { return; }

        const auto Cap = FMath::Max(1, UCk_Utils_Dialog_Settings_UE::Get_DebugHistorySize());

        auto& Debug = InEmitter.Get<FFragment_DialogEmitter_Debug>();
        auto Entry = FDialogEmitter_DebugEntry{};
        Entry._Result = InResult;
        Entry._Timestamp = InNow;
        Debug._History.Emplace(MoveTemp(Entry));

        while (Debug._History.Num() > Cap)
        { Debug._History.RemoveAt(0); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_DialogEmitter_TickCooldowns::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEmitter,
            FFragment_DialogEmitter_Cooldowns& InCooldowns) const
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InEmitter);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto Now = FDialog_QueryHelpers::Get_WorldTimeNow(World);

        // Collect before broadcasting: a handler is free to start or clear a cooldown on this same emitter, and
        // mutating the map while iterating it would be a use-after-invalidate.
        //
        // The Forever sentinel is FCk_Time::Max, so a Forever cooldown never satisfies Now >= Expiry and is retained.
        // A line whose entity died is retired silently — there is nothing left for an observer to act on.
        auto Lapsed = TArray<FCk_Handle_DialogLine>{};
        for (auto It = InCooldowns._Cooldowns.CreateIterator(); It; ++It)
        {
            const auto LineInvalid = ck::Is_NOT_Valid(It.Key());
            const auto Expired = NOT (Now < It.Value().Get_Expiry());

            if (NOT LineInvalid && NOT Expired)
            { continue; }

            if (NOT LineInvalid)
            { Lapsed.Emplace(It.Key()); }

            It.RemoveCurrent();
        }

        if (InCooldowns._Cooldowns.IsEmpty())
        { InEmitter.Try_Remove<FTag_DialogEmitter_HasCooldowns>(); }

        for (const auto& Line : Lapsed)
        { UUtils_Signal_OnDialogCooldownEnded::Broadcast(InEmitter, ck::MakePayload(InEmitter, Line)); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
