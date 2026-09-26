#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Processor.h"

#include "CkProceduralAnimation/Core/CkProceduralBodySupport.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <Kismet/KismetMathLibrary.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralBodyPose_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_body_pose_processor
{
    // The rotation that, applied to a body-local rotation, cancels the tilt (swing) the body took since InLastBodyRotation and
    // keeps its turn (twist) about the body's up: with the body-local delta Swing * Twist, the drawn body then keeps its
    // world tilt while its yaw follows the body.
    auto
        Get_SwingTransport(
            const FQuat& InLastBodyRotation,
            const FQuat& InBodyRotation)
        -> FQuat
    {
        const auto LocalDelta = (InLastBodyRotation.Inverse() * InBodyRotation).GetNormalized();
        auto Swing = FQuat::Identity;
        auto Twist = FQuat::Identity;
        LocalDelta.ToSwingTwist(FVector::UpVector, Swing, Twist);
        return (Twist.Inverse() * Swing.Inverse() * Twist).GetNormalized();
    }

    // The knee below the max attitude lag over which the carried share of the transport ramps from all to none.
    constexpr auto AttitudeLagKneeDegrees = 8.0f;

    // The share of this frame's transport the offset takes, given how far it already trails its target without it: all of
    // it until the knee, none at InMaxLagDegrees, linear between, so the drawn body's rate ramps up to the body's without a
    // step. With no knee (a max lag of 0) nothing is carried.
    auto
        Get_TransportShare(
            float InLagDegrees,
            float InMaxLagDegrees)
        -> float
    {
        const auto KneeDegrees = FMath::Min(AttitudeLagKneeDegrees, InMaxLagDegrees);
        if (KneeDegrees <= 0.0f)
        { return 0.0f; }

        return FMath::Clamp((InMaxLagDegrees - InLagDegrees) / KneeDegrees, 0.0f, 1.0f);
    }

    // Planted feet count fully; a swinging foot fades out over the first third of its swing and back in over the last third,
    // so the fitted plane does not step when the planted set changes.
    auto
        Get_ConformWeight(
            const FCk_ProceduralLeg_Foot& InFoot)
        -> float
    {
        if (InFoot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted)
        { return 1.0f; }

        const auto SwingAlpha = FMath::Clamp(InFoot.Get_SwingAlpha(), 0.0f, 1.0f);
        return FMath::Max(0.0f, 1.0f - 3.0f * SwingAlpha) + FMath::Max(0.0f, 3.0f * SwingAlpha - 2.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralBodyPose_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralBodyPose_Params& InParams,
            const FFragment_ProceduralBodyPose_SupportLayout& InSupportLayout,
            FFragment_ProceduralBodyPose& InPoseComp,
            const FFragment_ProceduralGait& InGaitComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto DeltaSeconds = static_cast<float>(InDeltaT.Get_Seconds());
        if (NOT FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
        { return; }

        auto Presentation = InParams.Get_Presentation();
        const auto PresentationLive = ck::IsValid(Presentation)
            && NOT Presentation.Has<FTag_DestroyEntity_Initiate>()
            && UCk_Utils_Transform_UE::Has(Presentation);
        CK_ENSURE_IF_NOT(PresentationLive,
            TEXT("Procedural body pose [{}] lost its presentation entity; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MissingPresentation);
            return;
        }

        const auto& HipLocals = InSupportLayout.Get_HipLocals();
        const auto LayoutValid = HipLocals.Num() == InGaitComp._Legs.Num();
        CK_ENSURE_IF_NOT(LayoutValid,
            TEXT("Procedural body pose [{}] captured [{}] hips for [{}] gait legs; feature is failed."),
            InHandle, HipLocals.Num(), InGaitComp._Legs.Num())
        {
            InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MalformedSupport);
            return;
        }

        constexpr auto Unsupported = 0.0f;
        constexpr auto Supporting = 1.0f;

        auto Legs = TArray<FProceduralBodySupportLeg, TInlineAllocator<8>>{};
        Legs.Reserve(HipLocals.Num());
        for (auto Index = 0; Index < HipLocals.Num(); ++Index)
        {
            const auto& Leg = InGaitComp._Legs[Index];
            const auto Supports = ck::IsValid(Leg)
                && NOT Leg.Has<FTag_DestroyEntity_Initiate>()
                && NOT Leg.Has<FTag_ProceduralLeg_Disabled>();
            Legs.Emplace(HipLocals[Index], Supports ? Supporting : Unsupported);
        }

        const auto& Support = InParams.Get_Support();
        const auto Settings = FProceduralBodySupportSettings{}
            .Set_CollapseDrop(Support.Get_CollapseDrop())
            .Set_MaxTiltDegrees(Support.Get_MaxTilt());

        const auto SupportTarget = ComputeProceduralBodySupportPose(Legs, Settings);
        const auto SupportTargetValid = SupportTarget.IsSet();
        CK_ENSURE_IF_NOT(SupportTargetValid,
            TEXT("Procedural body pose [{}] support input was malformed; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MalformedSupport);
            return;
        }

        const auto& Body = InTransform.Get_Transform();
        const auto Transport = ck_procedural_body_pose_processor::Get_SwingTransport(InPoseComp._LastBodyRotation, Body.GetRotation());
        InPoseComp._LastBodyRotation = Body.GetRotation();

        auto ConformTarget = FTransform::Identity;
        if (InHandle.Has<FFragment_ProceduralBodyPose_ConformState>())
        {
            const auto& ConformParams = InHandle.Get<FFragment_ProceduralBodyPose_Conform>();
            auto& ConformState = InHandle.Get<FFragment_ProceduralBodyPose_ConformState>();

            const auto BodyInverse = Body.Inverse();
            // Airborne feet are tucked under the body, not standing on anything, so they fit no plane.
            const auto Airborne = InGaitComp._Solver.IsAirborne();
            auto Feet = TArray<FProceduralBodyConformFoot, TInlineAllocator<16>>{};
            Feet.Reserve(InGaitComp._Legs.Num());
            for (const auto& Leg : InGaitComp._Legs)
            {
                const auto Stands = NOT Airborne
                    && ck::IsValid(Leg)
                    && NOT Leg.Has<FTag_DestroyEntity_Initiate>()
                    && NOT Leg.Has<FTag_ProceduralLeg_Disabled>();
                if (NOT Stands)
                {
                    Feet.Emplace(FVector::ZeroVector, FVector::ZeroVector, 0.0f);
                    continue;
                }

                const auto& Foot = Leg.Get<FFragment_ProceduralLeg>().Get_Foot();
                Feet.Emplace(BodyInverse.TransformPosition(Foot.Get_Position()),
                    Leg.Get<FFragment_ProceduralLeg_Params>().Get_Placement().Get_RestFootLocal(),
                    ck_procedural_body_pose_processor::Get_ConformWeight(Foot));
            }

            const auto ConformSettings = FProceduralBodyConformSettings{}
                .Set_MaxTiltDegrees(ConformParams.Get_MaxTilt())
                .Set_HeightWeight(ConformParams.Get_HeightWeight())
                .Set_MaxHeight(ConformParams.Get_MaxHeight());
            auto Fitted = FTransform::Identity;
            const auto Result = ComputeProceduralBodyConformPose(Feet, ConformSettings, Fitted);
            const auto ConformValid = Result != EProceduralBodyConformResult::Malformed;
            CK_ENSURE_IF_NOT(ConformValid,
                TEXT("Procedural body pose [{}] conform input was malformed; feature is failed."), InHandle)
            {
                InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MalformedConform);
                return;
            }

            if (Result == EProceduralBodyConformResult::Fitted)
            { ConformState._HeldTarget = Fitted; }

            const auto SlewSettings = FProceduralBodyConformSlewSettings{}
                .Set_MaxTiltRateDegrees(ConformParams.Get_MaxTiltRate())
                .Set_MaxHeightRate(ConformParams.Get_MaxHeightRate());
            const auto Applied = SlewProceduralBodyConformPose(ConformState._AppliedTarget, ConformState._HeldTarget, SlewSettings, InDeltaT);
            const auto AppliedValid = Applied.IsSet();
            CK_ENSURE_IF_NOT(AppliedValid,
                TEXT("Procedural body pose [{}] could not slew its conform target; feature is failed."), InHandle)
            {
                InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MalformedConform);
                return;
            }

            ConformState._AppliedTarget = *Applied;
            ConformTarget = ConformState._AppliedTarget;
        }

        const auto TargetRotation = ConformTarget.GetRotation() * SupportTarget->GetRotation();
        const auto TargetLocation = SupportTarget->GetLocation() + ConformTarget.GetLocation();
        InPoseComp._TargetOffset = FTransform{TargetRotation, TargetLocation};

        // The simulation body steps its tilt at every facet it crosses; carried into the offset, that step leaves the drawn
        // body's world tilt where it was, and the spring then eases it to the target. Only the carried share is limited: a
        // sustained rotation (a wall corner) would otherwise leave the drawn body trailing by twice the rate over the
        // spring's natural frequency, and the offset still reaches a jumping target only through the spring.
        // The rotation spring's angular velocity lives in the offset's own frame (q' = q w / 2), so a rotation applied on
        // the body side of the offset leaves it unchanged: it already turns with the offset.
        const auto& Spring = InParams.Get_Spring();
        const auto LagDegrees = FMath::RadiansToDegrees(InPoseComp._Offset.GetRotation().AngularDistance(TargetRotation));
        const auto Carried = FQuat::Slerp(FQuat::Identity, Transport,
            ck_procedural_body_pose_processor::Get_TransportShare(LagDegrees, Spring.Get_MaxAttitudeLag()));
        InPoseComp._Offset.SetRotation((Carried * InPoseComp._Offset.GetRotation()).GetNormalized());

        // The target is a pose that jumps when the supporting set changes, not a moving point: the engine would read
        // that jump as a one-frame target velocity and kick the spring past it (15% overshoot at 60 fps).
        constexpr auto TargetVelocityAmount = 0.0f;
        const auto Location = UKismetMathLibrary::VectorSpringInterp(InPoseComp._Offset.GetLocation(), TargetLocation,
            InPoseComp._TranslationSpring, Spring.Get_Stiffness(), Spring.Get_CriticalDampingFactor(), DeltaSeconds, Spring.Get_Mass(),
            TargetVelocityAmount);
        const auto Rotation = UKismetMathLibrary::QuaternionSpringInterp(InPoseComp._Offset.GetRotation(), TargetRotation,
            InPoseComp._RotationSpring, Spring.Get_Stiffness(), Spring.Get_CriticalDampingFactor(), DeltaSeconds, Spring.Get_Mass(),
            TargetVelocityAmount);
        InPoseComp._Offset = FTransform{Rotation.GetNormalized(), Location};

        const auto Posed = InPoseComp._Offset * Body;
        UCk_Utils_Transform_UE::Request_SetTransform(Presentation, FCk_Request_Transform_SetTransform{Posed}, {});
    }
}

// --------------------------------------------------------------------------------------------------------------------
