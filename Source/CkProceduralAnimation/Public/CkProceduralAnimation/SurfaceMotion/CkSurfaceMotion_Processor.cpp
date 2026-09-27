#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Processor.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"

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
        const auto RayCast = [&](const FVector& InStart, const FVector& InEnd) -> FProceduralSurfaceHit
        {
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
        const auto RidesPlantedFeet = InParams.Get_Contact().Get_HeightSource() == ECk_SurfaceMotion_HeightSource::PlantedFeet
            && InHandle.Has<FFragment_ProceduralGait_FeetPlane>();
        if (RidesPlantedFeet)
        {
            const auto& FeetPlane = InHandle.Get<FFragment_ProceduralGait_FeetPlane>();
            if (FeetPlane.Get_State() != EProceduralGaitFeetPlane::None)
            { FeetSupport = FeetPlane.Get_Support(); }
        }

        for (auto Iteration = 0; Iteration < Substeps; ++Iteration)
        {
            StepProceduralSurfaceMotion(Settings, InMotionComp._Direction, InMotionComp._Speed, Step, RayCast, FeetSupport, Body,
                InSupportComp._State);
        }
        InSupportComp._EvaluatedFrame = GFrameCounter;
        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        UCk_Utils_Transform_UE::Request_SetTransform(TransformHandle, FCk_Request_Transform_SetTransform{Body}, {});
    }
}
