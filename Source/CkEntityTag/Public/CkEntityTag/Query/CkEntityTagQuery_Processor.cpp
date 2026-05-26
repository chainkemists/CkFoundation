#include "CkEntityTagQuery_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityTag/CkEntityTag_Fragment.h"
#include "CkEntityTag/CkEntityTag_Log.h"
#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTagQuery_Evaluate);

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

            // Lazy prune: drop entries that are no longer valid OR no longer carry the tag.
            Results.RemoveAll([&](const FCk_Handle& H)
            {
                return ck::Is_NOT_Valid(H) || NOT UCk_Utils_EntityTag_UE::Has(H, Tag);
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
                    });
            }
        }

        // C3 adds the ensure-check, satisfaction predicate, and fire decision below.
        // For now, mark satisfied as false unconditionally; C3 will compute it properly.
        (void)AnyAppendedThisPass;
        InCurrent._IsSatisfied = false;
    }
}
