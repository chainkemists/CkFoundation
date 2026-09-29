#include "CkSway/CkSway_Kernel.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_sway_kernel
{
    auto
        DoGet_IsFinite(
            const FVector& InVector)
        -> bool
    {
        return NOT InVector.ContainsNaN();
    }

    auto
        DoGet_IsResponseValid(
            const FCk_Sway_Response& InResponse)
        -> bool
    {
        const auto& Max = InResponse.Get_Max();
        return DoGet_IsFinite(Max) && Max.X >= 0.0 && Max.Y >= 0.0 && Max.Z >= 0.0
            && FMath::IsFinite(InResponse.Get_FrequencyHz()) && InResponse.Get_FrequencyHz() > 0.0f
            && FMath::IsFinite(InResponse.Get_DampingRatio()) && InResponse.Get_DampingRatio() >= 0.0f;
    }

    auto
        DoClamp_PerAxis(
            const FVector& InValue,
            const FVector& InMax)
        -> FVector
    {
        return FVector{
            FMath::Clamp(InValue.X, -InMax.X, InMax.X),
            FMath::Clamp(InValue.Y, -InMax.Y, InMax.Y),
            FMath::Clamp(InValue.Z, -InMax.Z, InMax.Z)};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Get_IsSpecValid(
        const FCk_Sway_Spec& InSpec)
    -> bool
{
    if (NOT ck_sway_kernel::DoGet_IsResponseValid(InSpec.Get_Location()) || NOT ck_sway_kernel::DoGet_IsResponseValid(InSpec.Get_Rotation()))
    { return false; }

    const auto& RotationMax = InSpec.Get_Rotation().Get_Max();
    if (RotationMax.X > kMaxRotationClampDeg || RotationMax.Y > kMaxRotationClampDeg || RotationMax.Z > kMaxRotationClampDeg)
    { return false; }

    return ck_sway_kernel::DoGet_IsFinite(InSpec.Get_RotationFromAngularVelocity())
        && ck_sway_kernel::DoGet_IsFinite(InSpec.Get_LocationFromLinearVelocity())
        && FMath::IsFinite(InSpec.Get_LateralCmFromYawRate())
        && FMath::IsFinite(InSpec.Get_VerticalCmFromPitchRate())
        && FMath::IsFinite(InSpec.Get_RollDegFromLateralVelocity())
        && FMath::IsFinite(InSpec.Get_PitchDegFromForwardVelocity())
        && FMath::IsFinite(InSpec.Get_TeleportDistanceCm()) && InSpec.Get_TeleportDistanceCm() >= 0.0f
        && FMath::IsFinite(InSpec.Get_TeleportAngleDeg()) && InSpec.Get_TeleportAngleDeg() >= 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Compute_Stimulus(
        const FTransform& InPrevParentWorld,
        const FTransform& InParentWorld,
        float InDeltaSeconds,
        float InTeleportDistanceCm,
        float InTeleportAngleDeg)
    -> TOptional<FCk_Sway_Stimulus>
{
    if (NOT FMath::IsFinite(InDeltaSeconds) || InDeltaSeconds <= 0.0f)
    { return {}; }

    if (InPrevParentWorld.ContainsNaN() || InParentWorld.ContainsNaN())
    { return {}; }

    const auto Delta = InParentWorld.GetLocation() - InPrevParentWorld.GetLocation();
    auto LocalDeltaQ = InPrevParentWorld.GetRotation().Inverse() * InParentWorld.GetRotation();
    LocalDeltaQ.Normalize();
    // GetAngle is 2*acos(W): force the shortest arc so a small turn is never reported the long way round.
    if (LocalDeltaQ.W < 0.0)
    { LocalDeltaQ = -LocalDeltaQ; }

    if (InTeleportDistanceCm > 0.0f && Delta.Size() > InTeleportDistanceCm)
    { return {}; }

    if (InTeleportAngleDeg > 0.0f && FMath::RadiansToDegrees(LocalDeltaQ.GetAngle()) > InTeleportAngleDeg)
    { return {}; }

    const auto LinearVelocity = InParentWorld.GetRotation().UnrotateVector(Delta / InDeltaSeconds);
    const auto R = LocalDeltaQ.Rotator();
    const auto AngularVelocityDeg = FVector{R.Roll, R.Pitch, R.Yaw} / InDeltaSeconds;

    if (NOT ck_sway_kernel::DoGet_IsFinite(LinearVelocity) || NOT ck_sway_kernel::DoGet_IsFinite(AngularVelocityDeg))
    { return {}; }

    return FCk_Sway_Stimulus{LinearVelocity, AngularVelocityDeg};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Compute_LocationTarget(
        const FCk_Sway_Spec& InSpec,
        const FCk_Sway_Stimulus& InStimulus)
    -> FVector
{
    const auto& V = InStimulus.Get_LinearVelocity();
    const auto& W = InStimulus.Get_AngularVelocityDeg();

    auto Target = -(InSpec.Get_LocationFromLinearVelocity() * V);
    Target.Y += -InSpec.Get_LateralCmFromYawRate() * W.Z;
    Target.Z += -InSpec.Get_VerticalCmFromPitchRate() * W.Y;

    return ck_sway_kernel::DoClamp_PerAxis(Target, InSpec.Get_Location().Get_Max());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Compute_RotationTarget(
        const FCk_Sway_Spec& InSpec,
        const FCk_Sway_Stimulus& InStimulus)
    -> FVector
{
    const auto& V = InStimulus.Get_LinearVelocity();
    const auto& W = InStimulus.Get_AngularVelocityDeg();

    auto Target = -(InSpec.Get_RotationFromAngularVelocity() * W);
    Target.X += -InSpec.Get_RollDegFromLateralVelocity() * V.Y;
    Target.Y += -InSpec.Get_PitchDegFromForwardVelocity() * V.X;

    return ck_sway_kernel::DoClamp_PerAxis(Target, InSpec.Get_Rotation().Get_Max());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Step_Channel(
        FChannelState& InOutState,
        const FVector& InTarget,
        float InDeltaSeconds,
        const FCk_Sway_Response& InResponse)
    -> void
{
    if (InDeltaSeconds <= 0.0f)
    { return; }

    FMath::SpringDamper(InOutState._Value, InOutState._Velocity, InTarget, FVector::ZeroVector,
        InDeltaSeconds, InResponse.Get_FrequencyHz(), InResponse.Get_DampingRatio());

    const auto& Max = InResponse.Get_Max();
    for (auto Axis = 0; Axis < 3; ++Axis)
    {
        if (FMath::Abs(InOutState._Value[Axis]) <= Max[Axis])
        { continue; }

        const auto Sign = FMath::Sign(InOutState._Value[Axis]);
        InOutState._Value[Axis] = Sign * Max[Axis];

        if (InOutState._Velocity[Axis] * Sign > 0.0)
        { InOutState._Velocity[Axis] = 0.0; }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Get_IsSettled(
        const FChannelState& InState,
        float InValueEpsilon,
        float InVelocityEpsilon)
    -> bool
{
    return InState._Value.GetAbsMax() <= InValueEpsilon && InState._Velocity.GetAbsMax() <= InVelocityEpsilon;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Compose_Offset(
        const FVector& InLocationCm,
        const FVector& InRotationDeg)
    -> FTransform
{
    return FTransform{FRotator{InRotationDeg.Y, InRotationDeg.Z, InRotationDeg.X}, InLocationCm};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::sway::Compose_NodeOffset(
        const FTransform& InSwayOffset,
        const FTransform& InRestOffset)
    -> FTransform
{
    // FTransform A * B applies A first, then B: the sway is expressed in the rest frame.
    return InSwayOffset * InRestOffset;
}

// --------------------------------------------------------------------------------------------------------------------
