#include "CkTargeter_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"


#include "CkNet/CkNet_Utils.h"
#include "CkTargeting/CkTargeting_Log.h"
#include "Targetable/CkTargetable_Fragment.h"
#include "Targetable/CkTargetable_Utils.h"
#include "Targeter/CkTargeter_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Targeter_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams)
        -> void
    {
        UCk_Utils_Targeter_UE::Request_QueryTargets(InHandle, {}, {});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Targeter_Cleanup::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Targeter_Cleanup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_Targeter_Current& InCurrent)
        -> void
    {
        targeting::VeryVerbose(TEXT("Resetting Cached Targets from Targeter [{}]"), InHandle);
        InCurrent._CachedTargets.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Targeter_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent,
            FFragment_Targeter_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InParams, InCurrent, InRequestVariant);
        }));

        if (ck::Is_NOT_Valid(InHandle))
        {
            targeting::Verbose(TEXT("Targeter Entity [{}] was Marked Pending Kill during the handling of its requests."), InHandle);
            return;
        }

        if (InRequests.Get_Requests().IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_Targeter_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent,
            const FCk_Request_Targeter_QueryTargets& InRequest) const
        -> void
    {
        const auto TargeterBasicInfo = FCk_Targeter_BasicInfo{InHandle, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle)};

        if (InCurrent.Get_CachedTargets().IsSet())
        {
            UUtils_Signal_Targeter_OnTargetsQueried::Broadcast(InHandle, MakePayload(TargeterBasicInfo, *InCurrent.Get_CachedTargets()));
            return;
        }

        const auto& CustomTargetFilter = InParams.Get_Params().Get_CustomTargetFilter();

        CK_ENSURE_IF_NOT(ck::IsValid(CustomTargetFilter),
            TEXT("Targeter [{}] does NOT have a valid Custom Target Filter"),
            InHandle)
        { return; }

        auto ValidTargets = TArray<FCk_Targetable_BasicInfo>{};

        InHandle.View<FFragment_Targetable_Params, FFragment_Targetable_Current, CK_IGNORE_PENDING_KILL>()
        .ForEach([&](FCk_Entity InEntity, const FFragment_Targetable_Params&, const FFragment_Targetable_Current&)
        {
            const auto Handle = ck::MakeHandle(InEntity, InHandle);
            const auto Targetable = UCk_Utils_Targetable_UE::Cast(Handle);

            if (NOT UCk_Utils_Targeter_UE::Get_CanTarget(InHandle, Targetable))
            { return; }

            ValidTargets.Add(FCk_Targetable_BasicInfo{Targetable, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Targetable)});
        });

        const auto ValidTargetList  = FCk_Targeter_TargetList{ValidTargets};
        const auto& FilteredTargets = CustomTargetFilter->FilterTargets(TargeterBasicInfo, ValidTargetList);
        const auto& SortedTargets   = CustomTargetFilter->SortTargets(TargeterBasicInfo, FilteredTargets);

        InCurrent._CachedTargets = SortedTargets;

        UCk_Utils_Targeter_UE::Request_Cleanup(InHandle);

        UUtils_Signal_Targeter_OnTargetsQueried::Broadcast(InHandle, MakePayload(TargeterBasicInfo, *InCurrent._CachedTargets));
    }
}

// --------------------------------------------------------------------------------------------------------------------
