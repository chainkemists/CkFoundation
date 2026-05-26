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
        // TODO(C2/C3): lazy-prune, scan storage, ensure-bound, fire decision.
    }
}
