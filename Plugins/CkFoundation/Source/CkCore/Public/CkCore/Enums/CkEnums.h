#pragma once

#include "CkFormat/CkFormat.h"

#include "CkEnums.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * All common enums go in this file
*/

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Replication : uint8
{
    Replicates,
    DoesNotReplicate
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Replication);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ScaledUnscaled : uint8
{
    Scaled,
    Unscaled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ScaledUnscaled);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SucceededFailed : uint8
{
    Succeeded,
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SucceededFailed);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ValidInvalid : uint8
{
    Valid,
    Invalid
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ValidInvalid);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RelativeAbsolute : uint8
{
    Relative,
    Absolute
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RelativeAbsolute);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LocalWorld : uint8
{
    Local,
    World
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_LocalWorld);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Edges : uint8
{
    Left,
    Right,
    Top,
    Bottom
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Edges);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Direction_2D : uint8
{
    Left,
    Right,
    Up,
    Down
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Direction_2D);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Direction_3D : uint8
{
    Left,
    Right,
    Forward,
    Back,
    Up,
    Down
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Direction_3D);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PitchYawRoll : uint8
{
    Pitch,
    Yaw,
    Roll
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PitchYawRoll);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Logic_And_Or : uint8
{
    And,
    Or
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Logic_And_Or);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EnableDisable : uint8
{
    Enable,
    Disable
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EnableDisable);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_BeginEndOverlap : uint8
{
    BeginOverlap,
    EndOverlap,
    Both
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BeginEndOverlap);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Vector_Axis : uint8
{
    X UMETA (DisplayName = "X (Forward)"),
    Y UMETA (DisplayName = "Y (Right)"),
    Z UMETA (DisplayName = "Z (Up)"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vector_Axis);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Plane_Axis : uint8
{
    XY,
    XZ,
    YZ
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Plane_Axis);

// --------------------------------------------------------------------------------------------------------------------
