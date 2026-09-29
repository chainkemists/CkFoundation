#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CoreMinimal.h"
#include "CkSway_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSWAY_API FCk_Handle_Sway : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Sway); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Sway);

// --------------------------------------------------------------------------------------------------------------------

// One sprung channel. _Max clamps both the target and the output, per axis (cm for Location, deg for Rotation).
USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Sway_Response
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sway_Response);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Max = FVector{8.0, 8.0, 8.0};

    // Undamped natural frequency. Higher = snappier return to rest.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.01", UIMin = "0.01", UIMax = "20.0"))
    float _FrequencyHz = 4.0f;

    // 1 = critically damped (no overshoot); < 1 wobbles; > 1 sluggish.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
    float _DampingRatio = 0.8f;

public:
    CK_PROPERTY(_Max);
    CK_PROPERTY(_FrequencyHz);
    CK_PROPERTY(_DampingRatio);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sway_Response, _Max, _FrequencyHz, _DampingRatio);
};

// --------------------------------------------------------------------------------------------------------------------

// Every coupling follows one rule: target += -gain * stimulus. Positive gain lags (opposes the motion).
// Stimulus is in the DRIVER's local frame (UCk_Utils_SceneNode_UE::Get_DriverWorldTransform); offsets are in the rest
// frame. X forward, Y right, Z up; Roll/Pitch/Yaw about X/Y/Z.
USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Sway_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sway_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channels", meta = (AllowPrivateAccess = true))
    FCk_Sway_Response _Location = FCk_Sway_Response{FVector{8.0, 8.0, 8.0}, 3.5f, 0.75f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channels", meta = (AllowPrivateAccess = true))
    FCk_Sway_Response _Rotation = FCk_Sway_Response{FVector{6.0, 6.0, 6.0}, 4.5f, 0.8f};

    // deg of offset per deg/s of driver angular velocity, per matching axis (Roll, Pitch, Yaw).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Couplings", meta = (AllowPrivateAccess = true))
    FVector _RotationFromAngularVelocity = FVector{0.008, 0.008, 0.008};

    // cm of offset per cm/s of driver linear velocity, per matching axis (X, Y, Z).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Couplings", meta = (AllowPrivateAccess = true))
    FVector _LocationFromLinearVelocity = FVector{0.008, 0.008, 0.008};

    // cm of lateral (Y) offset per deg/s of yaw rate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Couplings", meta = (AllowPrivateAccess = true))
    float _LateralCmFromYawRate = 0.02f;

    // cm of vertical (Z) offset per deg/s of pitch rate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Couplings", meta = (AllowPrivateAccess = true))
    float _VerticalCmFromPitchRate = 0.02f;

    // deg of roll per cm/s of lateral (Y) velocity.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Couplings", meta = (AllowPrivateAccess = true))
    float _RollDegFromLateralVelocity = 0.0f;

    // deg of pitch per cm/s of forward (X) velocity.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Couplings", meta = (AllowPrivateAccess = true))
    float _PitchDegFromForwardVelocity = 0.0f;

    // A driver step larger than this is a teleport: no stimulus that frame, previous pose reseeded. 0 = off.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guards", meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0"))
    float _TeleportDistanceCm = 0.0f;

    // A driver turn larger than this (total angle) is a teleport. 0 = off.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guards", meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", ClampMax = "180.0"))
    float _TeleportAngleDeg = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guards", meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_Location);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_RotationFromAngularVelocity);
    CK_PROPERTY(_LocationFromLinearVelocity);
    CK_PROPERTY(_LateralCmFromYawRate);
    CK_PROPERTY(_VerticalCmFromPitchRate);
    CK_PROPERTY(_RollDegFromLateralVelocity);
    CK_PROPERTY(_PitchDegFromForwardVelocity);
    CK_PROPERTY(_TeleportDistanceCm);
    CK_PROPERTY(_TeleportAngleDeg);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sway_Spec, _Location, _Rotation);
};

// --------------------------------------------------------------------------------------------------------------------

// Driver-local motion measured this frame. Angular velocity is (Roll, Pitch, Yaw) in deg/s.
USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Sway_Stimulus
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sway_Stimulus);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _LinearVelocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _AngularVelocityDeg = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_LinearVelocity);
    CK_PROPERTY_GET(_AngularVelocityDeg);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sway_Stimulus, _LinearVelocity, _AngularVelocityDeg);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Request_Sway_UpdateSpec : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sway_UpdateSpec);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sway_UpdateSpec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Sway_Spec _Spec;

public:
    CK_PROPERTY_GET(_Spec);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sway_UpdateSpec, _Spec);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Request_Sway_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sway_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sway_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sway_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

// Zeroes both channels (value + velocity), reseeds the previous driver pose; the node returns to its rest offset.
USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Request_Sway_Reset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sway_Reset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sway_Reset);
};

// --------------------------------------------------------------------------------------------------------------------

// Replaces the rest pose Sway composes its spring offset onto (node offset = SwayOffset * Rest). Rejected when non-finite.
USTRUCT(BlueprintType)
struct CKSWAY_API FCk_Request_Sway_SetRestOffset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sway_SetRestOffset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sway_SetRestOffset);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _RestOffset = FTransform::Identity;

public:
    CK_PROPERTY_GET(_RestOffset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sway_SetRestOffset, _RestOffset);
};

// --------------------------------------------------------------------------------------------------------------------
