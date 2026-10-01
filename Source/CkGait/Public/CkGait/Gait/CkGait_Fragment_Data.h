#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CoreMinimal.h"
#include "CkGait_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGAIT_API FCk_Handle_Gait : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Gait); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Gait);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Gait_Footing : uint8
{
    Grounded,
    Airborne
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Gait_Footing);

UENUM(BlueprintType)
enum class ECk_Gait_Stance : uint8
{
    Standing,
    Crouched
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Gait_Stance);

// --------------------------------------------------------------------------------------------------------------------

class UNavMovementComponent;

// The stride clock: how speed maps to Amount and cadence.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Gait_StrideParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Gait_StrideParams);

private:
    // Ground speed (cm/s) at which Amount reaches 1 and the clock turns at _StridesPerSecond.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0", UIMin = "1.0"))
    float _ReferenceSpeed = 420.0f;

    // Full left-right-left cycles per second at _ReferenceSpeed; a footfall every half cycle.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "4.0"))
    float _StridesPerSecond = 1.6f;

    // Amount keeps growing past _ReferenceSpeed (sprint) up to this multiple.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
    float _MaxAmountScale = 1.5f;

    // How quickly Amount follows its target when starting and stopping (1/s).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0"))
    float _AmountInterpSpeed = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float _CrouchScale = 0.6f;

    // The clock keeps turning at this fraction of the reference cadence while slower (or at rest), so a new step does
    // not always begin at the same phase.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float _MinCadenceScale = 0.35f;

public:
    CK_PROPERTY(_ReferenceSpeed);
    CK_PROPERTY(_StridesPerSecond);
    CK_PROPERTY(_MaxAmountScale);
    CK_PROPERTY(_AmountInterpSpeed);
    CK_PROPERTY(_CrouchScale);
    CK_PROPERTY(_MinCadenceScale);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Gait_StrideParams, _ReferenceSpeed, _StridesPerSecond);
};

// --------------------------------------------------------------------------------------------------------------------

// The locomotion rhythm of a character: how fast the stride clock turns and how large the envelope (Amount) is.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Gait_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Gait_Spec);

private:
    // The motion source: Velocity, IsFalling() and IsCrouching() are sampled from it every frame. Any UNavMovementComponent
    // (character, floating pawn, a test's) is a valid source; Request_UpdateSpec re-points it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source", meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UNavMovementComponent> _MovementComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stride", meta = (AllowPrivateAccess = true))
    FCk_Gait_StrideParams _Stride;

    // Seconds per breath; consumers scale the breath by (1 - Amount).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breath", meta = (AllowPrivateAccess = true, ClampMin = "0.1", UIMin = "0.1", UIMax = "10.0"))
    float _BreathPeriodSeconds = 3.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guards", meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_MovementComponent);
    CK_PROPERTY(_Stride);
    CK_PROPERTY(_BreathPeriodSeconds);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Gait_Spec, _MovementComponent, _Stride);
};

// --------------------------------------------------------------------------------------------------------------------

// One frame's sample of the character's motion. Velocity is in WORLD cm/s.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Gait_Motion
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Gait_Motion);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_Gait_Footing _Footing = ECk_Gait_Footing::Grounded;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_Gait_Stance _Stance = ECk_Gait_Stance::Standing;

public:
    CK_PROPERTY_GET(_Velocity);
    CK_PROPERTY_GET(_Footing);
    CK_PROPERTY_GET(_Stance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Gait_Motion, _Velocity, _Footing, _Stance);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Gait_UpdateSpec : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Gait_UpdateSpec);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Gait_UpdateSpec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Gait_Spec _Spec;

public:
    CK_PROPERTY_GET(_Spec);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Gait_UpdateSpec, _Spec);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Gait_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Gait_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Gait_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Gait_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

// Zeroes the clock (phase, amount, breath) and the previous motion sample. The landing counter is NOT reset.
USTRUCT(BlueprintType)
struct CKGAIT_API FCk_Request_Gait_Reset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Gait_Reset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Gait_Reset);
};

// --------------------------------------------------------------------------------------------------------------------
