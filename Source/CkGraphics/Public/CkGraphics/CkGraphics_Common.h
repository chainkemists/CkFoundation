#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Materials/MaterialInterface.h>

#include "CkGraphics_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_MeshMaterialOverride
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MeshMaterialOverride);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0))
    int32 _MaterialSlot = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _ReplacementMaterial;

public:
    CK_PROPERTY_GET(_MaterialSlot);
    CK_PROPERTY_GET(_ReplacementMaterial);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MeshMaterialOverride, _MaterialSlot, _ReplacementMaterial);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CustomPrimitiveData_Type : uint8
{
    Float,
    Vector2,
    Vector3,
    Vector4,
    LinearColor
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CustomPrimitiveData_Type);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_CustomPrimitiveData_Value
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CustomPrimitiveData_Value);

public:
    FCk_CustomPrimitiveData_Value() = default;
    explicit FCk_CustomPrimitiveData_Value(float InFloat);
    explicit FCk_CustomPrimitiveData_Value(const FVector2D& InVector2);
    explicit FCk_CustomPrimitiveData_Value(const FVector& InVector3);
    explicit FCk_CustomPrimitiveData_Value(const FVector4& InVector4);
    explicit FCk_CustomPrimitiveData_Value(const FLinearColor& InLinearColor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_CustomPrimitiveData_Type _Type = ECk_CustomPrimitiveData_Type::Float;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_Type == ECk_CustomPrimitiveData_Type::Float", EditConditionHides))
    float _Float = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_Type == ECk_CustomPrimitiveData_Type::Vector2", EditConditionHides))
    FVector2D _Vector2 = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_Type == ECk_CustomPrimitiveData_Type::Vector3", EditConditionHides))
    FVector _Vector3 = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_Type == ECk_CustomPrimitiveData_Type::Vector4", EditConditionHides))
    FVector4 _Vector4 = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition="_Type == ECk_CustomPrimitiveData_Type::LinearColor", EditConditionHides))
    FLinearColor _LinearColor = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Type);
    CK_PROPERTY_GET(_Float);
    CK_PROPERTY_GET(_Vector2);
    CK_PROPERTY_GET(_Vector3);
    CK_PROPERTY_GET(_Vector4);
    CK_PROPERTY_GET(_LinearColor);

public:
    auto
    Get_FloatCount() const -> int32;

    auto
    ConvertToFloatArray() const -> TArray<float>;

    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_CustomPrimitiveData_Value, _Float);
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_CustomPrimitiveData_Value, _Vector2);
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_CustomPrimitiveData_Value, _Vector3);
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_CustomPrimitiveData_Value, _Vector4);
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_CustomPrimitiveData_Value, _LinearColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_CustomPrimitiveData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CustomPrimitiveData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0))
    int32 _CustomDataIndex  = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_CustomPrimitiveData_Value _Value;

public:
    CK_PROPERTY_GET(_CustomDataIndex );
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CustomPrimitiveData, _CustomDataIndex , _Value);
};

// --------------------------------------------------------------------------------------------------------------------
