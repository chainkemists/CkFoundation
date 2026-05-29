#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <AlphaBlend.h>
#include <CoreMinimal.h>

#include "CkGameplayCamera_Profile.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// The composed camera profile — the accumulated result of every active modifier's contribution. Ported from the
// reference FTuq_Camera_Profile_Params, adapted to CkFoundation conventions. The bone-name field is intentionally
// absent: the anchor is a FCk_Handle_Transform set up by the caller (socket-following transform entity), so the
// camera module never needs a bone name. Spring settings are added alongside the spring interpolator in the POV
// increment.
//
// Each leaf uses CK_PROPERTY (get + set) so BlendInto / modifiers can mutate the running profile.
// --------------------------------------------------------------------------------------------------------------------

// Generic rotation tuning (speed + limits + out-of-range behaviour). Reused by OrientationControl/AutoReorient/Noise.
USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_Rotation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_Rotation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Speed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_FloatRange _Limits = FCk_FloatRange(-180.0f, 180.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_FloatRange _NormalSpeedRange = FCk_FloatRange(-90.0f, 90.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _OutOfRangeMultiplier = 0.5f;

public:
    CK_PROPERTY(_Speed);
    CK_PROPERTY(_Limits);
    CK_PROPERTY(_NormalSpeedRange);
    CK_PROPERTY(_OutOfRangeMultiplier);
};

// --------------------------------------------------------------------------------------------------------------------

// Boom-arm rig: pivot offset, boom length, and framing offset/rotation applied at the boom end.
USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_Rig
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_Rig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _BoomArmPivotOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _BoomArmLength = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _FramingOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _FramingPitch = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _FramingYaw = 0.0f;

    // Fixed-angle cameras (top-down / isometric / fixed store cam): when set, the boom holds this absolute
    // rotation instead of the input-driven / anchor-seeded rotation. A negative pitch lifts the camera above the
    // pivot and looks down at it. Has no effect on orientation-control or auto-reorient modifiers (they drive the
    // boom themselves) — only meaningful when both are off.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _UseFixedBoomRotation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_UseFixedBoomRotation"))
    FRotator _FixedBoomRotation = FRotator::ZeroRotator;

public:
    CK_PROPERTY(_BoomArmPivotOffset);
    CK_PROPERTY(_BoomArmLength);
    CK_PROPERTY(_FramingOffset);
    CK_PROPERTY(_FramingPitch);
    CK_PROPERTY(_FramingYaw);
    CK_PROPERTY(_UseFixedBoomRotation);
    CK_PROPERTY(_FixedBoomRotation);
};

// --------------------------------------------------------------------------------------------------------------------

// Location-lag settings. A pragmatic stand-in for the reference's critically-damped spring interpolator:
// an FMath::VInterpTo speed per smoothed location (0 = instant / no lag). The full spring can replace this later
// without touching the profile's public shape (just the interpolation math in FPov).
USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_Springs
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_Springs);

private:
    // Follow lag for the boom pivot (group-base) location. 0 = instant (snap to anchor).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _GroupBaseLocationInterpSpeed = 0.0f;

    // Follow lag for the look-at location. 0 = instant.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _LookAtLocationInterpSpeed = 0.0f;

public:
    CK_PROPERTY(_GroupBaseLocationInterpSpeed);
    CK_PROPERTY(_LookAtLocationInterpSpeed);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_Sensor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_Sensor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _FOV = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ConstrainAspectRatio = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _AspectRatio = 1.77777777f;

public:
    CK_PROPERTY(_FOV);
    CK_PROPERTY(_ConstrainAspectRatio);
    CK_PROPERTY(_AspectRatio);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_Noise
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_Noise);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Roll;

public:
    CK_PROPERTY(_Pitch);
    CK_PROPERTY(_Yaw);
    CK_PROPERTY(_Roll);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_OrientationControl
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_OrientationControl);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FAlphaBlend _XIntentionCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FAlphaBlend _YIntentionCurve;

public:
    CK_PROPERTY(_Pitch);
    CK_PROPERTY(_Yaw);
    CK_PROPERTY(_XIntentionCurve);
    CK_PROPERTY(_YIntentionCurve);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_AutoReorient
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_AutoReorient);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rotation _Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _LookAtTargetLocationPitchOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _LookAtTargetLocationYawOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _LookAtCameraLocationPitchOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _LookAtCameraLocationYawOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _HasOrientationControlOverrideTimeout = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _OrientationControlOverrideTimeout = FCk_Time{};

public:
    CK_PROPERTY(_Pitch);
    CK_PROPERTY(_Yaw);
    CK_PROPERTY(_LookAtTargetLocationPitchOffset);
    CK_PROPERTY(_LookAtTargetLocationYawOffset);
    CK_PROPERTY(_LookAtCameraLocationPitchOffset);
    CK_PROPERTY(_LookAtCameraLocationYawOffset);
    CK_PROPERTY(_HasOrientationControlOverrideTimeout);
    CK_PROPERTY(_OrientationControlOverrideTimeout);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_Collision
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_Collision);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _UseAsyncTrace = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _NumPredictionWhiskers = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _MaxWhiskerAngle = 75.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _AdditionalOffsetFromHit = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ClippingSpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ReturningSpeed = 150.0f;

public:
    CK_PROPERTY(_UseAsyncTrace);
    CK_PROPERTY(_NumPredictionWhiskers);
    CK_PROPERTY(_MaxWhiskerAngle);
    CK_PROPERTY(_AdditionalOffsetFromHit);
    CK_PROPERTY(_ClippingSpeed);
    CK_PROPERTY(_ReturningSpeed);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile_DepthOfField
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile_DepthOfField);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0", ClampMax = "32.0", DisplayName = "Aperture (F-stop)"))
    float _Fstop = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "1.0", UIMax = "10000.0", DisplayName = "Focal Distance"))
    float _FocalDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.1", DisplayName = "Sensor Width (mm)"))
    float _SensorWidth = 24.576f;

public:
    CK_PROPERTY(_Fstop);
    CK_PROPERTY(_FocalDistance);
    CK_PROPERTY(_SensorWidth);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCAMERA_API FCk_GameplayCamera_Profile
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayCamera_Profile);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Rig _Rig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Springs _Springs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Sensor _Sensor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_GameplayCamera_Profile_Noise _Noise;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _HasOrientationControl = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_HasOrientationControl"))
    FCk_GameplayCamera_Profile_OrientationControl _OrientationControl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _HasAutoReorient = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_HasAutoReorient"))
    FCk_GameplayCamera_Profile_AutoReorient _AutoReorient;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _HasCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_HasCollision"))
    FCk_GameplayCamera_Profile_Collision _Collision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _UsePostProcess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_UsePostProcess"))
    FCk_GameplayCamera_Profile_DepthOfField _DepthOfField;

public:
    CK_PROPERTY(_Rig);
    CK_PROPERTY(_Springs);
    CK_PROPERTY(_Sensor);
    CK_PROPERTY(_Noise);
    CK_PROPERTY(_HasOrientationControl);
    CK_PROPERTY(_OrientationControl);
    CK_PROPERTY(_HasAutoReorient);
    CK_PROPERTY(_AutoReorient);
    CK_PROPERTY(_HasCollision);
    CK_PROPERTY(_Collision);
    CK_PROPERTY(_UsePostProcess);
    CK_PROPERTY(_DepthOfField);
};

// --------------------------------------------------------------------------------------------------------------------
