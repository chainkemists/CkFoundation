#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <Materials/MaterialInterface.h>

#include "CkPmg_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPMG_API FCk_Handle_Pmg_Donut : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Pmg_Donut); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Pmg_Donut);

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPMG_API FCk_Handle_Pmg_DebugShape : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Pmg_DebugShape); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Pmg_DebugShape);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pmg_Override : uint8
{
    UseExisting,
    Override
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_Override);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pmg_RenderMode : uint8
{
    SingleSided,
    DoubleSided,
    Hidden
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_RenderMode);

// --------------------------------------------------------------------------------------------------------------------
//
// Cone apex direction in shape-local space. The mesh + wireframe are built so the apex points
// along the chosen axis BEFORE the per-shape ECk_Plane_Axis rotation is applied. Use this to
// avoid the "Pitch=-90 in the SceneNode local rotation" workaround that consumers (gym agents,
// crowd debug) had to repeat to get an apex-forward cone — set Forward and the rotation is
// baked into the mesh.
//
// Default Up matches the historical behavior: apex at +Z, base on the XY plane.
//
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
struct CKPMG_API FCk_Fragment_Pmg_Donut_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Pmg_Donut_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _InnerRadius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _OuterRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "3", ClampMax = "128"))
    int32 _Segments = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "360.0"))
    float _FillAngle = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _Material;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _EnableCollision = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Pmg_RenderMode _RenderMode = ECk_Pmg_RenderMode::DoubleSided;

public:
    CK_PROPERTY(_InnerRadius);
    CK_PROPERTY(_OuterRadius);
    CK_PROPERTY(_Segments);
    CK_PROPERTY(_FillAngle);
    CK_PROPERTY(_Material);
    CK_PROPERTY(_EnableCollision);
    CK_PROPERTY(_RenderMode);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Request_Pmg_Donut_UpdateParams : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Pmg_Donut_UpdateParams);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Pmg_Donut_UpdateParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<float> _InnerRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<float> _OuterRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<int32> _Segments;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<float> _FillAngle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<UMaterialInterface*> _Material;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<bool> _EnableCollision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TOptional<ECk_Pmg_RenderMode> _RenderMode;

public:
    CK_PROPERTY(_InnerRadius);
    CK_PROPERTY(_OuterRadius);
    CK_PROPERTY(_Segments);
    CK_PROPERTY(_FillAngle);
    CK_PROPERTY(_Material);
    CK_PROPERTY(_EnableCollision);
    CK_PROPERTY(_RenderMode);
};

// --------------------------------------------------------------------------------------------------------------------
// Generic mutation requests for any PMG debug shape. The
// FFragment_Pmg_DebugShape_Common fragment is shared by every shape variant
// (basic / angular / directional / icon / symbol), so these requests work
// against the FCk_Handle_Pmg_DebugShape type-safe handle uniformly. Issued
// via UCk_Utils_Pmg_DebugShape_UE::Request_*; consumed by
// FProcessor_Pmg_DebugShape_HandleRequests, which updates the cached
// Common-fragment field AND any side effects on the live procmesh
// (material parameters, visibility, collision).
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
