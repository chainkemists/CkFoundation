#include "CkEntityTagQuery_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"
#include "CkEntityTag/CkEntityTag_Log.h"
#include "CkEntityTag/CkEntityTag_Stats.h"
#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("EntityTagQuery::AppendScan"), STAT_EntityTagQuery_AppendScan, STATGROUP_CkEntityTag);
DECLARE_CYCLE_STAT(TEXT("EntityTagQuery::EnsureScan"), STAT_EntityTagQuery_EnsureScan, STATGROUP_CkEntityTag);
DECLARE_CYCLE_STAT(TEXT("EntityTagQuery::BuildPayload"), STAT_EntityTagQuery_BuildPayload, STATGROUP_CkEntityTag);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_CancelPendingRequests);
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
                    // DoHandleRequest is void and has no rejection path, so reaching the line after the
                    // call IS the success condition.
                    auto Result = ECk_Request_OperationResult::Failed;
                    const auto Guard = MakeCompletionGuard(InEntry, InHandle, Result);

                    DoHandleRequest(InCurrent, InEntry);

                    Result = ECk_Request_OperationResult::Succeeded;
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

        // A new requirement has no cached results, and its tag version snapshot does not exist yet.
        InCurrent._NeedsEvaluate = true;
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

        // Satisfaction is computed across every requirement, so dropping one can flip it without
        // any tag having mutated.
        InCurrent._NeedsEvaluate = true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityTagQuery_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityTagQuery_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityTagQuery_Evaluate::
        DoTryConsume_UnchangedTagVersions(
            HandleType InHandle,
            FFragment_EntityTagQuery_Current& InCurrent)
        -> bool
    {
        const auto& Requirements = InCurrent._Requirements;
        auto& LastSeen = InCurrent._LastSeenTagVersionPerRequirement;

        const auto Registry = InHandle.Get_RegistryView();

        const auto Get_TagVersion = [&Registry](const FCk_EntityTagQuery_Requirement& InRequirement) -> uint64
        {
            return Registry.Get_DirtyMarkerVersion(entt::id_type{GetTypeHash(InRequirement.Get_Tag())});
        };

        const auto VersionsAreUnchanged = [&]() -> bool
        {
            if (InCurrent._NeedsEvaluate)
            { return false; }

            if (LastSeen.Num() != Requirements.Num())
            { return false; }

            for (auto Index = 0; Index < Requirements.Num(); ++Index)
            {
                if (LastSeen[Index] != Get_TagVersion(Requirements[Index]))
                { return false; }
            }

            return true;
        }();

        if (VersionsAreUnchanged)
        { return true; }

        // Stamped BEFORE the pass runs: a tag mutated DURING the pass (e.g. by an inline signal
        // handler) must leave its version ahead of this snapshot so the next pass re-runs.
        LastSeen.Reset();
        ck::algo::Transform(Requirements, ck::algo::ToTransform(LastSeen), Get_TagVersion);

        InCurrent._NeedsEvaluate = false;

        return false;
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

        // HandleRequests keeps these in sync; the resizes are defensive.
        if (InCurrent._ResultsPerRequirement.Num() != Requirements.Num())
        {
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

        // The tail of this function is already delta-only: nothing below can change a result set,
        // flip satisfaction, or broadcast unless an entity gained or lost one of this query's tags.
        // Unchanged versions therefore make the whole pass — prune, append, ensure — a provable no-op.
        if (DoTryConsume_UnchangedTagVersions(InHandle, InCurrent))
        { return; }

        auto AnyAppendedThisPass = false;

        for (int32 i = 0; i < Requirements.Num(); ++i)
        {
            const auto& Req     = Requirements[i];
            auto&       Results = InCurrent._ResultsPerRequirement[i];
            const auto  Tag     = Req.Get_Tag();

            auto& PendingRemoved = InCurrent._PendingRemoved[i];

            // TrackedEntity_Destructor prunes destroyed entities proactively in FGroup_EndPlay, but Eval can run
            // before EndPlay in the frame of the destruction — catch that window here so we never read tag state
            // from a dying handle.
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

            if (Results.Num() < Cap)
            {
                SCOPE_CYCLE_COUNTER(STAT_EntityTagQuery_AppendScan);

                FCk_Handle BaseHandle = InHandle;
                UCk_Utils_EntityTag_UE::ForEach_Entity(BaseHandle, Tag,
                    [&](FCk_Handle InEntity)
                    {
                        if (Results.Num() >= Cap)
                        { return; }

                        // The storage view does not filter pending-kill entities, and AddOrGet ensures on one.
                        if (ck::Is_NOT_Valid(InEntity))
                        { return; }

                        if (Results.Contains(InEntity))
                        { return; }
                        Results.Add(InEntity);
                        AnyAppendedThisPass = true;
                        InCurrent._PendingAdded[i].AddUnique(InEntity);

                        auto& Tracked = InEntity.AddOrGet<FFragment_EntityTagQuery_TrackedByQueries>();
                        Tracked._Queries.AddUnique(InHandle);
                    });
            }
        }

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

        // Delta-only broadcast: a satisfaction flip cannot happen without a result-count change, so skipping
        // no-change passes still delivers every _IsSatisfied transition. See CkEntityTag/CLAUDE.md § "Query system".
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

        // Deltas not consumed by a fire this pass are dropped, deliberately. The destructor writes _PendingRemoved
        // in FGroup_EndPlay (after Eval), so those writes survive to next frame's pass.
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
            if (ck::Is_NOT_Valid(QueryHandle))
            { continue; }

            auto MutableQuery = QueryHandle;
            if (NOT MutableQuery.Has<FFragment_EntityTagQuery_Current>())
            { continue; }

            auto& Current = MutableQuery.Get<FFragment_EntityTagQuery_Current>();

            // Recorded into _PendingRemoved so the next Evaluate pass bakes it into the payload before resetting.
            for (int32 i = 0; i < Current._ResultsPerRequirement.Num(); ++i)
            {
                auto& Results = Current._ResultsPerRequirement[i];

                const auto RemovedCount = Results.RemoveAll([&](const FCk_Handle& H)
                {
                    return H == InHandle;
                });

                if (RemovedCount == 0)
                { continue; }

                // An entity dying mutates no tag storage, so nothing else re-arms the version gate.
                // Deliberately outside the bounds guard below: inside it, a defensive array mismatch
                // would strand _IsSatisfied stale with every later pass skipped.
                Current._NeedsEvaluate = true;

                if (i < Current._PendingRemoved.Num())
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
