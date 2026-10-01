#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Processor.h"

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment.h"
#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Utils.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Query/CkJoltQuery_Utils.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_SurfaceMotion_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_SurfaceMotion_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_SurfaceMotion_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_SurfaceMotion_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_surface_motion
{
    auto
        DoGet_WallPolicy(
            ECk_SurfaceMotion_WallPolicy InWallPolicy)
        -> ck::EProceduralSurfaceWallPolicy
    {
        return InWallPolicy == ECk_SurfaceMotion_WallPolicy::Slide ? ck::EProceduralSurfaceWallPolicy::Slide : ck::EProceduralSurfaceWallPolicy::Climb;
    }

    auto
        DoBuild_Settings(
            const ck::FFragment_SurfaceMotion_Params& InParams)
        -> ck::FProceduralSurfaceMotionSettings
    {
        const auto& Contact = InParams.Get_Contact();
        const auto& Movement = InParams.Get_Movement();
        return ck::FProceduralSurfaceMotionSettings{}
            .Set_Clearance(Contact.Get_Clearance())
            .Set_ProbeReach(Contact.Get_ProbeReach())
            .Set_ContactGrace(Contact.Get_ContactGrace())
            .Set_ConfirmAngleDegrees(Contact.Get_ConfirmAngle())
            .Set_ConfirmTime(Contact.Get_ConfirmTime())
            .Set_SurfaceTurnRateDegrees(Movement.Get_SurfaceTurnRate())
            .Set_ClearanceSpeed(Movement.Get_ClearanceSpeed())
            .Set_Gravity(Movement.Get_Gravity())
            .Set_SteerFloor(Movement.Get_SteerFloor())
            .Set_MaxStepHeight(Contact.Get_MaxStepHeight())
            .Set_WallPolicy(DoGet_WallPolicy(Contact.Get_WallPolicy()));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_SurfaceMotion_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SurfaceMotion& InMotionComp,
            FFragment_SurfaceMotion_Support& InSupportComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto Rotation = InTransform.Get_Transform().GetRotation();
        InSupportComp._State.Set_SupportNormal(Rotation.GetAxisZ());
        InSupportComp._State.Set_TravelTangent(Rotation.GetAxisX());
        InMotionComp._Direction = Rotation.GetAxisX();

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SurfaceMotion_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SurfaceMotion_Params& InParams,
            FFragment_SurfaceMotion& InMotionComp,
            FFragment_SurfaceMotion_Requests& InRequestsComp)
        -> void
    {
        auto Requests = MoveTemp(InRequestsComp._Requests);
        InRequestsComp._Requests.Reset();
        for (const auto& Request : Requests)
        {
            auto Result = ECk_Request_OperationResult::Failed_Cancelled;
            const auto Guard = MakeCompletionGuard(Request, InHandle, Result);

            if (ck::Is_NOT_Valid(InHandle) || InHandle.Has<FTag_DestroyEntity_Initiate>())
            { continue; }

            InMotionComp._Direction = Request.Get_WorldDirection().GetSafeNormal();
            InMotionComp._Speed = FMath::Min(Request.Get_Speed(), InParams.Get_Movement().Get_MaxSpeed());
            Result = ECk_Request_OperationResult::Succeeded;
        }

        // A completion callback may enqueue more work; do not remove its new queue.
        if (InRequestsComp._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SurfaceMotion_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SurfaceMotion_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SurfaceMotion_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SurfaceMotion_Params& InParams,
            const FFragment_SurfaceMotion& InMotionComp,
            FFragment_SurfaceMotion_Support& InSupportComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto Dt = InDeltaT.Get_Seconds();
        if (NOT FMath::IsFinite(Dt) || Dt <= 0.0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        auto Body = InTransform.Get_Transform();
        const auto BodyFinite = NOT Body.ContainsNaN();
        CK_ENSURE_IF_NOT(BodyFinite, TEXT("Surface motion [{}] body transform contains NaN; motion is failed."), InHandle)
        {
            InHandle.Add<FFragment_SurfaceMotion_Failure>(ECk_SurfaceMotion_Failure::NanBody);
            return;
        }

        const auto Settings = ck_surface_motion::DoBuild_Settings(InParams);
        const auto& QueryFilter = InParams.Get_Contact().Get_QueryFilter();
        InSupportComp._ReachPaceScale = 1.0f;
        InSupportComp._ReachPaceState = ECk_SurfaceMotion_ReachPaceState::Free;
        InSupportComp._ReachPaceTrials = 0;
        InSupportComp._ReachPaceRays = 0;
        InSupportComp._AttemptedStanceSpeed = 0.0f;
        InSupportComp._ReachPaceFeedback.Reset();
        const auto RayCast = [&](const FVector& InStart, const FVector& InEnd) -> FProceduralSurfaceHit
        {
            ++InSupportComp._ReachPaceRays;
            const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(World, InStart, InEnd, QueryFilter);
            return FProceduralSurfaceHit{}
                .Set_Hit(Hit.Get_HasHit())
                .Set_Position(Hit.Get_Position())
                .Set_Normal(Hit.Get_Normal())
                .Set_Fraction(Hit.Get_Fraction());
        };

        // Bounded integration work, with swept fall contact in the core. A hitch advances the entire supplied duration
        // instead of silently discarding time through a delta clamp.
        constexpr auto IntegrationInterval = FCk_Time{0.016};
        const auto Substeps = FMath::Clamp(FMath::CeilToInt(FMath::Min(Dt / IntegrationInterval.Get_Seconds(), 64.0)), 1, 64);
        const auto Step = FCk_Time{Dt / Substeps};
        // The gait publishes its feet plane after this update runs, so the body rides the plane of the previous frame.
        auto FeetSupport = TOptional<FProceduralSurfaceFeetSupport>{};
        // A gait that has latched a failure no longer writes its plane, so the last one it wrote must not hold the body.
        const auto RidesPlantedFeet = InParams.Get_Contact().Get_HeightSource() == ECk_SurfaceMotion_HeightSource::PlantedFeet
            && InHandle.Has<FFragment_ProceduralGait_FeetPlane>()
            && UCk_Utils_ProceduralGait_UE::Get_Status(UCk_Utils_ProceduralGait_UE::Cast(InHandle)) == ECk_ProceduralAnimation_Status::Ready;
        if (RidesPlantedFeet)
        {
            const auto& FeetPlane = InHandle.Get<FFragment_ProceduralGait_FeetPlane>();
            if (FeetPlane.Get_State() != EProceduralGaitFeetPlane::None)
            { FeetSupport = FeetPlane.Get_Support(); }
        }

        // The gait writes this after the previous physics pass. Changes since that solve invalidate the snapshot rather
        // than making SurfaceMotion pace against a stale body transform or leg plant.
        auto ReachAnchors = TArray<FProceduralGaitReachAnchor, TInlineAllocator<64>>{};
        auto PlantedContacts = TArray<FProceduralSurfacePlantedContact, TInlineAllocator<64>>{};
        const auto HasGait = InHandle.Has<FFragment_ProceduralGait>();
        if (HasGait
            && UCk_Utils_ProceduralGait_UE::Get_Status(UCk_Utils_ProceduralGait_UE::Cast(InHandle)) == ECk_ProceduralAnimation_Status::Ready)
        {
            const auto& Gait = InHandle.Get<FFragment_ProceduralGait>();
            const auto& Stance = Gait._ReachStance;
            if (Stance.Get_HasSample() && Stance.Get_SolveSequence() == Gait._SolveSequence
                && Stance.Get_BodyAtSolve().Equals(Body, 1.0e-3))
            {
                for (const auto& Anchor : Stance.Get_Anchors())
                {
                    const auto& Leg = Anchor.Get_Leg();
                    if (ck::Is_NOT_Valid(Leg) || Leg.Has<FTag_DestroyEntity_Initiate>() || Leg.Has<FTag_ProceduralLeg_Disabled>()
                        || UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Leg) != InHandle.ConvertToHandle()
                        || NOT Leg.Has<FFragment_ProceduralLeg>())
                    { continue; }

                    const auto& Foot = Leg.Get<FFragment_ProceduralLeg>().Get_Foot();
                    if (Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted
                        && Foot.Get_Contact() == ECk_ProceduralLeg_FootContact::Trusted
                        && Foot.Get_Position().Equals(Anchor.Get_FootWorld(), 1.0e-3))
                    {
                        ReachAnchors.Add(Anchor);
                        PlantedContacts.Emplace(Foot.Get_Position(), Foot.Get_Normal());
                    }
                }
            }
        }

        const auto HasPose = InHandle.Has<FFragment_ProceduralBodyPose>()
            && UCk_Utils_ProceduralBodyPose_UE::Get_Status(UCk_Utils_ProceduralBodyPose_UE::Cast(InHandle))
                == ECk_ProceduralAnimation_Status::Ready;
        const auto PoseOffset = HasPose
            ? UCk_Utils_ProceduralBodyPose_UE::Get_Offset(UCk_Utils_ProceduralBodyPose_UE::CastChecked(InHandle))
            : FTransform::Identity;
        auto CoreAnchors = TArray<FProceduralSurfaceReachPaceAnchor, TInlineAllocator<64>>{};
        CoreAnchors.Reserve(ReachAnchors.Num());
        for (const auto& Anchor : ReachAnchors)
        {
            CoreAnchors.Add(FProceduralSurfaceReachPaceAnchor{
                Anchor.Get_FootWorld(), Anchor.Get_HipLocal(), Anchor.Get_Reach()});
        }
        const auto CorePoseOffset = HasPose ? TOptional<FTransform>{PoseOffset} : TOptional<FTransform>{};
        const auto PlantedContactView = TArrayView<const FProceduralSurfacePlantedContact>{PlantedContacts};
        const auto CorePlantedContacts = HasGait
            ? TOptional<TArrayView<const FProceduralSurfacePlantedContact>>{PlantedContactView}
            : TOptional<TArrayView<const FProceduralSurfacePlantedContact>>{};
        auto AttemptedStanceDistance = 0.0;
        for (auto Iteration = 0; Iteration < Substeps; ++Iteration)
        {
            const auto Outcome = StepProceduralSurfaceMotionPaced(Settings, InMotionComp._Direction, InMotionComp._Speed,
                Step, RayCast, FeetSupport, TArrayView<const FProceduralSurfaceReachPaceAnchor>{CoreAnchors},
                CorePoseOffset, Body, InSupportComp._State, CorePlantedContacts);
            // Earlier rejected trials are not unioned with the final accepted-body stamp. Only the final substep can
            // request release, and any physical override in this frame withdraws that request below.
            if (Iteration == Substeps - 1 && Outcome.Get_RejectedFullBody().IsSet())
            {
                const auto& TrialBody = Outcome.Get_RejectedFullBody().GetValue();
                const auto TrialPosedBody = HasPose ? PoseOffset * TrialBody : FTransform::Identity;
                for (const auto AnchorIndex : Outcome.Get_RejectedAnchorIndices())
                {
                    const auto& Anchor = ReachAnchors[AnchorIndex];
                    InSupportComp._ReachPaceFeedback.Emplace(Anchor.Get_Leg(), Anchor.Get_FootWorld(),
                        TrialBody.TransformPosition(Anchor.Get_HipLocal()), HasPose
                            ? TOptional<FVector>{TrialPosedBody.TransformPosition(Anchor.Get_HipLocal())} : TOptional<FVector>{});
                }
            }
            InSupportComp._ReachPaceTrials += Outcome.Get_Trials();
            AttemptedStanceDistance += Outcome.Get_AttemptedStanceSpeed() * Step.Get_Seconds();
            InSupportComp._ReachPaceScale = FMath::Min(InSupportComp._ReachPaceScale, Outcome.Get_Scale());
            if (Outcome.Get_PhysicalOverride())
            { InSupportComp._ReachPaceState = ECk_SurfaceMotion_ReachPaceState::PhysicalOverride; }
            else if (InSupportComp._ReachPaceState != ECk_SurfaceMotion_ReachPaceState::PhysicalOverride)
            {
                if (Outcome.Get_Scale() == 0.0f)
                { InSupportComp._ReachPaceState = ECk_SurfaceMotion_ReachPaceState::Blocked; }
                else if (Outcome.Get_Scale() < 1.0f && InSupportComp._ReachPaceState != ECk_SurfaceMotion_ReachPaceState::Blocked)
                { InSupportComp._ReachPaceState = ECk_SurfaceMotion_ReachPaceState::Pacing; }
            }
        }
        if (InSupportComp._ReachPaceState == ECk_SurfaceMotion_ReachPaceState::PhysicalOverride)
        { InSupportComp._ReachPaceFeedback.Reset(); }
        InSupportComp._AttemptedStanceSpeed = static_cast<float>(AttemptedStanceDistance / Dt);
        InSupportComp._EvaluatedBody = Body;
        InSupportComp._EvaluatedFrame = GFrameCounter;
        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        UCk_Utils_Transform_UE::Request_SetTransform(TransformHandle, FCk_Request_Transform_SetTransform{Body}, {});
    }
}
