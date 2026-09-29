#pragma once

#include "CkSway/CkSway_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::sway
{
    inline constexpr auto kSettledValueEpsilon = 0.01f;     // cm or deg
    inline constexpr auto kSettledVelocityEpsilon = 0.1f;   // cm/s or deg/s
    inline constexpr auto kMaxRotationClampDeg = 45.0f;     // small-angle model bound (DESIGN §5)

    struct CKSWAY_API FChannelState
    {
        FVector _Value = FVector::ZeroVector;
        FVector _Velocity = FVector::ZeroVector;
    };

    /** All fields finite; Max >= 0 per axis; Rotation Max <= kMaxRotationClampDeg; FrequencyHz > 0; DampingRatio >= 0; teleport thresholds >= 0. */
    CKSWAY_API auto Get_IsSpecValid(const FCk_Sway_Spec& InSpec) -> bool;

    /** Unset when InDeltaSeconds <= 0, when either pose is non-finite, or when the step exceeds a non-zero teleport threshold. */
    CKSWAY_API auto Compute_Stimulus(
        const FTransform& InPrevParentWorld,
        const FTransform& InParentWorld,
        float InDeltaSeconds,
        float InTeleportDistanceCm,
        float InTeleportAngleDeg) -> TOptional<FCk_Sway_Stimulus>;

    /** target += -gain * stimulus for every location coupling, then clamped to +-Spec.Location.Max per axis. */
    CKSWAY_API auto Compute_LocationTarget(const FCk_Sway_Spec& InSpec, const FCk_Sway_Stimulus& InStimulus) -> FVector;

    /** Same rule for the rotation couplings; result is (Roll, Pitch, Yaw) deg clamped to +-Spec.Rotation.Max. */
    CKSWAY_API auto Compute_RotationTarget(const FCk_Sway_Spec& InSpec, const FCk_Sway_Stimulus& InStimulus) -> FVector;

    /** FMath::SpringDamper with zero target rate, then clamp Value to +-Max per axis; a clamped axis has its outward velocity zeroed. No-op when InDeltaSeconds <= 0. */
    CKSWAY_API auto Step_Channel(FChannelState& InOutState, const FVector& InTarget, float InDeltaSeconds, const FCk_Sway_Response& InResponse) -> void;

    CKSWAY_API auto Get_IsSettled(const FChannelState& InState, float InValueEpsilon = kSettledValueEpsilon, float InVelocityEpsilon = kSettledVelocityEpsilon) -> bool;

    /** FTransform{FRotator{Pitch = InRotationDeg.Y, Yaw = InRotationDeg.Z, Roll = InRotationDeg.X}, InLocationCm}. */
    CKSWAY_API auto Compose_Offset(const FVector& InLocationCm, const FVector& InRotationDeg) -> FTransform;

    /** Node offset written to the scene node: the spring offset applied in the rest frame, then the rest. */
    CKSWAY_API auto Compose_NodeOffset(const FTransform& InSwayOffset, const FTransform& InRestOffset) -> FTransform;  // = InSwayOffset * InRestOffset
}

// --------------------------------------------------------------------------------------------------------------------
