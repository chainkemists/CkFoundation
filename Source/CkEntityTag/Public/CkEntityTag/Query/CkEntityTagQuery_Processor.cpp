#include "CkEntityTagQuery_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"
#include "CkEntityTag/CkEntityTag_Log.h"
#include "CkEntityTag/CkEntityTag_Stats.h"
#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("EntityTagQuery::AppendScan"), STAT_EntityTagQuery_AppendScan, STATGROUP_CkEntityTag);
DECLARE_CYCLE_STAT(TEXT("EntityTagQuery::EnsureScan"), STAT_EntityTagQuery_EnsureScan, STATGROUP_CkEntityTag);
DECLARE_CYCLE_STAT(TEXT("EntityTagQuery::BuildPayload"), STAT_EntityTagQuery_BuildPayload, STATGROUP_CkEntityTag);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_Evaluate);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_TrackedEntity_Destructor);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_Query_Destructor);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityTagQuery_HandleRequests::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityTagQuery_Current& InCurrent,
            FFragment_EntityTagQuery_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_EntityTagQuery_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests.Get_Requests(),
                ck::Visitor([&](const auto& InEntry)
                {
                    DoHandleRequest(InCurrent, InEntry);
                }),
                ck::policy::DontResetContainer{});
        });
    }

    // ----

    auto
        FProcessor_EntityTagQuery_HandleRequests::
        DoHandleRequest(
            FFragment_EntityTagQuery_Current& InCurrent,
            const FCk_Request_EntityTagQuery_AddRequirement& InRequest)
        -> void
    {
        InCurrent._Requirements.Emplace(InRequest.Get_Requirement());
        InCurrent._ResultsPerRequirement.Emplace(TArray<FCk_Handle>{});
        InCurrent._PendingAdded.Emplace(TArray<FCk_Handle>{});
        InCurrent._PendingRemoved.Emplace(TArray<FCk_Handle>{});
    }

    auto
        FProcessor_EntityTagQuery_HandleRequests::
        DoHandleRequest(
            FFragment_EntityTagQuery_Current& InCurrent,
            const FCk_Request_EntityTagQuery_RemoveRequirement& InRequest)
        -> void
    {
        const auto Index = ck::algo::FindIndex(InCurrent._Requirements,
            [&InRequest](const FCk_EntityTagQuery_Requirement& R)
            {
                return R.Get_Tag() == InRequest.Get_Tag();
            });

        if (Index == INDEX_NONE)
        { return; }

        InCurrent._Requirements.RemoveAt(Index);
        if (Index < InCurrent._ResultsPerRequirement.Num())
        {
            InCurrent._ResultsPerRequirement.RemoveAt(Index);
        }
        if (Index < InCurrent._PendingAdded.Num())
        {
            InCurrent._PendingAdded.RemoveAt(Index);
        }
        if (Index < InCurrent._PendingRemoved.Num())
        {
            InCurrent._PendingRemoved.RemoveAt(Index);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityTagQuery_Evaluate::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityTagQuery_Current& InCurrent) const
        -> void
    {
        const auto& Requirements = InCurrent._Requirements;

        if (Requirements.Num() == 0)
        {
            InCurrent._IsSatisfied = false;
            return;
        }

        if (InCurrent._ResultsPerRequirement.Num() != Requirements.Num())
        {
            // HandleRequests should keep these in sync. Defensive resize.
            InCurrent._ResultsPerRequirement.SetNum(Requirements.Num());
        }
        if (InCurrent._PendingAdded.Num() != Requirements.Num())
        {
            InCurrent._PendingAdded.SetNum(Requirements.Num());
        }
        if (InCurrent._PendingRemoved.Num() != Requirements.Num())
        {
            InCurrent._PendingRemoved.SetNum(Requirements.Num());
        }

        auto AnyAppendedThisPass = false;

        for (int32 i = 0; i < Requirements.Num(); ++i)
        {
            const auto& Req     = Requirements[i];
            auto&       Results = InCurrent._ResultsPerRequirement[i];
            const auto  Tag     = Req.Get_Tag();

            auto& PendingRemoved = InCurrent._PendingRemoved[i];

            // Lazy prune: drop entries whose tag was removed (Request_TryRemove on a still-living entity).
            // Destruction is handled proactively by FProcessor_EntityTagQuery_TrackedEntity_Destructor
            // in the EndPlay group, but Eval may run before EndPlay in the same frame as the destruction.
            // Catch that window lazily so we never read tag state from a dying handle.
            // Record dropped entries into _PendingRemoved so listeners get the delta in the payload.
            Results.RemoveAll([&](const FCk_Handle& H)
            {
                if (ck::Is_NOT_Valid(H))
                {
                    PendingRemoved.AddUnique(H);
                    return true;
                }
                if (NOT UCk_Utils_EntityTag_UE::Has(H, Tag))
                {
                    PendingRemoved.AddUnique(H);
                    return true;
                }
                return false;
            });

            // Determine target cap by mode.
            const auto Cap = [&]() -> int32
            {
                switch (Req.Get_Mode())
                {
                    case ECk_EntityTagQuery_CountMode::SingleOnly: return 1;
                    case ECk_EntityTagQuery_CountMode::Count:      return Req.Get_Count();
                    case ECk_EntityTagQuery_CountMode::All:        return MAX_int32;
                    default:                                       return 1;
                }
            }();

            // Append new matches from the global per-tag storage view, up to cap.
            if (Results.Num() < Cap)
            {
                SCOPE_CYCLE_COUNTER(STAT_EntityTagQuery_AppendScan);

                FCk_Handle BaseHandle = InHandle;
                UCk_Utils_EntityTag_UE::ForEach_Entity(BaseHandle, Tag,
                    [&](FCk_Handle InEntity)
                    {
                        if (Results.Num() >= Cap)
                        { return; }

                        // Skip pending-kill entities. The storage view doesn't filter them, but we
                        // must not Add them to results (would persist a dying handle until the
                        // next prune) and must not AddOrGet on them (CK_ENSURE_IF_NOT in AddOrGet
                        // fires for pending-kill handles per CkHandle.h:619).
                        if (ck::Is_NOT_Valid(InEntity))
                        { return; }

                        if (Results.Contains(InEntity))
                        { return; }
                        Results.Add(InEntity);
                        AnyAppendedThisPass = true;
                        InCurrent._PendingAdded[i].AddUnique(InEntity);

                        // Register this query on the tagged entity so the destructor can clean us up.
                        auto& Tracked = InEntity.AddOrGet<FFragment_EntityTagQuery_TrackedByQueries>();
                        Tracked._Queries.AddUnique(InHandle);
                    });
            }
        }

        // Ensure-bound check (independent of satisfaction).
        {
            SCOPE_CYCLE_COUNTER(STAT_EntityTagQuery_EnsureScan);

            for (const auto& Req : Requirements)
            {
                const auto MaxAllowed = Req.Get_MaxAllowedEnsure();
                if (MaxAllowed <= FCk_EntityTagQuery_Requirement::NoEnsure)
                { continue; }

                FCk_Handle BaseHandle = InHandle;
                auto GlobalCount = int32{0};
                UCk_Utils_EntityTag_UE::ForEach_Entity(BaseHandle, Req.Get_Tag(),
                    [&](FCk_Handle) { ++GlobalCount; });

                CK_ENSURE_IF_NOT(GlobalCount <= MaxAllowed,
                    TEXT("EntityTagQuery [{}] requirement on tag [{}] exceeded MaxAllowed [{}]; current global count [{}]"),
                    InHandle, Req.Get_Tag(), MaxAllowed, GlobalCount)
                { /* no-op: continue evaluation */ }
            }
        }

        // Satisfaction predicate.
        const auto WasSatisfied   = InCurrent._IsSatisfied;
        auto       IsNowSatisfied = true;
        for (int32 i = 0; i < Requirements.Num(); ++i)
        {
            const auto& Req       = Requirements[i];
            const auto  N         = InCurrent._ResultsPerRequirement[i].Num();
            const auto  Threshold = [&]() -> int32
            {
                switch (Req.Get_Mode())
                {
                    case ECk_EntityTagQuery_CountMode::SingleOnly: return 1;
                    case ECk_EntityTagQuery_CountMode::Count:      return Req.Get_Count();
                    case ECk_EntityTagQuery_CountMode::All:        return 1;
                    default:                                       return 1;
                }
            }();

            if (N < Threshold)
            {
                IsNowSatisfied = false;
                break;
            }
        }

        InCurrent._IsSatisfied = IsNowSatisfied;

        if (IsNowSatisfied)
        {
            // Fire decision.
            auto AnyAllModeAppended = false;
            if (AnyAppendedThisPass)
            {
                for (const auto& Req : Requirements)
                {
                    if (Req.Get_Mode() == ECk_EntityTagQuery_CountMode::All)
                    {
                        AnyAllModeAppended = true;
                        break;
                    }
                }
            }

            const auto FirstTime      = NOT InCurrent._HasFiredOnce;
            const auto AllModeRefire  = InCurrent._HasFiredOnce && AnyAllModeAppended;
            const auto DropAndRecover = InCurrent._HasFiredOnce && NOT WasSatisfied;
            const auto ShouldFire     = FirstTime || AllModeRefire || DropAndRecover;

            if (ShouldFire)
            {
                // Build result payload (including per-requirement add/remove deltas accumulated this pass).
                auto Payload = TArray<FCk_EntityTagQuery_Result>{};
                {
                    SCOPE_CYCLE_COUNTER(STAT_EntityTagQuery_BuildPayload);

                    Payload.Reserve(Requirements.Num());
                    for (int32 i = 0; i < Requirements.Num(); ++i)
                    {
                        Payload.Emplace(FCk_EntityTagQuery_Result{
                            Requirements[i].Get_Tag(),
                            InCurrent._ResultsPerRequirement[i],
                            InCurrent._PendingAdded[i],
                            InCurrent._PendingRemoved[i]});
                    }
                }

                InCurrent._HasFiredOnce = true;

                ck::UUtils_Signal_EntityTagQuery_OnSatisfied::Broadcast(
                    InHandle,
                    ck::MakePayload(InHandle, Payload));
            }
        }

        // Continuous update (opt-in). Broadcasts ONLY on passes where the result set actually
        // changed this pass (an entity entered or left a requirement's results) — not every
        // pass. Every consumer reacts to the per-requirement _Added / _Removed deltas and
        // no-ops on empty deltas, so a no-change pass has nothing to deliver; skipping it
        // avoids a per-frame payload alloc + broadcast + delegate invocation per bound query
        // (the dominant cost when many queries each hold a continuous listener). A satisfaction
        // flip cannot happen without a result-count change, so _IsSatisfied transitions still
        // ride out on the same pass as their delta. Pay-for-what-you-use: skip entirely when
        // refcount is 0.
        if (InCurrent._ContinuousUpdateListenerCount > 0)
        {
            auto AnyRemovedThisPass = false;
            for (const auto& Removed : InCurrent._PendingRemoved)
            {
                if (Removed.Num() > 0)
                { AnyRemovedThisPass = true; break; }
            }

            if (AnyAppendedThisPass || AnyRemovedThisPass)
            {
                auto ContinuousPayload = TArray<FCk_EntityTagQuery_Result>{};
                ContinuousPayload.Reserve(Requirements.Num());
                for (int32 i = 0; i < Requirements.Num(); ++i)
                {
                    ContinuousPayload.Emplace(FCk_EntityTagQuery_Result{
                        Requirements[i].Get_Tag(),
                        InCurrent._ResultsPerRequirement[i],
                        InCurrent._PendingAdded[i],
                        InCurrent._PendingRemoved[i]});
                }

                ck::UUtils_Signal_EntityTagQuery_OnContinuousUpdate::Broadcast(
                    InHandle,
                    ck::MakePayload(InHandle, InCurrent._IsSatisfied, ContinuousPayload));
            }
        }

        // Reset accumulators at the end of every Evaluate pass. Intentional:
        // - OnContinuousUpdate fires (and consumes deltas) only on passes that changed; a
        //   no-change pass has empty _PendingAdded/_PendingRemoved, so nothing is dropped.
        // - OnSatisfied path captures deltas when it fires; otherwise they're dropped.
        // - The destructor writes _PendingRemoved in FGroup_EndPlay (after Eval), so
        //   those writes survive to next frame's Eval and get baked into that pass.
        for (auto& A : InCurrent._PendingAdded)   { A.Reset(); }
        for (auto& R : InCurrent._PendingRemoved) { R.Reset(); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityTagQuery_TrackedEntity_Destructor::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_EntityTagQuery_TrackedByQueries& InTracked) const
        -> void
    {
        for (const auto& QueryHandle : InTracked.Get_Queries())
        {
            // Query may already be dead (concurrent destruction).
            if (ck::Is_NOT_Valid(QueryHandle))
            { continue; }

            auto MutableQuery = QueryHandle;
            if (NOT MutableQuery.Has<FFragment_EntityTagQuery_Current>())
            { continue; }

            auto& Current = MutableQuery.Get<FFragment_EntityTagQuery_Current>();

            // Remove the dying entity from every result array in this query.
            // Record the removal into _PendingRemoved at the matching index so the
            // next Evaluate pass bakes it into the payload before resetting.
            for (int32 i = 0; i < Current._ResultsPerRequirement.Num(); ++i)
            {
                auto& Results = Current._ResultsPerRequirement[i];

                const auto RemovedCount = Results.RemoveAll([&](const FCk_Handle& H)
                {
                    return H == InHandle;
                });

                if (RemovedCount > 0 && i < Current._PendingRemoved.Num())
                {
                    Current._PendingRemoved[i].AddUnique(FCk_Handle{InHandle});
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityTagQuery_Query_Destructor::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_EntityTagQuery_Current& InCurrent) const
        -> void
    {
        // For each entity this query currently tracks, remove the query from its _Queries list.
        for (const auto& Results : InCurrent.Get_ResultsPerRequirement())
        {
            for (const auto& TaggedEntity : Results)
            {
                if (ck::Is_NOT_Valid(TaggedEntity))
                { continue; }

                auto Mutable = TaggedEntity;
                if (NOT Mutable.Has<FFragment_EntityTagQuery_TrackedByQueries>())
                { continue; }

                auto& Tracked = Mutable.Get<FFragment_EntityTagQuery_TrackedByQueries>();
                Tracked._Queries.RemoveAll([&](const FCk_Handle_EntityTagQuery& Q)
                {
                    return Q == InHandle;
                });

                if (Tracked._Queries.Num() == 0)
                {
                    Mutable.Try_Remove<FFragment_EntityTagQuery_TrackedByQueries>();
                }
            }
        }
    }
}
