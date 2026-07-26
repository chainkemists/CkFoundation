#pragma once

#include "CkCamera/Camera/Profile/CkCameraProfile.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::camera
{
    struct CKCAMERA_API FPov_Input
    {
    public:
        // Boom pivot; must be sampled this frame, post-Transform groups.
        FTransform _AnchorTransform = FTransform::Identity;

        // X = yaw, Y = pitch, pre-scaled by the caller. Zero = no manual orbit.
        FVector _OrientationIntention = FVector::ZeroVector;

        float _DeltaSeconds = 0.0f;

        // Unset = no look-at (a zero vector is a valid target, so it cannot encode absence).
        TOptional<FVector> _LookAtLocation;

        // For collision traces (may be null — collision is then skipped).
        TWeakObjectPtr<UWorld> _World;
        TWeakObjectPtr<AActor> _TraceIgnoreActor;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FPov_State
    {
    public:
        bool       _Initialized = false;

        FRotator   _BoomArmRotation = FRotator::ZeroRotator;

        FTransform _GroupBaseAttachmentTransform = FTransform::Identity;
        FVector    _GroupBaseLocation            = FVector::ZeroVector;

        // Lagged toward the target; equals group-base when there is no look-at.
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

        // Zero when the intention is zero or orientation control is disabled.
        static auto Compute_OrientationControl(
            const FCk_CameraProfile&, const FPov_Input&, const FPov_State&) -> FRotator;

        // No-op without a look-at target or when auto-reorient is disabled.
        static auto Apply_AutoReorient(
            const FCk_CameraProfile&, const FPov_Input&, FPov_State&, FRotator& InOutRotation) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
