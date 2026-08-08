#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <InputCoreTypes.h>

#include "CkInputBias_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_InputBias_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InputBias_AxisInversion : uint8
{
    Normal,
    Inverted
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputBias_AxisInversion);

// --------------------------------------------------------------------------------------------------------------------

/**
 * How one axis key is conditioned on its way from the raw inbox to the value gameplay samples.
 *
 * The stages apply in a fixed order — inversion, deadzone, exponent, sensitivity — and every stage after
 * inversion operates on the MAGNITUDE and restores the sign, so an axis never changes direction partway
 * through. Every field defaults to the identity transform, so a row left at its defaults conditions nothing.
 *
 * Ranges, all rejected at the composition and request boundaries rather than clamped: deadzone in [0, 1)
 * (a deadzone of 1 would kill the axis outright and makes the rescale undefined), exponent > 0 and
 * sensitivity > 0 (zero or negative inverts or flattens the response in ways no caller means to ask for).
 *
 * The stages assume a normalised axis, where full deflection is 1. An unnormalised axis — a mouse delta in
 * pixels — is still conditioned, but only sensitivity means anything on it; a deadzone or exponent applied to
 * a pixel delta measures against a full-deflection anchor that axis does not have.
 */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_InputBias_AxisBias
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InputBias_AxisBias);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _AxisKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Deadzone = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Exponent = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Sensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputBias_AxisInversion _Inversion = ECk_InputBias_AxisInversion::Normal;

public:
    CK_PROPERTY_GET(_AxisKey);
    CK_PROPERTY(_Deadzone);
    CK_PROPERTY(_Exponent);
    CK_PROPERTY(_Sensitivity);
    CK_PROPERTY(_Inversion);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InputBias_AxisBias, _AxisKey);
};

// --------------------------------------------------------------------------------------------------------------------

// The latest sample of one axis, kept as the pair it arrived as. Both halves are stored because conditioning is
// lossy and not invertible — a deadzone maps a whole band onto zero — so anything that needs the physical fact
// (a recording, a debugger, an ordering consumer) has to be able to read it rather than derive it back.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_InputBias_ConditionedAxis
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InputBias_ConditionedAxis);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _AxisKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _RawValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ConditionedValue = 0.0f;

public:
    CK_PROPERTY_GET(_AxisKey);
    CK_PROPERTY(_RawValue);
    CK_PROPERTY(_ConditionedValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InputBias_ConditionedAxis, _AxisKey);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINPUT_API FCk_Handle_InputBias : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InputBias); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InputBias);

// --------------------------------------------------------------------------------------------------------------------

// The live conditioning table of one input source: at most one row per axis key, and an axis with no row passes
// through untouched. This is params rather than settings because the whole point of the stage is that a game
// retunes it while it runs — Request_SetAxisBias edits this array in place.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Fragment_InputBias_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InputBias_ParamsData);

    friend class ck::FProcessor_InputBias_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_InputBias_AxisBias> _AxisBiases;

public:
    CK_PROPERTY_GET(_AxisBiases);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InputBias_ParamsData, _AxisBiases);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputBias_SetAxisBias : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputBias_SetAxisBias);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputBias_SetAxisBias);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_InputBias_AxisBias _AxisBias;

public:
    CK_PROPERTY_GET(_AxisBias);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputBias_SetAxisBias, _AxisBias);
};

// --------------------------------------------------------------------------------------------------------------------
