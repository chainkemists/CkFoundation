#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Materials/MaterialInterface.h>

#include "CkGraphics_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UTexture;

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

UENUM(BlueprintType)
enum class ECk_Material_ParameterType : uint8
{
    Scalar,
    Color,
    Texture
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Material_ParameterType);

// --------------------------------------------------------------------------------------------------------------------

/**
 * A single Material parameter paired with the value to drive it. Tagged-union shape (mirrors
 * FCk_CustomPrimitiveData_Value), restricted to the parameter kinds a UMaterialInstanceDynamic can set at
 * runtime: Scalar (float), Color (FLinearColor) and Texture. _Override gates whether a consumer should
 * apply this value or leave the material's own default in place (non-destructive); the apply helper
 * UCk_Utils_Graphics_UE::Apply_MaterialParameter does not consult it — the caller decides. _Type is
 * normally assigned by discovery from the source material, not hand-picked.
 */
USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Material_Parameter
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Material_Parameter);

public:
    FCk_Material_Parameter() = default;
    FCk_Material_Parameter(FName InParameterName, float InValue);
    FCk_Material_Parameter(FName InParameterName, FLinearColor InValue);
    FCk_Material_Parameter(FName InParameterName, UTexture* InValue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Parameter",
              meta = (AllowPrivateAccess = true))
    FName _ParameterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Parameter",
              meta = (AllowPrivateAccess = true))
    ECk_Material_ParameterType _Type = ECk_Material_ParameterType::Scalar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Parameter",
              meta = (AllowPrivateAccess = true))
    bool _Override = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Parameter",
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_Type == ECk_Material_ParameterType::Scalar", EditConditionHides))
    float _ScalarValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Parameter",
              meta = (AllowPrivateAccess = true, sRGB = "true",
                      EditCondition = "_Type == ECk_Material_ParameterType::Color", EditConditionHides))
    FLinearColor _ColorValue = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Parameter",
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_Type == ECk_Material_ParameterType::Texture", EditConditionHides))
    TObjectPtr<UTexture> _TextureValue = nullptr;

public:
    CK_PROPERTY(_ParameterName);
    CK_PROPERTY(_Type);
    CK_PROPERTY(_Override);
    CK_PROPERTY(_ScalarValue);
    CK_PROPERTY(_ColorValue);
    CK_PROPERTY(_TextureValue);

public:
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_Material_Parameter, _ParameterName, _ScalarValue)
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_Material_Parameter, _ParameterName, _ColorValue)
};

// --------------------------------------------------------------------------------------------------------------------
