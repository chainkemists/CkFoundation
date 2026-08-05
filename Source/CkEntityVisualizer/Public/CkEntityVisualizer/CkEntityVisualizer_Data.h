#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkEntityVisualizer_Data.generated.h"

UENUM(BlueprintType)
enum class ECk_EntityVisualizer_Primitive : uint8
{
    Box,
    Sphere,
    Cylinder,
    Cone,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityVisualizer_Primitive);

USTRUCT(BlueprintType)
struct CKENTITYVISUALIZER_API FCk_EntityVisualizer_IsmPrimitiveParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityVisualizer_IsmPrimitiveParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_EntityVisualizer_Primitive _Primitive = ECk_EntityVisualizer_Primitive::Sphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FTransform _LocalTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY(_Primitive);
    CK_PROPERTY(_LocalTransform);
    CK_PROPERTY(_Color);
};

USTRUCT(BlueprintType)
struct CKENTITYVISUALIZER_API FCk_EntityVisualizer_TransformGizmoParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityVisualizer_TransformGizmoParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin=1.0, UIMin=1.0))
    float _AxisLength = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin=0.1, UIMin=0.1))
    float _ShaftRadius = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin=1.0, UIMin=1.0))
    float _ArrowHeadLength = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin=0.1, UIMin=0.1))
    float _ArrowHeadRadius = 3.0f;

public:
    CK_PROPERTY(_AxisLength);
    CK_PROPERTY(_ShaftRadius);
    CK_PROPERTY(_ArrowHeadLength);
    CK_PROPERTY(_ArrowHeadRadius);
};
