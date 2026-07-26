#include "CkLagCompProjectile_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment.h"

#include "CkProjectile/Ballistic/CkBallistic_Utils.h"
#include "CkProjectile/BallisticMotion/CkBallisticMotion_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_LagCompProjectile_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::lag_comp_projectile_cvars
{
    // Lundgren, "Hit Registration in Networked First Person Shooters": rewinding by ping alone leaves
    // the server's view of a character slightly BEHIND what the shooter saw; a constant on top registers best
    static TAutoConsoleVariable<float> CVarRewindFudgeFactor(
        TEXT("Ck.LagComp.RewindFudgeFactor"),
        0.06f,
        TEXT("Constant added to the compensation window when rewinding, in seconds."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_LagCompProjectile_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            const FFragment_LagCompProjectile_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_LagCompProjectile_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests.Get_Requests(), Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_LagCompProjectile_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            const FCk_Request_LagCompProjectile_LaunchCompensated& InRequest)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        const auto Now = UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();

        const auto FudgeFactor = FCk_Time{
            static_cast<double>(lag_comp_projectile_cvars::CVarRewindFudgeFactor.GetValueOnGameThread())};
        auto Window = InRequest.Get_CompensationWindow() + FudgeFactor;

        CK_ENSURE_IF_NOT(Window >= FCk_Time::ZeroSecond(),
            TEXT("LaunchCompensated on [{}] resolved a NEGATIVE rewind window [{}] (request window [{}] + fudge [{}]) — launching uncompensated"),
            InHandle, Window, InRequest.Get_CompensationWindow(), FudgeFactor)
        { Window = FCk_Time::ZeroSecond(); }

        const auto LaunchTime = Now - Window;

        const auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        const auto InitialConditions = FCk_Ballistic_InitialConditions{
            LaunchTime, StartLocation, InRequest.Get_StartVelocity()};

        UCk_Utils_BallisticMotion_UE::Request_Launch(InHandle,
            FCk_Request_BallisticMotion_Launch{InRequest.Get_StartVelocity()}
                .Set_LaunchTimePolicy(ECk_BallisticMotion_LaunchTime::OverrideTime)
                .Set_OverrideStartTime(LaunchTime));

        const auto& TrajectoryParams = InParams.Get_TrajectoryParams();
        const auto SubstepSeconds = FMath::Max(InRequest.Get_SubstepTime().Get_Seconds(), 0.005);

        auto RewindHits = TArray<FCk_LagComp_RewindHit>{};

        InHandle.View<FFragment_RewindHistory_Params, FFragment_RewindHistory_Current>().ForEach(
        [&](FCk_Entity InHistoryEntity, FFragment_RewindHistory_Params&, FFragment_RewindHistory_Current& InHistoryCurrent)
        {
            auto HistoryHandle = ck::MakeHandle(InHistoryEntity, InHandle);

            if (HistoryHandle == InHandle || HistoryHandle == InRequest.Get_IgnoredHistoryEntity())
            { return; }

            const auto& Frames = InHistoryCurrent.Get_Frames();

            if (Frames.Get_Count() < 2)
            { return; }

            // Clamping to recorded history also bounds this loop against an oversized compensation window
            const auto OldestRecordedTime = Frames.Get_FrameAt(0).Get_WorldTime().Get_Seconds();
            const auto SweepStart = FMath::Max(LaunchTime.Get_Seconds(), OldestRecordedTime);

            for (auto SliceStart = SweepStart; SliceStart < Now.Get_Seconds(); SliceStart += SubstepSeconds)
            {
                const auto SliceEnd = FMath::Min(SliceStart + SubstepSeconds, Now.Get_Seconds());

                const auto SegmentStart = ballistics::Get_PositionAtTime(
                    InitialConditions, TrajectoryParams, FCk_Time{SliceStart} - LaunchTime);
                const auto SegmentEnd = ballistics::Get_PositionAtTime(
                    InitialConditions, TrajectoryParams, FCk_Time{SliceEnd} - LaunchTime);

                const auto Hit = lag_comp::Sweep_SegmentVsHistory(
                    Frames, SegmentStart, SegmentEnd,
                    FCk_Time{SliceStart}, FCk_Time{SliceEnd}, InRequest.Get_ProjectileRadius());

                if (NOT Hit.IsSet())
                { continue; }

                auto ConfirmedHit = *Hit;
                ConfirmedHit.Set_TargetEntity(HistoryHandle);

                RewindHits.Emplace(ConfirmedHit);

                UUtils_Signal_LagCompProjectile_OnRewindHit::Broadcast(InHandle,
                    MakePayload(InHandle, ConfirmedHit));

                // One hit per target — the projectile does not pass through
                break;
            }
        });

        UUtils_Signal_LagCompProjectile_OnLaunchCompensated::Broadcast(InHandle,
            MakePayload(InHandle, InitialConditions, RewindHits));
    }
}

// --------------------------------------------------------------------------------------------------------------------
