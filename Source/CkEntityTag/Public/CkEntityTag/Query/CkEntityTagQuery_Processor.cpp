#include "CkEntityTagQuery_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"
#include "CkEntityTag/CkEntityTag_Log.h"
#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

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

        auto AnyAppendedThisPass = false;

        for (int32 i = 0; i < Requirements.Num(); ++i)
        {
            const auto& Req     = Requirements[i];
            auto&       Results = InCurrent._ResultsPerRequirement[i];
            const auto  Tag     = Req.Get_Tag();

            // Lazy prune: drop entries whose tag was removed (Request_TryRemove on a still-living entity).
            // Destruction is handled proactively by FProcessor_EntityTagQuery_TrackedEntity_Destructor.
            Results.RemoveAll([&](const FCk_Handle& H)
            {
                return NOT UCk_Utils_EntityTag_UE::Has(H, Tag);
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
                FCk_Handle BaseHandle = InHandle;
                UCk_Utils_EntityTag_UE::ForEach_Entity(BaseHandle, Tag,
                    [&](FCk_Handle InEntity)
                    {
                        if (Results.Num() >= Cap)
                        { return; }
                        if (Results.Contains(InEntity))
                        { return; }
                        Results.Add(InEntity);
                        AnyAppendedThisPass = true;

                        // Register this query on the tagged entity so the destructor can clean us up.
                        auto& Tracked = InEntity.AddOrGet<FFragment_EntityTagQuery_TrackedByQueries>();
                        Tracked._Queries.AddUnique(InHandle);
                    });
            }
        }

        // Ensure-bound check (independent of satisfaction).
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

        if (NOT IsNowSatisfied)
        { return; }

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

        if (NOT ShouldFire)
        { return; }

        // Build result payload.
        auto Payload = TArray<FCk_EntityTagQuery_Result>{};
        Payload.Reserve(Requirements.Num());
        for (int32 i = 0; i < Requirements.Num(); ++i)
        {
            Payload.Emplace(FCk_EntityTagQuery_Result{
                Requirements[i].Get_Tag(),
                InCurrent._ResultsPerRequirement[i]});
        }

        InCurrent._HasFiredOnce = true;

        ck::UUtils_Signal_EntityTagQuery_OnSatisfied::Broadcast(
            InHandle,
            ck::MakePayload(InHandle, Payload));
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
            for (auto& Results : Current._ResultsPerRequirement)
            {
                Results.RemoveAll([&](const FCk_Handle& H)
                {
                    return H == InHandle;
                });
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
