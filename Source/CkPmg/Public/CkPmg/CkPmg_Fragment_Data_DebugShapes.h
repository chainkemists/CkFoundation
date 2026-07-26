#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPmg_Fragment_Data_DebugShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPMG_API FCk_Handle_Pmg_DebugShape : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Pmg_DebugShape); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Pmg_DebugShape);

// --------------------------------------------------------------------------------------------------------------------
// Cone apex direction in shape-local space, baked into the mesh + wireframe BEFORE the
// per-shape ECk_Plane_Axis rotation is applied.
UENUM(BlueprintType)
enum class ECk_Pmg_ConeOrientation : uint8
{
    Up,         // apex along +Z (default — backward-compatible with all existing callers)
    Forward,    // apex along +X (use for facing indicators on agents / characters)
    Right,      // apex along +Y
    Down,       // apex along -Z (drop indicators)
    Backward,   // apex along -X
    Left        // apex along -Y
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_ConeOrientation);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pmg_TextAlign : uint8
{
    Left,
    Center,
    Right
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_TextAlign);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pmg_DebugShape_Type : uint8
{
    Sphere,
    Box,
    Circle,
    Cone,
    Cylinder,
    Capsule,
    Arrow,
    Ring,
    Wedge,
    Frustum,
    Arc,
    Torus,
    Cross,
    WedgeCone,
    Star,
    Plane,
    Pyramid,
    Hemisphere,
    DashedLine,
    Checkmark,
    Diamond,
    Pivot,
    Warning,
    Prohibition,
    NoEntry,
    MagnifyingGlass,
    QuestionMark,
    ExclamationMark,
    Flag,
    InfoCircle,
    Pin,
    Text
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_DebugShape_Type);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetColor : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetColor);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetColor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _NewColor = FLinearColor::White;

public:
    CK_PROPERTY_GET(_NewColor);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetColor, _NewColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetText : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetText);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetText);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FString _NewText;

public:
    CK_PROPERTY_GET(_NewText);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetText, _NewText);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetLineThickness : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetLineThickness);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetLineThickness);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _NewLineThickness = 2.0f;

public:
    CK_PROPERTY_GET(_NewLineThickness);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetLineThickness, _NewLineThickness);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetDrawLines : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetDrawLines);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetDrawLines);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _NewDrawLines = true;

public:
    CK_PROPERTY_GET(_NewDrawLines);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetDrawLines, _NewDrawLines);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetDuration : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetDuration);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetDuration);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _NewDuration = FCk_Time{0.0f};

public:
    CK_PROPERTY_GET(_NewDuration);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetDuration, _NewDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetRenderMode : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetRenderMode);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetRenderMode);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Pmg_RenderMode _NewRenderMode = ECk_Pmg_RenderMode::DoubleSided;

public:
    CK_PROPERTY_GET(_NewRenderMode);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetRenderMode, _NewRenderMode);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_DebugShape_SetEnableCollision : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_DebugShape_SetEnableCollision);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_DebugShape_SetEnableCollision);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _NewEnableCollision = false;

public:
    CK_PROPERTY_GET(_NewEnableCollision);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Pmg_DebugShape_SetEnableCollision, _NewEnableCollision);
};

// --------------------------------------------------------------------------------------------------------------------
