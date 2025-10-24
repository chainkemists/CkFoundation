#include "CkGraphics_Common.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_CustomPrimitiveData_Value::
    FCk_CustomPrimitiveData_Value(
        float InFloat)
    : _Type(ECk_CustomPrimitiveData_Type::Float)
    , _Float(InFloat)
{
}

FCk_CustomPrimitiveData_Value::
    FCk_CustomPrimitiveData_Value(
        const FVector2D& InVector2)
    : _Type(ECk_CustomPrimitiveData_Type::Vector2)
    , _Vector2(InVector2)
{
}

FCk_CustomPrimitiveData_Value::
    FCk_CustomPrimitiveData_Value(
        const FVector& InVector3)
    : _Type(ECk_CustomPrimitiveData_Type::Vector3)
    , _Vector3(InVector3)
{
}

FCk_CustomPrimitiveData_Value::
    FCk_CustomPrimitiveData_Value(
        const FVector4& InVector4)
    : _Type(ECk_CustomPrimitiveData_Type::Vector4)
    , _Vector4(InVector4)
{
}

FCk_CustomPrimitiveData_Value::
    FCk_CustomPrimitiveData_Value(
        const FLinearColor& InLinearColor)
    : _Type(ECk_CustomPrimitiveData_Type::LinearColor)
    , _LinearColor(InLinearColor)
{
}

auto
    FCk_CustomPrimitiveData_Value::
    Get_FloatCount() const
    -> int32
{
    switch (_Type)
    {
        case ECk_CustomPrimitiveData_Type::Float:
        {
            return 1;
        }
        case ECk_CustomPrimitiveData_Type::Vector2:
        {
            return 2;
        }
        case ECk_CustomPrimitiveData_Type::Vector3:
        {
            return 3;
        }
        case ECk_CustomPrimitiveData_Type::Vector4:
        case ECk_CustomPrimitiveData_Type::LinearColor:
        {
            return 4;
        }
        default:
        {
            CK_INVALID_ENUM(_Type);
            return 0;
        }
    }
}

auto
    FCk_CustomPrimitiveData_Value::
    ConvertToFloatArray() const
    -> TArray<float>
{
    auto Result = TArray<float>{};

    switch (_Type)
    {
        case ECk_CustomPrimitiveData_Type::Float:
        {
            Result.Add(_Float);
            break;
        }
        case ECk_CustomPrimitiveData_Type::Vector2:
        {
            Result.Add(_Vector2.X);
            Result.Add(_Vector2.Y);
            break;
        }
        case ECk_CustomPrimitiveData_Type::Vector3:
        {
            Result.Add(_Vector3.X);
            Result.Add(_Vector3.Y);
            Result.Add(_Vector3.Z);
            break;
        }
        case ECk_CustomPrimitiveData_Type::Vector4:
        {
            Result.Add(_Vector4.X);
            Result.Add(_Vector4.Y);
            Result.Add(_Vector4.Z);
            Result.Add(_Vector4.W);
            break;
        }
        case ECk_CustomPrimitiveData_Type::LinearColor:
        {
            Result.Add(_LinearColor.R);
            Result.Add(_LinearColor.G);
            Result.Add(_LinearColor.B);
            Result.Add(_LinearColor.A);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(_Type);
            break;
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
