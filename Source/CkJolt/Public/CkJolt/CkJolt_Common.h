#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MotionType : uint8
{
    Static = 0,
    Kinematic,
    Dynamic,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MotionType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_BackFaceMode : uint8
{
    IgnoreBackFaces,
    CollideWithBackFaces,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BackFaceMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MotionQuality : uint8
{
    // FAST - use this for most bodies
    Discrete UMETA(DisplayName = "Discrete"),
    // SLOWER - only when continuous collision detection is needed. On a Probe this is a manual CastShape
    // sweep (Jolt sensors don't support LinearCast); on a JoltBody it is Jolt's NATIVE LinearCast quality.
    LinearCast UMETA(DisplayName = "LinearCast (CCD)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MotionQuality);

// --------------------------------------------------------------------------------------------------------------------
