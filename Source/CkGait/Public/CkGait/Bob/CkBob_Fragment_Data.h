#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkGait/Gait/CkGait_Fragment_Data.h"

#include "CoreMinimal.h"
#include "CkBob_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGAIT_API FCk_Handle_Bob : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Bob); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Bob);

// --------------------------------------------------------------------------------------------------------------------

// The vertical spring that carries air lift and the landing dip. Low damping = bouncy.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Bob_SpringResponse
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Bob_SpringResponse);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.01", UIMin = "0.01", UIMax = "20.0"))
    float _FrequencyHz = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
    float _DampingRatio = 0.35f;

public:
    CK_PROPERTY(_FrequencyHz);
    CK_PROPERTY(_DampingRatio);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Bob_SpringResponse, _FrequencyHz, _DampingRatio);
};

// --------------------------------------------------------------------------------------------------------------------

// Stride-driven offsets at Amount = 1, in the rest frame.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Bob_StrideParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Bob_StrideParams);

private:
    // Dip at each footfall (cm). Shape |sin(phase)|.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _VerticalCm = 1.6f;

    // Side-to-side drift over a stride (cm). Shape sin(phase).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _LateralCm = 1.2f;

    // Constant forward push while moving (cm), scaled by Amount.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _ForwardCm = 0.0f;

    // Roll with the side-to-side drift (deg).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "45.0"))
    float _RollDeg = 2.5f;

    // Nod at each footfall (deg, positive tips down).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "45.0"))
    float _PitchDeg = 1.5f;

public:
    CK_PROPERTY(_VerticalCm);
    CK_PROPERTY(_LateralCm);
    CK_PROPERTY(_ForwardCm);
    CK_PROPERTY(_RollDeg);
    CK_PROPERTY(_PitchDeg);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Bob_StrideParams, _VerticalCm, _LateralCm);
};

// --------------------------------------------------------------------------------------------------------------------

// Airborne lift and the landing dip, carried by one vertical spring.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Bob_AirParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Bob_AirParams);

private:
    // Lift while falling, per cm/s of fall speed (cm), clamped to +-_MaxLiftCm.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _LiftCmPerFallSpeed = 0.006f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MaxLiftCm = 5.0f;

    // Downward spring kick on landing per cm/s of impact speed (cm/s of spring velocity), capped at _MaxLandKick.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _LandKickPerImpactSpeed = 0.09f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MaxLandKick = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Bob_SpringResponse _Spring;

public:
    CK_PROPERTY(_LiftCmPerFallSpeed);
    CK_PROPERTY(_MaxLiftCm);
    CK_PROPERTY(_LandKickPerImpactSpeed);
    CK_PROPERTY(_MaxLandKick);
    CK_PROPERTY(_Spring);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Bob_AirParams, _LiftCmPerFallSpeed, _MaxLiftCm);
};

// --------------------------------------------------------------------------------------------------------------------

// Procedural locomotion bob of a scene node, shaped by a Gait. Offsets are in the node's REST frame (X forward, Y right,
// Z up; Roll/Pitch/Yaw about X/Y/Z). Every amplitude is at Amount = 1 and is multiplied by _Intensity.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Bob_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Bob_Spec);

private:
    // The gait that shapes this bob. A config-authored spec leaves it unset and receives it at composition
    // (Set_Gait); Request_UpdateSpec re-points it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source", meta = (AllowPrivateAccess = true))
    FCk_Handle_Gait _Gait;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride", meta = (AllowPrivateAccess = true))
    FCk_Bob_StrideParams _Stride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air", meta = (AllowPrivateAccess = true))
    FCk_Bob_AirParams _Air;

    // First-order smoothing of the whole bob target (1/s). 0 = none. Gives the loose, camera-trails-the-head feel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing", meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "40.0"))
    float _LagRate = 0.0f;

    // Rise and fall while standing still (cm); fades out as Amount rises.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breath", meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _BreathCm = 0.4f;

    // Scales the whole bob (0 = off, 1 = authored). The hook for a comfort/accessibility setting.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float _Intensity = 1.0f;

    // The location offset never exceeds this magnitude (cm), whatever the spring does.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MaxOffsetCm = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guards", meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_Gait);
    CK_PROPERTY(_Stride);
    CK_PROPERTY(_Air);
    CK_PROPERTY(_LagRate);
    CK_PROPERTY(_BreathCm);
    CK_PROPERTY(_Intensity);
    CK_PROPERTY(_MaxOffsetCm);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Bob_Spec, _Gait, _Stride, _Air);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Bob_UpdateSpec : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Bob_UpdateSpec);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Bob_UpdateSpec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Bob_Spec _Spec;

public:
    CK_PROPERTY_GET(_Spec);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Bob_UpdateSpec, _Spec);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Bob_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Bob_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Bob_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Bob_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

// Zeroes the spring and the smoothed target and re-seeds the consumed landing count from the gait; the node returns to
// its rest offset through Update's normal publish path.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Bob_Reset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Bob_Reset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Bob_Reset);
};

// --------------------------------------------------------------------------------------------------------------------

// Replaces the rest pose Bob composes its offset onto (node offset = BobOffset * Rest). Rejected when non-finite.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Bob_SetRestOffset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Bob_SetRestOffset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Bob_SetRestOffset);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _RestOffset = FTransform::Identity;

public:
    CK_PROPERTY_GET(_RestOffset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Bob_SetRestOffset, _RestOffset);
};

// --------------------------------------------------------------------------------------------------------------------
