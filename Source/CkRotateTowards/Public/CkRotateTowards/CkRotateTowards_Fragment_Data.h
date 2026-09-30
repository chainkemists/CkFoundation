#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CoreMinimal.h"
#include "CkRotateTowards_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKROTATETOWARDS_API FCk_Handle_RotateTowards : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RotateTowards); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RotateTowards);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RotateTowards_Mode : uint8
{
    RateLimited,
    Instant
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RotateTowards_Mode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RotateTowards_AxisMode : uint8
{
    Free,
    Locked
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RotateTowards_AxisMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RotateTowards_ClearReason : uint8
{
    Requested,
    TargetLost
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RotateTowards_ClearReason);

// --------------------------------------------------------------------------------------------------------------------

// One rotation axis. A Locked axis holds the entity's current angle; its turn rate is ignored.
USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_RotateTowards_Axis
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RotateTowards_Axis);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_RotateTowards_AxisMode _Mode = ECk_RotateTowards_AxisMode::Free;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "720.0"))
    float _TurnRateDegPerSec = 180.0f;

public:
    CK_PROPERTY(_Mode);
    CK_PROPERTY(_TurnRateDegPerSec);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RotateTowards_Axis, _Mode, _TurnRateDegPerSec);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_RotateTowards_Tunables
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RotateTowards_Tunables);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_RotateTowards_Mode _Mode = ECk_RotateTowards_Mode::RateLimited;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_Axis _Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_Axis _Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_Axis _Roll;

    // "At target" when every axis is within this many degrees of the desired rotation. Never gates stepping.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0"))
    float _ReachedToleranceDeg = 1.0f;

public:
    CK_PROPERTY(_Mode);
    CK_PROPERTY(_Pitch);
    CK_PROPERTY(_Yaw);
    CK_PROPERTY(_Roll);
    CK_PROPERTY(_ReachedToleranceDeg);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RotateTowards_Tunables, _Mode);
};

// --------------------------------------------------------------------------------------------------------------------

// Degrees measured from the rest rotation, both bounds within [-180, 180], Min <= Max. Ignored while Disable.
USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_RotateTowards_AxisRange
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RotateTowards_AxisRange);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_FloatRange _RangeDeg = FCk_FloatRange{-45.0, 45.0};

public:
    CK_PROPERTY(_Enabled);
    CK_PROPERTY(_RangeDeg);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RotateTowards_AxisRange, _Enabled, _RangeDeg);
};

// --------------------------------------------------------------------------------------------------------------------

// Ranges are measured from the REST rotation: the look-at from the entity to _RestReferencePoint. Whenever any
// axis is enabled the rest reference must be a valid Transform other than the entity itself (rejected otherwise).
USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_RotateTowards_RangeClamp
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RotateTowards_RangeClamp);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _RestReferencePoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_AxisRange _Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_AxisRange _Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_AxisRange _Roll;

public:
    CK_PROPERTY(_RestReferencePoint);
    CK_PROPERTY(_Pitch);
    CK_PROPERTY(_Yaw);
    CK_PROPERTY(_Roll);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RotateTowards_RangeClamp, _RestReferencePoint);
};

// --------------------------------------------------------------------------------------------------------------------

// An invalid _Target means "no target yet" (set one later with Request_SetTarget). A range clamp with no
// axis enabled means "no clamp".
USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_RotateTowards_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RotateTowards_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_Tunables _Tunables;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_RangeClamp _RangeClamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_Target);
    CK_PROPERTY(_Tunables);
    CK_PROPERTY(_RangeClamp);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_RotateTowards_Spec, _Target);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_Request_RotateTowards_SetTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RotateTowards_SetTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RotateTowards_SetTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Target;

public:
    CK_PROPERTY_GET(_Target);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RotateTowards_SetTarget, _Target);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_Request_RotateTowards_ClearTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RotateTowards_ClearTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RotateTowards_ClearTarget);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_Request_RotateTowards_UpdateTunables : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RotateTowards_UpdateTunables);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RotateTowards_UpdateTunables);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_Tunables _Tunables;

public:
    CK_PROPERTY_GET(_Tunables);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RotateTowards_UpdateTunables, _Tunables);
};

// --------------------------------------------------------------------------------------------------------------------

// Replaces the whole range clamp. At least one axis must be enabled; use Request_ClearRangeClamp to remove it.
USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_Request_RotateTowards_SetRangeClamp : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RotateTowards_SetRangeClamp);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RotateTowards_SetRangeClamp);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_RotateTowards_RangeClamp _RangeClamp;

public:
    CK_PROPERTY_GET(_RangeClamp);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RotateTowards_SetRangeClamp, _RangeClamp);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_Request_RotateTowards_ClearRangeClamp : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RotateTowards_ClearRangeClamp);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RotateTowards_ClearRangeClamp);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKROTATETOWARDS_API FCk_Request_RotateTowards_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RotateTowards_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RotateTowards_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RotateTowards_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_RotateTowards_OnTargetChanged,
    FCk_Handle_RotateTowards, InHandle,
    FCk_Handle_Transform, InNewTarget,
    FCk_Handle_Transform, InPreviousTarget);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_RotateTowards_OnTargetReached,
    FCk_Handle_RotateTowards, InHandle,
    FCk_Handle_Transform, InTarget);

// --------------------------------------------------------------------------------------------------------------------

// InPreviousTarget is invalid when InReason is TargetLost (the target entity is gone).
DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_RotateTowards_OnTargetCleared,
    FCk_Handle_RotateTowards, InHandle,
    FCk_Handle_Transform, InPreviousTarget,
    ECk_RotateTowards_ClearReason, InReason);

// --------------------------------------------------------------------------------------------------------------------
