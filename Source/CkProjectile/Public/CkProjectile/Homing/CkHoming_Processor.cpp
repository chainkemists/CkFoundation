#include "CkHoming_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkProjectile/Homing/CkHoming_ProNav.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Homing_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Homing_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::homing_detail
{
    static auto
    RestoreBaseAcceleration(
        FCk_Handle InHandle,
        const FVector& InBaseAcceleration) -> void
    {
        auto AccelerationHandle = UCk_Utils_Acceleration_UE::CastChecked(InHandle);
        UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, InBaseAcceleration);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Homing_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_Homing& InHandle,
            const FFragment_Homing_Params& InParams,
            FFragment_Homing_Current& InCurrent,
            const FFragment_Homing_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_Homing_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_Homing_HandleRequests::
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_SetTargetEntity& InRequest)
        -> void
    {
        const auto& Target = InRequest.Get_TargetEntity();

        CK_ENSURE_IF_NOT(ck::IsValid(Target),
            TEXT("Request_SetTargetEntity on [{}] with an invalid target entity"), InHandle)
        { return; }

        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(Target),
            TEXT("Homing target [{}] has no Transform feature — there is no location to chase"), Target)
        { return; }

        InCurrent._TargetEntity = Target;
        InCurrent._TargetMode = ECk_Homing_TargetMode::Entity;
        InCurrent._LocalOffset = FVector::ZeroVector;

        if (InRequest.Get_TargetPoint() == ECk_Homing_TargetPoint::WorldSpacePointOnTarget)
        {
            const auto TargetTransformHandle = UCk_Utils_Transform_UE::CastChecked(Target);
            const auto TargetTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TargetTransformHandle);
            InCurrent._LocalOffset = TargetTransform.InverseTransformPosition(InRequest.Get_WorldSpacePoint());
        }

        InCurrent._HasPreviousTargetLocation = false;
        InCurrent._PreviouslyClosing = false;
        InCurrent._MissNotified = false;

        if (NOT InHandle.Has<FTag_Homing_Disabled>())
        {
            InHandle.AddOrGet<FTag_Homing_Active>();
        }
    }

    auto
        FProcessor_Homing_HandleRequests::
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_SetTargetLocation& InRequest)
        -> void
    {
        InCurrent._TargetEntity = FCk_Handle{};
        InCurrent._TargetMode = ECk_Homing_TargetMode::WorldPoint;
        InCurrent._WorldPoint = InRequest.Get_TargetLocation();
        InCurrent._LocalOffset = FVector::ZeroVector;

        InCurrent._HasPreviousTargetLocation = false;
        InCurrent._PreviouslyClosing = false;
        InCurrent._MissNotified = false;

        if (NOT InHandle.Has<FTag_Homing_Disabled>())
        {
            InHandle.AddOrGet<FTag_Homing_Active>();
        }
    }

    auto
        FProcessor_Homing_HandleRequests::
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_ClearTarget& InRequest)
        -> void
    {
        if (InCurrent._TargetMode != ECk_Homing_TargetMode::None)
        {
            homing_detail::RestoreBaseAcceleration(InHandle, InCurrent._BaseAcceleration);
        }

        InCurrent._TargetEntity = FCk_Handle{};
        InCurrent._TargetMode = ECk_Homing_TargetMode::None;
        InCurrent._HasPreviousTargetLocation = false;

        InHandle.Try_Remove<FTag_Homing_Active>();
    }

    auto
        FProcessor_Homing_HandleRequests::
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_SetDesiredTimeToImpact& InRequest)
        -> void
    {
        InCurrent._ImpactTiming = InRequest.Get_ImpactTiming();
        InCurrent._DesiredTimeToImpactRemaining = InRequest.Get_DesiredTimeToImpact();
    }

    auto
        FProcessor_Homing_HandleRequests::
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_EnableDisable& InRequest)
        -> void
    {
        switch (const auto EnableDisable = InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                InHandle.Try_Remove<FTag_Homing_Disabled>();

                if (InCurrent._TargetMode != ECk_Homing_TargetMode::None)
                {
                    InHandle.AddOrGet<FTag_Homing_Active>();
                }

                break;
            }
            case ECk_EnableDisable::Disable:
            {
                if (InHandle.Has<FTag_Homing_Active>())
                {
                    homing_detail::RestoreBaseAcceleration(InHandle, InCurrent._BaseAcceleration);
                }

                InHandle.AddOrGet<FTag_Homing_Disabled>();
                InHandle.Try_Remove<FTag_Homing_Active>();

                break;
            }
            default:
            {
                CK_INVALID_ENUM(EnableDisable);
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Homing_Update::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_Homing& InHandle,
            const FFragment_Homing_Params& InParams,
            FFragment_Homing_Current& InCurrent,
            const FFragment_Velocity_Current& InVelocity,
            FFragment_Acceleration_Current& InAcceleration) const
        -> void
    {
        auto TargetLocation = FVector{};
        auto TargetVelocity = FVector::ZeroVector;

        switch (const auto TargetMode = InCurrent.Get_TargetMode())
        {
            case ECk_Homing_TargetMode::Entity:
            {
                const auto& Target = InCurrent.Get_TargetEntity();

                if (ck::Is_NOT_Valid(Target) || NOT UCk_Utils_Transform_UE::Has(Target))
                {
                    // Deactivate BEFORE broadcasting — delegates fire synchronously and must
                    // observe homing already shut down (inactive, base acceleration restored)
                    const auto LostTarget = Target;
                    DoDeactivate(InHandle, InCurrent);

                    UUtils_Signal_Homing_OnTargetLost::Broadcast(InHandle,
                        MakePayload(InHandle, FCk_Homing_Payload_OnTargetLost{LostTarget}));

                    return;
                }

                const auto TargetTransformHandle = UCk_Utils_Transform_UE::CastChecked(Target);
                const auto TargetTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TargetTransformHandle);
                TargetLocation = TargetTransform.TransformPosition(InCurrent._LocalOffset);

                if (UCk_Utils_Velocity_UE::Has(Target))
                {
                    TargetVelocity = UCk_Utils_Velocity_UE::Get_CurrentVelocity(UCk_Utils_Velocity_UE::CastChecked(Target));
                }
                else if (InCurrent._HasPreviousTargetLocation)
                {
                    TargetVelocity = (TargetLocation - InCurrent._PreviousTargetLocation) / InDeltaT.Get_Seconds();
                }

                InCurrent._PreviousTargetLocation = TargetLocation;
                InCurrent._HasPreviousTargetLocation = true;

                break;
            }
            case ECk_Homing_TargetMode::WorldPoint:
            {
                TargetLocation = InCurrent._WorldPoint;
                break;
            }
            case ECk_Homing_TargetMode::None:
            default:
            {
                CK_INVALID_ENUM(TargetMode);
                return;
            }
        }

        const auto PursuerTransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        const auto PursuerLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(PursuerTransformHandle);

        const auto Kinematics = FCk_Homing_Kinematics{
            PursuerLocation, InVelocity.Get_CurrentVelocity(), TargetLocation, TargetVelocity};

        const auto GuidanceState = pronav::Compute_GuidanceState(Kinematics);
        InCurrent._LastGuidanceState = GuidanceState;

        auto Settings = InParams.Get_GuidanceSettings();
        Settings.Set_ImpactTiming(InCurrent._ImpactTiming);
        Settings.Set_DesiredTimeToImpact(InCurrent._DesiredTimeToImpactRemaining);

        if (InCurrent._ImpactTiming == ECk_Homing_ImpactTiming::ArriveAtDesiredTime)
        {
            InCurrent._DesiredTimeToImpactRemaining -= InDeltaT;
        }

        const auto HomingAcceleration = pronav::Compute_HomingAcceleration(
            GuidanceState, Kinematics, Settings, InCurrent._BaseAcceleration, InDeltaT);

        auto AccelerationHandle = UCk_Utils_Acceleration_UE::CastChecked(InHandle);
        UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(
            AccelerationHandle, InCurrent._BaseAcceleration + HomingAcceleration);

        DoCheckForMissedTarget(InHandle, InParams, InCurrent, GuidanceState);
    }

    auto
        FProcessor_Homing_Update::
        DoDeactivate(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent)
        -> void
    {
        homing_detail::RestoreBaseAcceleration(InHandle, InCurrent._BaseAcceleration);

        InCurrent._TargetEntity = FCk_Handle{};
        InCurrent._TargetMode = ECk_Homing_TargetMode::None;
        InCurrent._HasPreviousTargetLocation = false;

        InHandle.Try_Remove<FTag_Homing_Active>();
    }

    auto
        FProcessor_Homing_Update::
        DoCheckForMissedTarget(
            FCk_Handle_Homing InHandle,
            const FFragment_Homing_Params& InParams,
            FFragment_Homing_Current& InCurrent,
            const FCk_Homing_GuidanceState& InState)
        -> void
    {
        const auto Threshold = InParams.Get_MissNotifyDistanceThreshold();
        const auto WithinThreshold = Threshold <= 0.0f || InState.Get_LineOfSightDistance() <= Threshold;

        if (WithinThreshold)
        {
            if (InCurrent._PreviouslyClosing && InState.Get_ClosingSpeed() < 0.0 && NOT InCurrent._MissNotified)
            {
                UUtils_Signal_Homing_OnTargetMissed::Broadcast(InHandle,
                    MakePayload(InHandle, FCk_Homing_Payload_OnTargetMissed{
                        InCurrent.Get_TargetEntity(),
                        static_cast<float>(InState.Get_LineOfSightDistance())}));

                InCurrent._MissNotified = true;
            }
        }
        else
        {
            InCurrent._MissNotified = false;
        }

        InCurrent._PreviouslyClosing = InState.Get_ClosingSpeed() > 0.0;
    }
}

// --------------------------------------------------------------------------------------------------------------------
