#include "CkEntityTag_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityTag/CkEntityTag_Log.h"
#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTag_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityTag_BroadcastOnDestroy);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityTag_HandleRequests::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityTag_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_EntityTag_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests.Get_Requests(),
                ck::Visitor([&](const auto& InEntry)
                {
                    DoHandleRequest(InHandle, InEntry);
                }),
                ck::policy::DontResetContainer{});
        });
    }

    // ----

    auto
        FProcessor_EntityTag_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_Add& InRequest)
        -> void
    {
        UCk_Utils_EntityTag_UE::DoApply_Add(InHandle, InRequest.Get_Tag());
    }

    auto
        FProcessor_EntityTag_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_TryRemove& InRequest)
        -> void
    {
        UCk_Utils_EntityTag_UE::DoApply_TryRemove(InHandle, InRequest.Get_Tag());
    }

    auto
        FProcessor_EntityTag_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_AddGameplayTag& InRequest)
        -> void
    {
        UCk_Utils_EntityTag_UE::DoApply_AddGameplayTag(InHandle, InRequest.Get_Tag());
    }

    auto
        FProcessor_EntityTag_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_TryRemoveGameplayTag& InRequest)
        -> void
    {
        UCk_Utils_EntityTag_UE::DoApply_TryRemoveGameplayTag(InHandle, InRequest.Get_Tag());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityTag_BroadcastOnDestroy::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_EntityTag_Current& InCurrent) const
        -> void
    {
        auto MutatedEntity = InHandle;
        for (const auto& TagCount : InCurrent.Get_Tags())
        {
            UCk_Utils_EntityTag_UE::DoBroadcast_AnyEntityListeners(
                MutatedEntity, TagCount._Name, ECk_EntityTagUpdate::Removed);
        }
    }
}
