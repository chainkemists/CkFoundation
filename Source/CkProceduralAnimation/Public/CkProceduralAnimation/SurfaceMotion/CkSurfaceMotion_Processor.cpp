#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Processor.h"

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
        Get_TrustedHit(
            const FCk_Jolt_HitResult& InHit,
            const FVector& InRayDirection)
        -> bool
    {
        return InHit.Get_HasHit() && InHit.Get_Fraction() > 0.0f && InHit.Get_Fraction() <= 1.0f
            && NOT InHit.Get_Position().ContainsNaN()
            && NOT InHit.Get_Normal().ContainsNaN() && NOT InHit.Get_Normal().IsNearlyZero()
            && FVector::DotProduct(InHit.Get_Normal(), InRayDirection) < -KINDA_SMALL_NUMBER;
    }

    auto
        Get_Contact(
            UWorld* InWorld,
            const FVector& InPosition,
            const FVector& InUp,
            const FVector& InForward,
            const FCk_SurfaceMotion_Contact& InContact,
            bool InMoving)
        -> FCk_Jolt_HitResult
    {
        // Forward contact is used only within the body's clearance, so a distant wall cannot
        // pull a creature off its floor. All candidates are current world queries; held feet
        // never masquerade as newly observed support normals.
        if (InMoving)
        {
            const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, InPosition,
                InPosition + InForward * InContact.Get_Clearance(), InContact.Get_QueryFilter());

            if (Get_TrustedHit(Hit, InForward))
            { return Hit; }
        }

        auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, InPosition + InUp * InContact.Get_Clearance(),
            InPosition - InUp * InContact.Get_ProbeReach(), InContact.Get_QueryFilter());

        if (Get_TrustedHit(Hit, -InUp))
        { return Hit; }

        // Recover around a convex edge from a bounded fan rather than extending a ray forever.
        if (InMoving)
        {
            const auto Direction = (-InUp - InForward).GetSafeNormal();
            Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld,
                InPosition + InForward * InContact.Get_Clearance(),
                InPosition + InForward * InContact.Get_Clearance() + Direction * InContact.Get_ProbeReach(),
                InContact.Get_QueryFilter());

            if (Get_TrustedHit(Hit, Direction))
            { return Hit; }
        }

        return {};
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
        InSupportComp._SupportNormal = Rotation.GetAxisZ();
        InSupportComp._TravelTangent = Rotation.GetAxisX();
        InMotionComp._Direction = InSupportComp._TravelTangent;

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

        const auto& Contact = InParams.Get_Contact();
        const auto& Movement = InParams.Get_Movement();
        const auto StartPosition = Body.GetLocation();
        // Bounded integration work, with swept fall contact below. A hitch advances the entire
        // supplied duration instead of silently discarding time through a delta clamp.
        constexpr auto IntegrationInterval = FCk_Time{0.016};
        const auto Substeps = FMath::Clamp(FMath::CeilToInt(FMath::Min(Dt / IntegrationInterval.Get_Seconds(), 64.0)), 1, 64);
        const auto Step = FCk_Time{Dt / Substeps};
        for (auto Iteration = 0; Iteration < Substeps; ++Iteration)
        {
            const auto OldRotation = Body.GetRotation();
            // Attachment owns the accepted surface frame. It must not use the visual body's
            // partially eased up axis: doing so alternates floor/wall hits during a corner turn.
            const auto Up = InSupportComp._SupportNormal;
            auto Forward = FVector::VectorPlaneProject(InMotionComp._Direction, Up).GetSafeNormal();
            if (Forward.IsNearlyZero())
            { Forward = InSupportComp._TravelTangent; }
            const auto Candidate = Body.GetLocation() + Forward * (InMotionComp._Speed * Step.Get_Seconds());
            const auto Moving = InMotionComp._Speed > 0.0f;
            const auto Hit = ck_surface_motion::Get_Contact(World, Candidate, Up, Forward, Contact, Moving);
            InSupportComp._ContactQuery = Hit.Get_HasHit() ? ECk_SurfaceMotion_ContactQuery::Trusted : ECk_SurfaceMotion_ContactQuery::Missed;
            if (Hit.Get_HasHit())
            {
                const auto Normal = Hit.Get_Normal().GetSafeNormal();
                const auto Transport = FQuat::FindBetweenNormals(Up, Normal);
                const auto TargetForward = Transport.RotateVector(Forward);
                InSupportComp._SupportNormal = Normal;
                InSupportComp._TravelTangent = TargetForward;
                const auto TargetRotation = FRotationMatrix::MakeFromZX(Normal, TargetForward).ToQuat();
                const auto Angle = OldRotation.AngularDistance(TargetRotation);
                const auto Alpha = Angle > KINDA_SMALL_NUMBER
                    ? FMath::Min(1.0, FMath::DegreesToRadians(Movement.Get_SurfaceTurnRate()) * Step.Get_Seconds() / Angle) : 1.0;
                Body.SetRotation(FQuat::Slerp(OldRotation, TargetRotation, Alpha).GetNormalized());
                const auto Height = FVector::DotProduct(Candidate - Hit.Get_Position(), Normal);
                const auto Correction = FMath::Clamp(Contact.Get_Clearance() - Height,
                    -Movement.Get_ClearanceSpeed() * Step.Get_Seconds(), Movement.Get_ClearanceSpeed() * Step.Get_Seconds());
                Body.SetLocation(Candidate + Normal * Correction);
                InSupportComp._MissingContact = FCk_Time{};
                InSupportComp._Support = ECk_SurfaceMotion_Support::Grounded;
                InSupportComp._Velocity = (Body.GetLocation() - StartPosition) / (Step.Get_Seconds() * (Iteration + 1));
                continue;
            }
            InSupportComp._MissingContact += Step;
            if (InSupportComp._Support == ECk_SurfaceMotion_Support::Grounded && InSupportComp._MissingContact <= Contact.Get_ContactGrace())
            {
                Body.SetLocation(Candidate);
                InSupportComp._Velocity = Forward * InMotionComp._Speed;
                continue;
            }
            InSupportComp._Support = ECk_SurfaceMotion_Support::Airborne;
            InSupportComp._Velocity += Movement.Get_Gravity() * Step.Get_Seconds();
            const auto FallStart = Body.GetLocation();
            const auto FallEnd = FallStart + InSupportComp._Velocity * Step.Get_Seconds();
            const auto FallDirection = (FallEnd - FallStart).GetSafeNormal();
            const auto FallHit = UCk_Utils_JoltQuery_UE::Get_RayCast(World, FallStart,
                FallEnd + FallDirection * Contact.Get_Clearance(), Contact.Get_QueryFilter());
            if (ck_surface_motion::Get_TrustedHit(FallHit, FallDirection))
            {
                InSupportComp._ContactQuery = ECk_SurfaceMotion_ContactQuery::Trusted;
                const auto Normal = FallHit.Get_Normal().GetSafeNormal();
                Body.SetLocation(FallHit.Get_Position() + Normal * Contact.Get_Clearance());
                Body.SetRotation(FRotationMatrix::MakeFromZX(Normal, Forward).ToQuat());
                InSupportComp._Velocity = FVector::ZeroVector;
                InSupportComp._Support = ECk_SurfaceMotion_Support::Grounded;
                InSupportComp._SupportNormal = Normal;
                InSupportComp._TravelTangent = Body.GetRotation().GetAxisX();
                InSupportComp._MissingContact = FCk_Time{};
            }
            else
            {
                Body.SetLocation(FallEnd);
            }
        }
        InSupportComp._EvaluatedFrame = GFrameCounter;
        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        UCk_Utils_Transform_UE::Request_SetTransform(TransformHandle, FCk_Request_Transform_SetTransform{Body}, {});
    }
}
