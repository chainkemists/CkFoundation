#include "CkVisibleRange_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Chrono/CkCadence_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVisibleRange/CkVisibleRange_Log.h"
#include "CkVisibleRange/CkVisibleRange_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VisibleRange_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisibleRange_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VisibleRange_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisibleRange_Params& InParams,
            FFragment_VisibleRange_Current& InCurrent)
        -> void
    {
        if (NOT ck::cadence::ShouldRun(InCurrent._CadenceChrono, InDeltaT))
        { return; }

        InCurrent._FadeAlpha = UCk_Utils_VisibleRange_UE::Compute_FadeAlpha(
            InParams.Get_MinRange(), InParams.Get_MaxRange(), InParams.Get_FadeBandCm(), InCurrent._Distance);

        const auto IsOutOfRange = NOT UCk_Utils_VisibleRange_UE::Compute_IsInRange(
            InParams.Get_MinRange(), InParams.Get_MaxRange(), InCurrent._Distance);

        if (IsOutOfRange == InCurrent._IsOutOfRange)
        { return; }

        visiblerange::VeryVerbose(TEXT("VisibleRange own-range boundary crossed for Entity [{}]. IsOutOfRange [{}]"), InHandle, IsOutOfRange);

        InHandle.AddOrGet<FFragment_VisibleRange_Requests>()._Requests.Emplace(
            FCk_Request_VisibleRange_ApplyRangeState{IsOutOfRange});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisibleRange_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            FFragment_VisibleRange_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InHandle, InCurrent, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_VisibleRange_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            const FCk_Request_VisibleRange_ApplyRangeState& InRequest)
        -> void
    {
        const auto NewIsOutOfRange = InRequest.Get_IsOutOfRange();

        if (NewIsOutOfRange == InCurrent._IsOutOfRange)
        { return; }

        const auto WasVisible = NOT InHandle.Has<FTag_VisibleRange_Hidden>();

        if (NewIsOutOfRange)
        { InHandle.Add<FTag_VisibleRange_Hidden>(); }
        else
        { InHandle.Remove<FTag_VisibleRange_Hidden>(); }

        InCurrent._IsOutOfRange = NewIsOutOfRange;

        const auto IsVisible = NOT InHandle.Has<FTag_VisibleRange_Hidden>();

        if (WasVisible != IsVisible)
        {
            UUtils_Signal_OnVisibleRange_HiddenChanged::Broadcast(InHandle,
                MakePayload(ck::StaticCast<FCk_Handle_VisibleRange>(InHandle), NOT IsVisible));
        }
    }

    auto
        FProcessor_VisibleRange_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            const FCk_Request_VisibleRange_SetVisibility& InRequest)
        -> void
    {
        const auto NewIsExplicitlyHidden = InRequest.Get_ShowHide() == ECk_VisibleRange_ShowHide::Hide;

        if (NewIsExplicitlyHidden == InCurrent._IsExplicitlyHidden)
        { return; }

        const auto WasVisible = NOT InHandle.Has<FTag_VisibleRange_Hidden>();

        if (NewIsExplicitlyHidden)
        { InHandle.Add<FTag_VisibleRange_Hidden>(); }
        else
        { InHandle.Remove<FTag_VisibleRange_Hidden>(); }

        InCurrent._IsExplicitlyHidden = NewIsExplicitlyHidden;

        const auto IsVisible = NOT InHandle.Has<FTag_VisibleRange_Hidden>();

        if (WasVisible != IsVisible)
        {
            UUtils_Signal_OnVisibleRange_HiddenChanged::Broadcast(InHandle,
                MakePayload(ck::StaticCast<FCk_Handle_VisibleRange>(InHandle), NOT IsVisible));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
