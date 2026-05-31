#pragma once

#include "CkCamera/Camera/Profile/CkCameraProfile.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The POV pipeline — a stateless helper that operates on a persistent FPov_State (stored on the director's Current
// fragment) plus the composed profile and per-frame inputs.
//
// Input-independent: OrientationIntention defaults to zero (a fixed/follow camera needs no input). Whoever owns
// input pushes an intention each frame; the helper just consumes the value.
//
// Pipeline: attachment + spring-lagged pivot, boom rotation (orientation control + auto-reorient look-at), boom-end,
// framing, camera transform, collision push-in, noise.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::camera
{
    // Per-frame inputs to the POV pipeline.
    struct CKCAMERA_API FPov_Input
    {
    public:
        // World transform of the anchor entity (boom pivot). Sampled this frame, post-Transform groups.
        FTransform _AnchorTransform = FTransform::Identity;

        // Abstract orbit intention (X = yaw, Y = pitch), pre-scaled by the caller. Zero = no manual orbit.
        FVector _OrientationIntention = FVector::ZeroVector;

        float _DeltaSeconds = 0.0f;

        // Look-at target world location for auto-reorient (lock-on). Input-free — driven by a target entity.
        // Unset = no look-at (a zero vector is a valid target, so presence can't be encoded in the value).
        TOptional<FVector> _LookAtLocation;

        // For collision traces (may be null — collision is then skipped).
        TWeakObjectPtr<UWorld> _World;
        TWeakObjectPtr<AActor> _TraceIgnoreActor;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Persistent POV state carried on the director across frames. Plain struct (internal runtime state, not edited
    // in-editor) so it can hold a TOptional and be mutated directly by the camera processor + FPov.
    struct CKCAMERA_API FPov_State
    {
    public:
        bool       _Initialized = false;

        FRotator   _BoomArmRotation = FRotator::ZeroRotator;

        FTransform _GroupBaseAttachmentTransform = FTransform::Identity;
        FVector    _GroupBaseLocation            = FVector::ZeroVector;

        // Smoothed look-at location (lagged toward the target); equals group-base when there is no look-at.
        FVector    _LookAtLocation = FVector::ZeroVector;

        FTransform _BoomArmEndTransform = FTransform::Identity;
        FTransform _FramingTransform    = FTransform::Identity;
        FTransform _CameraTransform     = FTransform::Identity;

        TOptional<float> _CollisionDistance;

        bool     _PitchNoisePositive = true;
        bool     _YawNoisePositive   = true;
        bool     _RollNoisePositive  = true;
        FRotator _NoiseRotator       = FRotator::ZeroRotator;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FPov
    {
    public:
        // Advances InOutState by one frame. After this, InOutState._CameraTransform holds the resolved POV.
        static auto
        Run(
            const FCk_CameraProfile& InProfile,
            const FPov_Input& InInput,
            FPov_State& InOutState) -> void;

    private:
        static auto Step_Attachment   (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_LookAt       (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_BoomRotation (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_BoomEnd      (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_Framing      (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_CameraXform  (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_Collision    (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;
        static auto Step_Noise        (const FCk_CameraProfile&, const FPov_Input&, FPov_State&) -> void;

        // Orbit rotation delta from the (already-scaled) orientation intention. Returns zero when intention is
        // zero or orientation control is disabled — the input-independent path.
        static auto Compute_OrientationControl(
            const FCk_CameraProfile&, const FPov_Input&, const FPov_State&) -> FRotator;

        // Reorients the boom toward the look-at target (lock-on). Input-free; no-op without a look-at target or
        // when auto-reorient is disabled.
        static auto Apply_AutoReorient(
            const FCk_CameraProfile&, const FPov_Input&, FPov_State&, FRotator& InOutRotation) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
