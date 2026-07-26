#include "CkCamera_POV.h"

#include <CollisionQueryParams.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::camera
{
    namespace ck_camera_pov
    {
        auto SanitizeRotation(FRotator& InRotation) -> void
        {
            if (InRotation.ContainsNaN())
            {
                if (FMath::IsNaN(InRotation.Yaw))   { InRotation.Yaw = 0.0f; }
                if (FMath::IsNaN(InRotation.Pitch)) { InRotation.Pitch = 0.0f; }
            }
            InRotation.Roll = 0.0f;
        }

        // Wrap-safe: clamping the SHORTEST signed distance from the window centre keeps a cone straddling +/-180
        // correct, where a naive Clamp on an unwound angle snaps to the wrong edge. [-180, 180] = unrestricted.
        auto ClampYawToWindow(float InDesiredYaw, float InMin, float InMax) -> float
        {
            const auto Center = (InMin + InMax) * 0.5f;
            const auto Half   = (InMax - InMin) * 0.5f;
            const auto Delta  = FMath::UnwindDegrees(InDesiredYaw - Center);
            return FMath::UnwindDegrees(Center + FMath::Clamp(Delta, -Half, Half));
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Run(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        if (NOT InOutState._Initialized)
        {
            // Seed from the anchor so the first frame doesn't snap from the origin.
            InOutState._GroupBaseLocation = InInput._AnchorTransform.GetLocation();
            InOutState._LookAtLocation    = InInput._AnchorTransform.GetLocation();
            InOutState._BoomArmRotation   = InInput._AnchorTransform.Rotator();
            InOutState._BoomArmRotation.Roll = 0.0f;
            InOutState._Initialized = true;
        }

        Step_Attachment  (InProfile, InInput, InOutState);
        Step_LookAt      (InProfile, InInput, InOutState);
        Step_BoomRotation(InProfile, InInput, InOutState);
        Step_BoomEnd     (InProfile, InInput, InOutState);
        Step_Framing     (InProfile, InInput, InOutState);
        Step_CameraXform (InProfile, InInput, InOutState);
        Step_Collision   (InProfile, InInput, InOutState);
        Step_Noise       (InProfile, InInput, InOutState);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_Attachment(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        InOutState._GroupBaseAttachmentTransform = InInput._AnchorTransform;

        const auto TargetLocation = InInput._AnchorTransform.GetLocation();
        const auto InterpSpeed    = InProfile.Get_Springs().Get_GroupBaseLocationInterpSpeed();

        // VInterpTo with speed <= 0 snaps to target — i.e. zero lag by default.
        InOutState._GroupBaseLocation = FMath::VInterpTo(
            InOutState._GroupBaseLocation, TargetLocation, InInput._DeltaSeconds, InterpSpeed);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_LookAt(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        // No look-at target → collapses to the pivot, which is how Apply_AutoReorient detects "no look-at".
        const auto Target = InInput._LookAtLocation.Get(InOutState._GroupBaseLocation);
        const auto Speed  = InProfile.Get_Springs().Get_LookAtLocationInterpSpeed();

        InOutState._LookAtLocation = FMath::VInterpTo(InOutState._LookAtLocation, Target, InInput._DeltaSeconds, Speed);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Apply_AutoReorient(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState,
            FRotator& InOutRotation)
        -> void
    {
        if (NOT InProfile.Get_HasAutoReorient())
        { return; }

        const auto& AutoReorient = InProfile.Get_AutoReorient();
        const auto  Intention    = InInput._OrientationIntention;

        const auto GroupBaseLocation = InOutState._GroupBaseLocation;
        auto       LookAtLocation    = InOutState._LookAtLocation;

        if (LookAtLocation.Equals(GroupBaseLocation))
        { return; }

        const auto& Rig = InProfile.Get_Rig();

        auto FramingRotation = InOutState._BoomArmEndTransform.Rotator();
        FramingRotation.Pitch += Rig.Get_FramingPitch();
        FramingRotation.Yaw   += Rig.Get_FramingYaw();

        const auto TargetOffset = FramingRotation.RotateVector(
            FVector(AutoReorient.Get_LookAtTargetLocationPitchOffset(), AutoReorient.Get_LookAtTargetLocationYawOffset(), 0.0f));
        LookAtLocation += TargetOffset;

        const auto Direction = LookAtLocation - GroupBaseLocation;
        auto       RightDir  = FVector::CrossProduct(Direction, FVector::UpVector);
        auto       UpDir     = FVector::CrossProduct(Direction, RightDir);
        RightDir.Normalize();
        UpDir.Normalize();

        const auto FinalCameraLocation = GroupBaseLocation
            + (RightDir * AutoReorient.Get_LookAtCameraLocationYawOffset())
            + (UpDir    * AutoReorient.Get_LookAtCameraLocationPitchOffset());

        const auto FinalDirection = LookAtLocation - FinalCameraLocation;

        auto Reorient = FinalDirection.ToOrientationRotator();
        Reorient.Roll = 0.0f;
        Reorient.Normalize();

        Reorient.Pitch = FMath::Clamp(Reorient.Pitch,
            AutoReorient.Get_Pitch().Get_Limits().Get_Min(), AutoReorient.Get_Pitch().Get_Limits().Get_Max());
        Reorient.Yaw = FMath::Clamp(Reorient.Yaw,
            AutoReorient.Get_Yaw().Get_Limits().Get_Min(), AutoReorient.Get_Yaw().Get_Limits().Get_Max());

        auto CurrentPitch = FMath::UnwindDegrees(InOutRotation.Pitch);
        auto CurrentYaw   = FMath::UnwindDegrees(InOutRotation.Yaw);

        const auto PitchIsPlayerDriven = NOT FMath::IsNearlyZero(Intention.Y);
        if (NOT PitchIsPlayerDriven)
        {
            FMath::WindRelativeAnglesDegrees(CurrentPitch, Reorient.Pitch);
            InOutRotation.Pitch = FMath::FInterpConstantTo(
                CurrentPitch, Reorient.Pitch, InInput._DeltaSeconds, AutoReorient.Get_Pitch().Get_Speed());
        }

        FMath::WindRelativeAnglesDegrees(CurrentYaw, Reorient.Yaw);
        InOutRotation.Yaw = FMath::FInterpConstantTo(
            CurrentYaw, Reorient.Yaw, InInput._DeltaSeconds, AutoReorient.Get_Yaw().Get_Speed());

        ck_camera_pov::SanitizeRotation(InOutRotation);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Compute_OrientationControl(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            const FPov_State& InState)
        -> FRotator
    {
        if (NOT InProfile.Get_HasOrientationControl())
        { return FRotator::ZeroRotator; }

        const auto& OrientationControl = InProfile.Get_OrientationControl();

        // A per-frame DELTA already scaled by the caller: no delta-time multiply and no X/YIntentionCurve clamp
        // (both are deliberate — CkCamera/CLAUDE.md, Anti-patterns 0).
        const auto Intention = InInput._OrientationIntention;

        if (Intention.IsNearlyZero())
        { return FRotator::ZeroRotator; }

        const auto CurrentPitch = FMath::UnwindDegrees(InState._BoomArmRotation.Pitch);

        auto Out = FRotator::ZeroRotator;
        Out.Pitch = static_cast<float>(Intention.Y) * OrientationControl.Get_Pitch().Get_Speed();
        Out.Yaw   = static_cast<float>(Intention.X) * OrientationControl.Get_Yaw().Get_Speed();
        Out.Roll  = 0.0f;

        const auto MinPitch = OrientationControl.Get_Pitch().Get_NormalSpeedRange().Get_Min();
        const auto MaxPitch = OrientationControl.Get_Pitch().Get_NormalSpeedRange().Get_Max();

        if (CurrentPitch < MinPitch || CurrentPitch > MaxPitch)
        { Out.Pitch *= OrientationControl.Get_Pitch().Get_OutOfRangeMultiplier(); }

        return Out;
    }

    auto
        FPov::
        Step_BoomRotation(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        if (InProfile.Get_Rig().Get_UseFixedBoomRotation())
        {
            auto Fixed = InProfile.Get_Rig().Get_FixedBoomRotation();
            ck_camera_pov::SanitizeRotation(Fixed);
            InOutState._BoomArmRotation = Fixed;
            return;
        }

        auto NewRotation = InOutState._BoomArmRotation;

        Apply_AutoReorient(InProfile, InInput, InOutState, NewRotation);

        auto OrientationDelta = Compute_OrientationControl(InProfile, InInput, InOutState);
        ck_camera_pov::SanitizeRotation(OrientationDelta);

        const auto& OrientationControl = InProfile.Get_OrientationControl();

        const auto CurrentPitch = FMath::UnwindDegrees(NewRotation.Pitch);
        const auto CurrentYaw   = FMath::UnwindDegrees(NewRotation.Yaw);

        NewRotation.Pitch = FMath::Clamp(
            FMath::UnwindDegrees(CurrentPitch - OrientationDelta.Pitch),
            OrientationControl.Get_Pitch().Get_Limits().Get_Min(),
            OrientationControl.Get_Pitch().Get_Limits().Get_Max());

        NewRotation.Yaw = ck_camera_pov::ClampYawToWindow(
            CurrentYaw + OrientationDelta.Yaw,
            OrientationControl.Get_Yaw().Get_Limits().Get_Min(),
            OrientationControl.Get_Yaw().Get_Limits().Get_Max());

        ck_camera_pov::SanitizeRotation(NewRotation);

        InOutState._BoomArmRotation = NewRotation;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_BoomEnd(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        const auto& Rig = InProfile.Get_Rig();

        const auto GroupBaseRotation = InOutState._GroupBaseAttachmentTransform.Rotator();
        const auto LocalOffset       = GroupBaseRotation.RotateVector(Rig.Get_BoomArmPivotOffset());
        const auto AttachmentPoint   = InOutState._GroupBaseLocation + LocalOffset;

        const auto DesiredDistance   = Rig.Get_BoomArmLength();
        const auto RotationDirection = InOutState._BoomArmRotation.Vector();
        const auto DesiredLocation   = AttachmentPoint - (RotationDirection * DesiredDistance);

        InOutState._BoomArmEndTransform = FTransform(InOutState._BoomArmRotation, DesiredLocation);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_Framing(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        const auto& Rig = InProfile.Get_Rig();

        auto FramingTransform = InOutState._BoomArmEndTransform;

        auto Rotation = FramingTransform.Rotator();
        Rotation.Pitch += Rig.Get_FramingPitch();
        Rotation.Yaw   += Rig.Get_FramingYaw();

        FramingTransform.AddToTranslation(Rotation.RotateVector(Rig.Get_FramingOffset()));
        FramingTransform.SetRotation(Rotation.Quaternion());

        InOutState._FramingTransform = FramingTransform;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_CameraXform(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        const auto& FramingTransform = InOutState._FramingTransform;

        // Framing rotation is adopted directly: auto-reorient already drove the boom upstream in Step_BoomRotation.
        InOutState._CameraTransform = FTransform(FramingTransform.Rotator(), FramingTransform.GetLocation());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_Collision(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        if (NOT InProfile.Get_HasCollision())
        { return; }

        auto* World = InInput._World.Get();
        if (NOT IsValid(World))
        { return; }

        auto* TraceIgnore = InInput._TraceIgnoreActor.Get();

        const auto& Rig = InProfile.Get_Rig();
        const auto& Collision = InProfile.Get_Collision();

        const auto GroupBaseRotation = InOutState._GroupBaseAttachmentTransform.Rotator();
        const auto AttachmentPoint   = InOutState._GroupBaseLocation
                                     + GroupBaseRotation.RotateVector(Rig.Get_BoomArmPivotOffset());

        const auto CameraLocation = InOutState._CameraTransform.GetLocation();

        auto      SweepDirection = CameraLocation - AttachmentPoint;
        const auto SweepDistance = SweepDirection.Size();
        SweepDirection.Normalize();

        auto Hit = FHitResult{};
        const auto QueryParams = FCollisionQueryParams{SCENE_QUERY_STAT(CkCamera_POV), false, TraceIgnore};

        constexpr auto TraceChannel = ECollisionChannel::ECC_Camera;
        const auto Blocked = World->LineTraceSingleByChannel(Hit, AttachmentPoint, CameraLocation, TraceChannel, QueryParams);

        const auto CollisionHit = Blocked && Hit.bBlockingHit;
        const auto HitDistance  = CollisionHit
                                  ? (Hit.Distance - Collision.Get_AdditionalOffsetFromHit())
                                  : SweepDistance;

        if (NOT CollisionHit && NOT InOutState._CollisionDistance.IsSet())
        { return; }

        if (HitDistance <= 0.0f)
        { return; }

        const auto LastDistance = InOutState._CollisionDistance.Get(SweepDistance);
        const auto IsClipping   = HitDistance < LastDistance;
        const auto Speed        = IsClipping ? Collision.Get_ClippingSpeed() : Collision.Get_ReturningSpeed();

        const auto NewDistance = FMath::FInterpConstantTo(LastDistance, HitDistance, InInput._DeltaSeconds, Speed);

        if (FMath::IsNearlyEqual(NewDistance, SweepDistance))
        {
            InOutState._CollisionDistance.Reset();
            return;
        }

        InOutState._CollisionDistance = NewDistance;
        InOutState._CameraTransform.SetLocation(AttachmentPoint + (SweepDirection * NewDistance));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPov::
        Step_Noise(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState)
        -> void
    {
        const auto& Noise = InProfile.Get_Noise();

        const auto Step = [&](const FCk_CameraProfile_Rotation& InRot, float InCurrent, bool& InOutPositive) -> float
        {
            const auto Target = InOutPositive ? InRot.Get_Limits().Get_Max() : InRot.Get_Limits().Get_Min();
            const auto Result = FMath::FInterpConstantTo(
                FMath::UnwindDegrees(InCurrent), Target, InInput._DeltaSeconds, InRot.Get_Speed());

            if (Result == Target)
            { InOutPositive = NOT InOutPositive; }

            return Result;
        };

        auto NoiseRot = InOutState._NoiseRotator;
        NoiseRot.Pitch = Step(Noise.Get_Pitch(), NoiseRot.Pitch, InOutState._PitchNoisePositive);
        NoiseRot.Yaw   = Step(Noise.Get_Yaw(),   NoiseRot.Yaw,   InOutState._YawNoisePositive);
        NoiseRot.Roll  = Step(Noise.Get_Roll(),  NoiseRot.Roll,  InOutState._RollNoisePositive);

        InOutState._NoiseRotator = NoiseRot;

        const auto CurrentRotation = InOutState._CameraTransform.GetRotation().Rotator();
        InOutState._CameraTransform = FTransform(
            CurrentRotation + NoiseRot,
            InOutState._CameraTransform.GetLocation(),
            InOutState._CameraTransform.GetScale3D());
    }
}

// --------------------------------------------------------------------------------------------------------------------
