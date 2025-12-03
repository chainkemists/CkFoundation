#pragma once

#include "CkCore/Macros/CkMacros.h"
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

UENUM(BlueprintType)
enum class ECk_Pmg_DebugShape_Type : uint8
{
    Sphere,
    Box,
    Circle,
    Cone,
    Cylinder,
    Capsule
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

USTRUCT(BlueprintType)
struct CKPMG_API FCk_Fragment_Pmg_DebugShape_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Pmg_DebugShape_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Pmg_DebugShape_Type _ShapeType = ECk_Pmg_DebugShape_Type::Sphere;

    // Primary dimension: radius for sphere/circle/cone/cylinder/capsule, X extent for box
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.1"))
    float _Size = 100.0f;

    // Secondary dimension: height for cone/cylinder, halfHeight for capsule, Y/Z extent for box
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.1"))
    float _SecondarySize = 100.0f;

    // Tertiary dimension: Z extent for box (when needed)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.1"))
    float _TertiarySize = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "3", ClampMax = "128"))
    int32 _Segments = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "2", ClampMax = "64"))
    int32 _Rings = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _DrawLines = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _LineThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _DrawDirectionLine = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _Duration = FCk_Time(0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _EnableCollision = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Pmg_RenderMode _RenderMode = ECk_Pmg_RenderMode::DoubleSided;

public:
    CK_PROPERTY(_ShapeType);
    CK_PROPERTY(_Size);
    CK_PROPERTY(_SecondarySize);
    CK_PROPERTY(_TertiarySize);
    CK_PROPERTY(_Segments);
    CK_PROPERTY(_Rings);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_DrawLines);
    CK_PROPERTY(_LineThickness);
    CK_PROPERTY(_DrawDirectionLine);
    CK_PROPERTY(_Duration);
    CK_PROPERTY(_EnableCollision);
    CK_PROPERTY(_RenderMode);
};

// --------------------------------------------------------------------------------------------------------------------
