#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Generic Jolt motion vocabulary. Migrated from CkSpatialQuery's Probe (jolt-collision-world
// campaign Phase 3) — CoreRedirects in Config/DefaultCkFoundation.ini keep serialized BP
// references valid; AngelScript rebinds by short name automatically.
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
    // SLOWER - only when continuous collision detection is needed. On a Probe this is the manual
    // CastShape sweep (Jolt sensors don't support LinearCast); on a JoltBody it maps to Jolt's
    // NATIVE LinearCast motion quality.
    LinearCast UMETA(DisplayName = "LinearCast (CCD)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MotionQuality);

// --------------------------------------------------------------------------------------------------------------------
