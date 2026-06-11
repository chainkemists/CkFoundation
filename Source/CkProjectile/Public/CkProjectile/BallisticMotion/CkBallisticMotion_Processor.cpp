#include "CkBallisticMotion_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkProjectile/Ballistic/CkBallistic_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_BallisticMotion_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_BallisticMotion_HandleImpacts);
CK_REGISTER_PROCESSOR(ck::FProcessor_BallisticMotion_UpdateTrajectory);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ballistic_motion_detail
{
    static auto
    Get_CurrentWorldTime(
        const FCk_Handle& InHandle) -> FCk_Time
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        return UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();
    }

    static auto
    DoBroadcastStopped(
        FCk_Handle_BallisticMotion& InHandle,
        const FVector& InFinalLocation) -> void
    {
        InHandle.Try_Remove<FTag_BallisticMotion_Active>();
        UUtils_Signal_BallisticMotion_OnStopped::Broadcast(InHandle, MakePayload(InHandle, InFinalLocation));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_BallisticMotion_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FFragment_BallisticMotion_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_BallisticMotion_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests.Get_Requests(), Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_BallisticMotion_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FCk_Request_BallisticMotion_Launch& InRequest)
        -> void
    {
        const auto StartTime = [&]() -> FCk_Time
        {
            switch (const auto LaunchTimePolicy = InRequest.Get_LaunchTimePolicy())
            {
                case ECk_BallisticMotion_LaunchTime::CurrentWorldTime:
                {
                    return ballistic_motion_detail::Get_CurrentWorldTime(InHandle);
                }
                case ECk_BallisticMotion_LaunchTime::OverrideTime:
                {
                    return InRequest.Get_OverrideStartTime();
                }
                default:
                {
                    CK_INVALID_ENUM(LaunchTimePolicy);
                    return ballistic_motion_detail::Get_CurrentWorldTime(InHandle);
                }
            }
        }();

        const auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        InCurrent._InitialConditions = FCk_Ballistic_InitialConditions{StartTime, StartLocation, InRequest.Get_StartVelocity()};
        InCurrent._CurrentVelocity = InRequest.Get_StartVelocity();
        InCurrent._TrajectorySegmentIndex = 0;
        InCurrent._HandledImpactEntities.Reset();

        InHandle.AddOrGet<FTag_BallisticMotion_Active>();

        UUtils_Signal_BallisticMotion_OnTrajectoryChanged::Broadcast(InHandle,
            MakePayload(InHandle, InCurrent.Get_InitialConditions(), InCurrent.Get_TrajectorySegmentIndex()));
    }

    auto
        FProcessor_BallisticMotion_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FCk_Request_BallisticMotion_Stop& InRequest)
        -> void
    {
        const auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        const auto CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        ballistic_motion_detail::DoBroadcastStopped(InHandle, CurrentLocation);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BallisticMotion_HandleImpacts::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FFragment_Probe_Current& InProbeCurrent)
        -> void
    {
        const auto& CurrentOverlaps = InProbeCurrent.Get_CurrentOverlaps();

        // A handled contact stays muted only while the overlap persists
        for (auto It = InCurrent._HandledImpactEntities.CreateIterator(); It; ++It)
        {
            const auto StillOverlapping = CurrentOverlaps.Contains(FCk_Probe_OverlapInfo{*It});

            if (NOT StillOverlapping)
            {
                It.RemoveCurrent();
            }
        }

        for (const auto& Overlap : CurrentOverlaps)
        {
            if (InCurrent._HandledImpactEntities.Contains(Overlap.Get_OtherEntity()))
            { continue; }

            DoHandleImpact(InHandle, InParams, InCurrent, Overlap);

            // Each impact re-anchors the trajectory — remaining overlaps refer to the old
            // segment and are handled on subsequent frames if their contacts persist
            break;
        }
    }

    auto
        FProcessor_BallisticMotion_HandleImpacts::
        DoHandleImpact(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FCk_Probe_OverlapInfo& InOverlap)
        -> void
    {
        const auto& InitialConditions = InCurrent.Get_InitialConditions();
        const auto& TrajectoryParams = InParams.Get_TrajectoryParams();

        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);

        const auto ImpactLocation = InOverlap.Get_ContactPoints().IsEmpty()
            ? UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle)
            : InOverlap.Get_ContactPoints()[0];

        const auto TimeOfFlightToImpact = FCk_Time{FMath::Max(
            ballistics::Get_TimeOfFlightTo(InitialConditions, TrajectoryParams, ImpactLocation, InCurrent.Get_CurrentVelocity()).Get_Seconds(),
            0.0)};

        const auto ImpactWorldTime = InitialConditions.Get_StartTime() + TimeOfFlightToImpact;
        const auto ImpactVelocity = ballistics::Get_VelocityAtTime(InitialConditions, TrajectoryParams, TimeOfFlightToImpact);

        // Normal must oppose the incoming velocity for the reflection to make sense
        const auto ImpactNormal = [&]() -> FVector
        {
            const auto Normal = InOverlap.Get_ContactNormal();

            if (Normal.SizeSquared() < UE_DOUBLE_SMALL_NUMBER)
            { return FVector::ZeroVector; }

            return FVector::DotProduct(Normal, ImpactVelocity) > 0.0 ? -Normal : Normal;
        }();

        InCurrent._HandledImpactEntities.Add(InOverlap.Get_OtherEntity());

        const auto ReflectedVelocity = [&]() -> FVector
        {
            if (ImpactNormal.IsNearlyZero())
            { return FVector::ZeroVector; }

            const auto VelocityAlongNormal = FVector::DotProduct(ImpactVelocity, ImpactNormal);
            return ImpactVelocity - (1.0 + InParams.Get_Restitution()) * VelocityAlongNormal * ImpactNormal;
        }();

        const auto ShouldBounce =
            InParams.Get_ImpactResponse() == ECk_BallisticMotion_ImpactResponse::Bounce &&
            ReflectedVelocity.Size() >= InParams.Get_MinBounceSpeed();

        const auto Outcome = ShouldBounce
            ? ECk_BallisticMotion_ImpactOutcome::Bounced
            : ECk_BallisticMotion_ImpactOutcome::Stopped;

        UUtils_Signal_BallisticMotion_OnImpact::Broadcast(InHandle,
            MakePayload(InHandle, FCk_BallisticMotion_Payload_OnImpact{
                InOverlap.Get_OtherEntity(), ImpactLocation, ImpactNormal, ImpactVelocity, ImpactWorldTime, Outcome}));

        if (NOT ShouldBounce)
        {
            UCk_Utils_Transform_UE::Request_SetLocation(TransformHandle,
                FCk_Request_Transform_SetLocation{ImpactLocation});

            ballistic_motion_detail::DoBroadcastStopped(InHandle, ImpactLocation);
            return;
        }

        const auto BounceLocation = ImpactLocation + ImpactNormal * InParams.Get_ImpactNudgeDistance();

        InCurrent._InitialConditions = FCk_Ballistic_InitialConditions{ImpactWorldTime, BounceLocation, ReflectedVelocity};
        InCurrent._CurrentVelocity = ReflectedVelocity;
        ++InCurrent._TrajectorySegmentIndex;

        UUtils_Signal_BallisticMotion_OnTrajectoryChanged::Broadcast(InHandle,
            MakePayload(InHandle, InCurrent.Get_InitialConditions(), InCurrent.Get_TrajectorySegmentIndex()));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BallisticMotion_UpdateTrajectory::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent)
        -> void
    {
        const auto& InitialConditions = InCurrent.Get_InitialConditions();
        const auto& TrajectoryParams = InParams.Get_TrajectoryParams();

        const auto Now = ballistic_motion_detail::Get_CurrentWorldTime(InHandle);
        const auto TimeSinceStart = Now - InitialConditions.Get_StartTime();

        const auto NewLocation = ballistics::Get_PositionAtTime(InitialConditions, TrajectoryParams, TimeSinceStart);
        const auto NewVelocity = ballistics::Get_VelocityAtTime(InitialConditions, TrajectoryParams, TimeSinceStart);

        InCurrent._CurrentVelocity = NewVelocity;

        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);

        UCk_Utils_Transform_UE::Request_SetLocationAndRotation(TransformHandle,
            FCk_Request_Transform_SetLocationAndRotation{NewLocation, NewVelocity.Rotation()});
    }
}

// --------------------------------------------------------------------------------------------------------------------
